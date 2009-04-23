/* -*- c++ -*- */
/*
 * Copyright 2009 Dominic Spill & Mark Steward
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_BLUETOOTH_BLUECORE_SOURCE_H
#define INCLUDED_BLUETOOTH_BLUECORE_SOURCE_H

#include <gr_sync_block.h>

class bluetooth_bluecore_source;
typedef boost::shared_ptr<bluetooth_bluecore_source> bluetooth_bluecore_source_sptr;

bluetooth_bluecore_source_sptr
bluetooth_make_bluecore_source (int device);

/*!
 * \brief Read stream from Bluetooth bluecore
 * \ingroup source
 */

class bluetooth_bluecore_source : public gr_sync_block
{
  friend bluetooth_bluecore_source_sptr gr_make_file_source (int device);
 private:
  int		d_device;

 protected:
  bluetooth_bluecore_source (int device);

 public:
  ~bluetooth_bluecore_source ();

  int work (int noutput_items,
	    gr_vector_const_void_star &input_items,
	    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_BLUECORE_SOURCE_H */
