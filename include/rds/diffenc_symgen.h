/*
 * Copyright 2025 Samantaz Fox
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_RDS_DIFFENC_SYMGEN_H
#define INCLUDED_RDS_DIFFENC_SYMGEN_H

#include <rds/api.h>
#include <gnuradio/sync_interpolator.h>

namespace gr {
namespace rds {

class RDS_API diffenc_symgen : virtual public gr::sync_interpolator
{
 public:
	typedef std::shared_ptr<diffenc_symgen> sptr;
	static sptr make();
};

} // namespace rds
} // namespace gr

#endif /* INCLUDED_RDS_DIFFENC_SYMGEN_H */
