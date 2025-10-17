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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tag_to_msg_impl.h"
#include <gnuradio/io_signature.h>
#include <iomanip>
#include <iostream>

namespace gr {
namespace rds {

tag_to_msg::sptr tag_to_msg::make(size_t sizeof_stream_item) {
  return gnuradio::get_initial_sptr(new tag_to_msg_impl(sizeof_stream_item));
}

tag_to_msg_impl::tag_to_msg_impl(size_t sizeof_stream_item)
    : sync_block("tag_to_msg", io_signature::make(1, -1, sizeof_stream_item),
                 io_signature::make(0, 0, 0)),
      d_port(pmt::mp("strobe")) {
  message_port_register_out(d_port);
}

tag_to_msg_impl::~tag_to_msg_impl() {}

int tag_to_msg_impl::work(int noutput_items,
                          gr_vector_const_void_star &input_items,
                          gr_vector_void_star &output_items) {
  for (size_t i = 0; i < input_items.size(); i++) {
    d_tags.clear();
    get_tags_in_window(d_tags, i, 0, noutput_items, pmt::mp("rds_latency_strobe"));

    for (d_tags_itr = d_tags.begin(); d_tags_itr != d_tags.end();
         d_tags_itr++) {
      pmt::pmt_t d = pmt::make_dict();
      d = pmt::dict_add(d, pmt::mp(d_tags_itr->key),
                        pmt::mp(d_tags_itr->value));
      message_port_pub(d_port, pmt::cons(d, pmt::PMT_NIL));
    }
  }

  return noutput_items;
}

} /* namespace rds */
} /* namespace gr */
