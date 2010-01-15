/* -*- c++ -*- */
/*
 * Copyright 2010 Michael Ossmann
 * 
 * This file is part of gr-bluetooth
 * 
 * gr-bluetooth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * gr-bluetooth is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with gr-bluetooth; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "bluetooth_top_block.h"
#include <gr_io_signature.h>
#include <gr_file_source.h>

// Shared pointer constructor
bluetooth_top_block_sptr bluetooth_make_top_block()
{
  return gnuradio::get_initial_sptr(new bluetooth_top_block());
}

// Hierarchical block constructor, with no inputs or outputs
bluetooth_top_block::bluetooth_top_block() : 
	gr_top_block("bluetooth_top_block")
{
	//FIXME using file source for testing
	gr_file_source_sptr src = gr_make_file_source (sizeof(gr_complex), "/tmp/headset3.cfile", true);
	double sample_rate = 2000000;
	double center_freq = 2476000000;
	double squelch_threshold = -1000;
	sink = bluetooth_make_kismet_block(sample_rate, center_freq, squelch_threshold);

	connect(src, 0, sink, 0);
}
//FIXME handle termination
