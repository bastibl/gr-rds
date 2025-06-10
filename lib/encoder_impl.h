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

#ifndef INCLUDED_RDS_ENCODER_IMPL_H
#define INCLUDED_RDS_ENCODER_IMPL_H

#include <rds/encoder.h>
#include <gnuradio/thread/thread.h>
#include <time.h>

namespace gr {
namespace rds {

class encoder_impl : public encoder
{
public:
	encoder_impl(unsigned char pty_locale, int pty, bool ms, std::string ps,
                 bool af, double af1, bool tp, bool ta, bool tmc, bool ct,
                 int pi_country_code, int pi_coverage_area, int pi_reference_number,
                 std::string radiotext, bool ecc, unsigned char ecc_code);

    void set_ps(std::string ps) override;

private:
	~encoder_impl() override;

	// --- Member Functions ---
	int work(int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items) override;
	void rebuild();
	void set_ms(bool ms);
	void set_tp(bool tp);
	void set_ta(bool ta);
	void set_af1(double af1);
	void set_pty(unsigned int pty);
	void set_pi(unsigned int pty);
	void set_radiotext(std::string text);
	void count_groups();
	void create_group(const int, const bool);
	void prepare_group0(const bool);
	void prepare_group1a();
	void prepare_group2(const bool);
	void prepare_group3a();
	void prepare_group4a();
	void prepare_group8a();
	void prepare_group11a();
	void prepare_buffer(int);
	unsigned int encode_af(double);
	unsigned int calc_syndrome(unsigned long, unsigned char);
	void rds_in(pmt::pmt_t msg);


	// --- Member Variables ---

	// RDS Configuration & Data
	gr::thread::mutex d_mutex;
	unsigned char   d_pty_locale;
	unsigned int    d_pi;
	unsigned char   d_pty;
	double          d_af1;
	unsigned char   d_ecc_code;

	// Feature-Enabling Flags
	bool d_ms;
	bool d_tp;
	bool d_ta;
	bool d_af;
	bool d_tmc;
	bool d_ct;
	bool d_ecc;

	// Data Buffers
	unsigned char d_radiotext[64];
	unsigned char d_ps[8];

	// Internal State & Buffers
	unsigned int  d_infoword[4];
	unsigned int  d_checkword[4];
	unsigned int  d_block[4];
	unsigned char **d_buffer;
	char*         d_is_group4a;
	int           d_nbuffers;
	int           d_groups[32];

	// Message Segment Counters
	int d_ps_segment_index;
	int d_radiotext_segment_index;
	int d_tmc_segment_index;

	// Streaming counters
	int    d_current_buffer;
	int    d_buffer_bit_counter;
	time_t d_last_ct_time;

	// RDS-TMC Alert-C Data
	struct TmcAlertData {
		int duration_persistence;
		int extent;
		int event_code;
		int location_code;
	};
	TmcAlertData d_tmc_alert_data;
};

} /* namespace rds */
} /* namespace gr */

#endif /* INCLUDED_RDS_ENCODER_IMPL_H */
