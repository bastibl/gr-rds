/*
 * Copyright 2025 Samantaz Fox
 *
 * This file is part of gr-rds
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_RDS_DIFFENC_SYMGEN_IMPL_H
#define INCLUDED_RDS_DIFFENC_SYMGEN_IMPL_H

#include <rds/diffenc_symgen.h>
#include <gnuradio/thread/thread.h>

namespace gr {
namespace rds {

class diffenc_symgen_impl : public diffenc_symgen
{
 public:
	diffenc_symgen_impl();
	~diffenc_symgen_impl() override;

	int work(
		int noutput_items,
		gr_vector_const_void_star& input_items,
		gr_vector_void_star& output_items
	) override;

 private:
	mutable gr::thread::mutex d_mutex;
	unsigned int d_last_out;
};

} /* namespace rds */
} /* namespace gr */

#endif /* INCLUDED_RDS_DIFFENC_SYMGEN_IMPL_H */
