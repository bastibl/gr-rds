/*
 * Copyright (C) 2014, 2016 Bastian Bloessl <bloessl@ccs-labs.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "encoder_impl.h"
#include "constants.h"
#include <gnuradio/io_signature.h>
#include <boost/spirit/include/qi.hpp>
#include <math.h>
#include <ctype.h>
#include <time.h>
#include <cstdio>
#include <iostream>

using namespace gr::rds;

encoder_impl::encoder_impl(unsigned char pty_locale, int pty, bool ms, std::string ps,
                           bool af, double af1, bool tp, bool ta, bool tmc, bool ct,
                           int pi_country_code, int pi_coverage_area, int pi_reference_number,
                           std::string radiotext, bool ecc, unsigned char ecc_code)
    : gr::sync_block("gr_rds_encoder", gr::io_signature::make(0, 0, 0), gr::io_signature::make(1, 1, sizeof(unsigned char))),

      // RDS Parameters and Flags
      d_pty_locale(pty_locale),   // PTY display standard (Europe or North America)
      d_pi(0),                    // Program Identification, calculated in constructor body
      d_pty(pty),                 // programm type (education)
      d_af1(af1),                 // alternate frequency 1
      d_ecc_code(ecc_code),       // Value for the new ECC feature
      d_ms(ms),                   // music/speech switch (1=music)
      d_tp(tp),                   // traffic programm
      d_ta(ta),                   // traffic announcement

      // Feature Flags
      d_af(af),                   // Enable sending Alternate Frequency in Group 0A
      d_tmc(tmc),                 // Enable sending TMC groups
      d_ct(ct),                   // Enable sending Clock-Time group
      d_ecc(ecc),                 // Enable sending Extended Country Code group

      // Internal State
      d_buffer(nullptr),
      d_is_group4a(nullptr),
      d_nbuffers(0),
      d_ps_segment_index(0),
      d_radiotext_segment_index(0),
      d_tmc_segment_index(0),
      d_current_buffer(0),
      d_buffer_bit_counter(0),
      d_last_ct_time(0),

      // Hard-coded TMC Data
      d_tmc_alert_data({3, 2, 1340, 11023})
{
	message_port_register_in(pmt::mp("rds in"));
	set_msg_handler(pmt::mp("rds in"), [this](pmt::pmt_t msg) { this->rds_in(msg); });

	std::memset(d_infoword,    0, sizeof(d_infoword));
	std::memset(d_checkword,   0, sizeof(d_checkword));
	std::memset(d_groups,      0, sizeof(d_groups));
    std::memset(d_radiotext,   ' ', sizeof(d_radiotext));
    std::memset(d_ps,          ' ', sizeof(d_ps));

	if (pi_country_code == 0) {
		d_pi = pi_reference_number;
	} else {
		d_pi = (pi_country_code & 0xF) << 12 | (pi_coverage_area & 0xF) << 8 | (pi_reference_number);
	}

	set_radiotext(radiotext);
	set_ps(ps);

	// Configure which groups are set based on flags
	d_groups[0] = 1;  // basic tuning and switching
	d_groups[2] = 1;  // radio text
	d_groups[11] = 1; // Open Data Applications (in-house)
    if (d_ecc) { d_groups[1] = 1; } // Extended Country Code
    if (d_ct)  { d_groups[4] = 1; } // clock time
    if (d_tmc) {                  // tmc
        d_groups[3] = 1; // announce TMC
        d_groups[8] = 1;
    }

    rebuild();
}

encoder_impl::~encoder_impl() {
    if (d_buffer) {
        for(int i = 0; i < d_nbuffers; i++) {
            free(d_buffer[i]);
        }
        free(d_buffer);
    }
    free(d_is_group4a);
}

void encoder_impl::rebuild() {
	gr::thread::scoped_lock lock(d_mutex);

    if (d_buffer) {
        for(int i = 0; i < d_nbuffers; i++) {
            free(d_buffer[i]);
        }
        free(d_buffer);
        d_buffer = nullptr;
    }
    free(d_is_group4a);
    d_is_group4a = nullptr;

	count_groups();
	d_current_buffer = 0;
    d_last_ct_time = 0; // Reset last update time on rebuild

	// allocate memory for nbuffers buffers of 104 unsigned chars each
	d_buffer = (unsigned char **)malloc(d_nbuffers * sizeof(unsigned char *));
    d_is_group4a = (char *)malloc(d_nbuffers * sizeof(char));
    std::memset(d_is_group4a, 0, d_nbuffers * sizeof(char));

	for(int i = 0; i < d_nbuffers; i++) {
		d_buffer[i] = (unsigned char *)malloc(104 * sizeof(unsigned char));
		for(int j = 0; j < 104; j++) d_buffer[i][j] = 0;
	}

	// prepare each of the groups
	for(int i = 0; i < 32; i++) {
		if(d_groups[i] == 1) {
            int group_type = i % 16;
            bool ab_flag = (i >= 16);

            // Mark the buffer if it is for Group 4A before creating it
            if (group_type == 4) {
                d_is_group4a[d_current_buffer] = 1;
            }

			create_group(group_type, ab_flag);
			if(group_type == 0)
				for(int j = 0; j < 3; j++) create_group(group_type, ab_flag);
			if(group_type == 2)
				for(int j = 0; j < 15; j++) create_group(group_type, ab_flag);
			if(group_type == 3)
				create_group(group_type, ab_flag);
		}
	}

	d_current_buffer = 0;
}

void encoder_impl::rds_in(pmt::pmt_t msg) {
	if(!pmt::is_pair(msg)) return;

	using boost::spirit::qi::phrase_parse;
	using boost::spirit::qi::lexeme;
	using boost::spirit::qi::char_;
	using boost::spirit::qi::hex;
	using boost::spirit::qi::uint_;
	using boost::spirit::qi::bool_;
	using boost::spirit::qi::double_;
	using boost::spirit::qi::space;
	using boost::spirit::qi::lit;

	int msg_len = pmt::blob_length(pmt::cdr(msg));
	std::string in = std::string((char*)pmt::blob_data(pmt::cdr(msg)), msg_len);

	unsigned int ui1;
	std::string s1;
	bool b1;
	double d1;

	if(phrase_parse(in.begin(), in.end(), "pty" >> (("0x" >> hex) | uint_), space, ui1)) {
		set_pty(ui1);
	} else if(phrase_parse(in.begin(), in.end(), "text" >> lexeme[+(char_ - '\n')] >> -lit("\n"), space, s1)) {
		set_radiotext(s1);
	} else if(phrase_parse(in.begin(), in.end(), "ps" >> lexeme[+(char_ - '\n')] >> -lit("\n"), space, s1)) {
		set_ps(s1);
	} else if(phrase_parse(in.begin(), in.end(), "ta" >> bool_, space, b1)) {
		set_ta(b1);
	} else if(phrase_parse(in.begin(), in.end(), "tp" >> bool_, space, b1)) {
		set_tp(b1);
	} else if(phrase_parse(in.begin(), in.end(), "ms" >> bool_, space, b1)) {
		set_ms(b1);
	} else if(phrase_parse(in.begin(), in.end(), "pi" >> lit("0x") >> hex, space, ui1)) {
		set_pi(ui1);
	} else if(phrase_parse(in.begin(), in.end(), "af1" >> double_, space, d1)) {
		set_af1(d1);
	} else {
		std::cout << "RDS: command not understood" << std::endl;
	}

	rebuild();
}

void encoder_impl::set_ms(bool ms) { d_ms = ms; }
void encoder_impl::set_af1(double af1) { d_af1 = af1; }
void encoder_impl::set_tp(bool tp) { d_tp = tp; }
void encoder_impl::set_ta(bool ta) { d_ta = ta; }
void encoder_impl::set_pty(unsigned int pty) { if (pty <= 31) d_pty = pty; }
void encoder_impl::set_pi(unsigned int pi) { if (pi <= 0xFFFF) d_pi = pi; }

void encoder_impl::set_radiotext(std::string text) {
	size_t len = std::min(sizeof(d_radiotext), text.length());
	std::memset(d_radiotext, ' ', sizeof(d_radiotext));
	std::memcpy(d_radiotext, text.c_str(), len);
}

void encoder_impl::set_ps(std::string ps) {
	size_t len = std::min(sizeof(d_ps), ps.length());
	std::memset(d_ps, ' ', sizeof(d_ps));
	std::memcpy(d_ps, ps.c_str(), len);
}

/* see Annex B, page 64 of the standard */
unsigned int encoder_impl::calc_syndrome(unsigned long message, unsigned char mlen) {
	unsigned long reg = 0;
	const unsigned long poly = 0x5B9;
	const unsigned char plen = 10;
	for (unsigned int i = mlen; i > 0; i--)  {
		reg = (reg << 1) | ((message >> (i - 1)) & 0x01);
		if (reg & (1 << plen)) reg = reg ^ poly;
	}
	for (unsigned int i = plen; i > 0; i--) {
		reg = reg << 1;
		if (reg & (1 << plen)) reg = reg ^ poly;
	}
	return reg & ((1 << plen) - 1);
}

