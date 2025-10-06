/*
 * Copyright 2025 Samantaz Fox
 *
 * This file is part of gr-rds
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <gnuradio/io_signature.h>
#include "diffenc_symgen_impl.h"

namespace gr {
namespace rds {

diffenc_symgen::sptr diffenc_symgen::make()
{
	return gnuradio::make_block_sptr<diffenc_symgen_impl>();
}

diffenc_symgen_impl::diffenc_symgen_impl()
	: sync_interpolator("diffenc_symgen",
	                    gr::io_signature::make(1, 1, sizeof(unsigned char)),
	                    gr::io_signature::make(1, 1, sizeof(float)),
	                    2),
	  d_last_out(0)
{
}

diffenc_symgen_impl::~diffenc_symgen_impl() {}

int diffenc_symgen_impl::work(int noutput_items,
                              gr_vector_const_void_star &input_items,
                              gr_vector_void_star &output_items)
{
	gr::thread::scoped_lock guard(d_mutex);

	const unsigned char* in = (const unsigned char*)input_items[0];
	float* out = (float*)output_items[0];

	int o = 0;
	unsigned last_out = d_last_out;

	// Because of unpacking, we have a in:out ratio of 1:2
	for (int i = 0; i < (noutput_items / 2); i++) {
		// Differential encoder
		last_out = (in[i] ^ last_out) & 1;

		// Map [1,2] + unpack_k_bits + chunks to symbols [-1, 1]
		if (last_out == 0) {
			out[o++] = -1;
			out[o++] = 1;
		} else {
			out[o++] = 1;
			out[o++] = -1;
		}
	}

	d_last_out = last_out;

	return o;
}

} /* namespace rds */
} /* namespace gr */
