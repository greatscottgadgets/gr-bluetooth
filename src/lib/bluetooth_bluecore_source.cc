/* -*- c++ -*- */
/*
 * Copyright 2009 Dominic Spill & Mark Steward
 * 
 * This file is part of gr-bluetooth
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <bluetooth_bluecore_source.h>
#include <gr_io_signature.h>


bluetooth_bluecore_source::bluetooth_bluecore_source (char* device)
  : gr_sync_block ("file_source",
		   gr_make_io_signature (0, 0, 0),
		   gr_make_io_signature (1, 1, itemsize)))
{
    d_device = device;
    /* TODO
     * Do some sort of connecting to the device here
     * Throw errors if needed
     * d_device is the device ID (in case we have multiple dongles)
     */
}

// public constructor that returns a shared_ptr
// FIXME: static?
bluetooth_bluecore_source_sptr
gr_make_file_source (char* device)
{
  return bluetooth_bluecore_source_sptr (new bluetooth_bluecore_source (device));
}

bluetooth_bluecore_source::~bluetooth_bluecore_source ()
{
  
  /* TODO
   * Close the device if you need to
   * Don't expect this to be run, there's little chance that scripts end properly
   */
}

int 
bluetooth_bluecore_source::work (int noutput_items,
		      gr_vector_const_void_star &input_items,
		      gr_vector_void_star &output_items)
{
  /* TODO
   * Read data from dongle here
   * put the data in the output_items vector
   * return noutput_items, the number of items that you've put into the vector
   */

  return noutput_items;
}