/* see page 41 in the standard; this is an implementation of AF method A
 * FIXME need to add code that declares the number of AF to follow... */
unsigned int encoder_impl::encode_af(const double af) {
	unsigned int af_code = 0;
	if(( af >= 87.6) && (af <= 107.9))
		af_code = nearbyint((af - 87.5) * 10);
	else if((af >= 153) && (af <= 279))
		af_code = nearbyint((af - 144) / 9);
	else if((af >= 531) && (af <= 1602))
		af_code = nearbyint((af - 531) / 9 + 16);
	return af_code;
}

void encoder_impl::count_groups(void) {
	d_nbuffers = 0;
	for(int i = 0; i < 32; i++) {
		if(d_groups[i] == 1) {
			if(i % 16 == 0) d_nbuffers += 4;
			else if(i % 16 == 2) d_nbuffers += 16;
			else if(i % 16 == 3) d_nbuffers += 2;
			else d_nbuffers++;
		}
	}
}

/* create the 4 infowords, according to group type.
 * then calculate checkwords and put everything in the groups */
void encoder_impl::create_group(const int group_type, const bool AB) {
	d_infoword[0] = d_pi;
	d_infoword[1] = (((group_type & 0xf) << 12) | (AB << 11) | (d_tp << 10) | (d_pty << 5));

	if(group_type == 0) prepare_group0(AB);
	else if(group_type == 1) prepare_group1a();
	else if(group_type == 2) prepare_group2(AB);
	else if(group_type == 3) prepare_group3a();
	else if(group_type == 4) prepare_group4a();
	else if(group_type == 8) prepare_group8a();
	else if(group_type == 11) prepare_group11a();

	for(int i= 0; i < 4; i++) {
		d_checkword[i]=calc_syndrome(d_infoword[i], 16);
		d_block[i] = ((d_infoword[i] & 0xffff) << 10) | (d_checkword[i] & 0x3ff);
		// add the offset word
		if((i == 2) && AB) d_block[2] ^= offset_word[4];
		else d_block[i] ^= offset_word[i];
	}

	prepare_buffer(d_current_buffer);
	d_current_buffer++;
}

