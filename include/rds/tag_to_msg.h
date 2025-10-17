/* -*- c++ -*- */
/*
 * Copyright 2019 Derek Kozel.
 * Copyright 2025 Clayton Smith.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_RDS_TAG_TO_MSG_H
#define INCLUDED_RDS_TAG_TO_MSG_H

#include <gnuradio/sync_block.h>
#include <rds/api.h>

namespace gr {
namespace rds {
class RDS_API tag_to_msg : virtual public gr::sync_block {
public:
  typedef std::shared_ptr<tag_to_msg> sptr;
  static sptr make(size_t sizeof_stream_item);
};
} // namespace rds
} // namespace gr

#endif /* INCLUDED_RDS_TAG_TO_MSG_H */
