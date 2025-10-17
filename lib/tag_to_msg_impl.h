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

#ifndef INCLUDED_RDS_TAG_TO_MSG_IMPL_H
#define INCLUDED_RDS_TAG_TO_MSG_IMPL_H

#include <rds/tag_to_msg.h>

namespace gr {
namespace rds {

class tag_to_msg_impl : public tag_to_msg {
private:
  std::vector<tag_t> d_tags;
  std::vector<tag_t>::iterator d_tags_itr;
  const pmt::pmt_t d_port;

public:
  tag_to_msg_impl(size_t sizeof_stream_item);
  ~tag_to_msg_impl();

  int work(int noutput_items, gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
};

} // namespace rds
} // namespace gr

#endif /* INCLUDED_RDS_TAG_TO_MSG_IMPL_H */