void encoder_impl::prepare_group0(const bool AB) {
	d_infoword[1] |= (d_ta << 4) | (d_ms << 3);
	//FIXME: make DI configurable
	if(d_ps_segment_index == 3)
		d_infoword[1] |= 0x5;  // d0=1 (stereo), d1-3=0
	d_infoword[1] |= (d_ps_segment_index & 0x3);
	if(!AB) { // This is Group 0A
        if (d_af) { // AF is enabled: transmit the AF code
            d_infoword[2] = (225 << 8) | // 1 AF follows
                (encode_af(d_af1/1000000) & 0xff);
        } else { // AF is disabled: repeat the PI code in this block for robustness
            d_infoword[2] = d_pi;
        }
	} else { // This is Group 0B
		d_infoword[2] = d_pi;
	}
	d_infoword[3] = (d_ps[2 * d_ps_segment_index] << 8) | d_ps[2 * d_ps_segment_index + 1];
	d_ps_segment_index = (d_ps_segment_index + 1) % 4;
}

void encoder_impl::prepare_group1a(void) {
	d_infoword[2] = (0x80 << 8) | d_ecc_code;
	d_infoword[3] = 0;
}

void encoder_impl::prepare_group2(const bool AB) {
	d_infoword[1] |= ((AB << 4) | (d_radiotext_segment_index & 0xf));
	if(!AB) {
		d_infoword[2] = (d_radiotext[d_radiotext_segment_index * 4] << 8 | d_radiotext[d_radiotext_segment_index * 4 + 1]);
		d_infoword[3] = (d_radiotext[d_radiotext_segment_index * 4 + 2] << 8 | d_radiotext[d_radiotext_segment_index * 4 + 3]);
	} else {
		d_infoword[2] = d_pi;
		d_infoword[3] = (d_radiotext[d_radiotext_segment_index * 2] << 8 | d_radiotext[d_radiotext_segment_index * 2 + 1]);
	}
	d_radiotext_segment_index = (d_radiotext_segment_index + 1) % 16;
}

void encoder_impl::prepare_group3a(void) {
	if(d_tmc_segment_index) {
		d_infoword[1] |= (0x31d0 & 0x1f);
		d_infoword[2] = 0x6280;
		d_infoword[3] = 0xcd46;
	} else {
		d_infoword[1] |= (0x31d0 & 0x1f);
		d_infoword[2] = 0x0066;
		d_infoword[3] = 0xcd46; // AID for TMC (Alert C)
	}
	d_tmc_segment_index = (d_tmc_segment_index + 1) % 2;
}

/* see page 28 and Annex G, page 81 in the standard
 * NOTE: The logic to transmit this group only once per minute, at the
 * top of the minute, is handled in the work() function. */
