/* -*- c++ -*- */

%feature("autodoc", "1");		// generate python docstrings

%include "exception.i"
%include "gnuradio.i"			// the common stuff

%{
#include "gr_rds_bpsk_demod.h"
#include "gr_rds_data_decoder.h"
#include "gr_rds_data_encoder.h"
#include "gr_rds_freq_divider.h"
#include "gr_rds_freq_statistics.h"
#include <stdexcept>
%}

//------------------------------------------------------------------

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 *
 * This does some behind-the-scenes magic so we can
 * access howto_square_ff from python as howto.square_ff
 */

GR_SWIG_BLOCK_MAGIC (gr_rds, freq_statistics);

gr_rds_freq_statistics_sptr gr_rds_make_freq_statistics(unsigned int vlen,
			double start_freq,
			double step_freq);

class gr_rds_freq_statistics: public gr_sync_block
{
private:
	gr_rds_freq_statistics(unsigned int vlen,
			double start_freq,
			double step_freq);
};

//------------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC (gr_rds, data_decoder);

gr_rds_data_decoder_sptr gr_rds_make_data_decoder(gr_msg_queue_sptr msgq);

class gr_rds_data_decoder: public gr_sync_block
{
private:
	gr_rds_data_decoder(gr_msg_queue_sptr msgq);
public:
	void reset(void);
};

//------------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC (gr_rds, data_encoder);

gr_rds_data_encoder_sptr gr_rds_make_data_encoder(const char *xmlfile);

class gr_rds_data_encoder: public gr_sync_block
{
private:
	gr_rds_data_encoder(const char *xmlfile);
};


// ------------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC (gr_rds, freq_divider);

gr_rds_freq_divider_sptr gr_rds_make_freq_divider (unsigned int divider);

class gr_rds_freq_divider: public gr_sync_block
{
private:
	gr_rds_freq_divider (unsigned int divider);
};

// -----------------------------------------------------------------

GR_SWIG_BLOCK_MAGIC (gr_rds, bpsk_demod);

gr_rds_bpsk_demod_sptr gr_rds_make_bpsk_demod (double sampling_rate);

class gr_rds_bpsk_demod: public gr_block
{
private:
	gr_rds_bpsk_demod (double sampling_rate);
public:
	void reset(void);
};