void encoder_impl::prepare_group4a(void) {
	time_t rightnow;
	time(&rightnow);
	tm *utc = gmtime(&rightnow);
    tm *local = localtime(&rightnow);

	/* we're supposed to send UTC time; the receiver should then add the
	* local timezone offset */
	int m = utc->tm_min;
	int h = utc->tm_hour;
	int D = utc->tm_mday;
	int M = utc->tm_mon + 1;  // January: M=0
	int Y = utc->tm_year;
	int toffset=local->tm_hour-h;

	int L = ((M == 1) || (M == 2)) ? 1 : 0;
	int mjd=14956+D+int((Y-L)*365.25)+int((M+1+L*12)*30.6001);

	d_infoword[1] |= ((mjd >> 15) & 0x3);
	d_infoword[2] = (((mjd >> 7) & 0xff) << 8) | ((mjd & 0x7f) << 1) | ((h >> 4) & 0x1);
	d_infoword[3] = ((h & 0xf) << 12) | (((m >> 2) & 0xf) << 8) | ((m & 0x3) << 6) |
		((toffset > 0 ? 0 : 1) << 5) | (abs(toffset * 2));
}

// TMC Alert-C
void encoder_impl::prepare_group8a(void) {
	d_infoword[1] |= (1 << 3) | (d_tmc_alert_data.duration_persistence & 0x7);
	d_infoword[2] = (1 << 15) | ((d_tmc_alert_data.extent & 0x7) << 11) | (d_tmc_alert_data.event_code & 0x7ff);
	d_infoword[3] = d_tmc_alert_data.location_code;
}

// for now single-group only
void encoder_impl::prepare_group11a(void) {
	d_infoword[1] |= (0xb1c8 & 0x1f);
	d_infoword[2] = 0x2038;
	d_infoword[3] = 0x4456;
}

void encoder_impl::prepare_buffer(int which) {
	for(int q = 0; q < 104; q++) {
		int a = q / 26;
		int b = 25 - (q % 26);
		d_buffer[which][q] = (unsigned char)(d_block[a] >> b) & 0x1;
	}
}

//////////////////////// WORK ////////////////////////////////////
int encoder_impl::work (int noutput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items) {

	gr::thread::scoped_lock lock(d_mutex);
	unsigned char *out = (unsigned char *) output_items[0];

	for(int i = 0; i < noutput_items; i++) {
        // At the start of a new group, check if it's a dynamic time group
        if (d_buffer_bit_counter == 0 && d_nbuffers > 0 && d_ct && d_is_group4a[d_current_buffer]) {
            time_t now;
            time(&now);
            // Update time only once per minute (at the top of the minute)
            if ((now / 60) != (d_last_ct_time / 60)) {
                d_last_ct_time = now;
                // Regenerate this specific group's data.
                const int group_type = 4;
                const bool AB = false;
                // 1. Prepare infowords with current time
                d_infoword[0] = d_pi;
                d_infoword[1] = (((group_type & 0xf) << 12) | (AB << 11) | (d_tp << 10) | (d_pty << 5));
                prepare_group4a();
                // 2. Calculate checkwords and blocks
                for(int k = 0; k < 4; k++) {
                    d_checkword[k] = calc_syndrome(d_infoword[k], 16);
                    d_block[k] = ((d_infoword[k] & 0xffff) << 10) | (d_checkword[k] & 0x3ff);
                    if((k == 2) && AB) d_block[k] ^= offset_word[4];
                    else d_block[k] ^= offset_word[k];
                }
                // 3. Populate the buffer for the current group
                prepare_buffer(d_current_buffer);
            }
        }

		out[i] = d_buffer[d_current_buffer][d_buffer_bit_counter];
		if(++d_buffer_bit_counter > 103) {
			d_buffer_bit_counter = 0;
            // Protect against division by zero if no buffers are configured
			if (d_nbuffers > 0) {
			    d_current_buffer = (d_current_buffer + 1) % d_nbuffers;
            } else {
                d_current_buffer = 0;
            }
		}
	}

	return noutput_items;
}

encoder::sptr encoder::make (unsigned char pty_locale, int pty, bool ms,
		std::string ps, bool af, double af1, bool tp,
		bool ta, bool tmc, bool ct, int pi_country_code, int pi_coverage_area,
		int pi_reference_number, std::string radiotext, bool ecc, unsigned char ecc_code) {

	return gnuradio::get_initial_sptr(
			new encoder_impl(pty_locale, pty, ms, ps, af, af1, tp, ta,
                    tmc, ct, pi_country_code, pi_coverage_area, pi_reference_number,
					radiotext, ecc, ecc_code));
}
