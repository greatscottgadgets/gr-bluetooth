/* -*- c++ -*- */
/*
 * Copyright 2008, 2009 Dominic Spill, Michael Ossmann                                                                                            
 * Copyright 2007 Dominic Spill                                                                                                                   
 * Copyright 2005, 2006 Free Software Foundation, Inc.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bluetooth_UAP.h"

/*
 * Create a new instance of bluetooth_UAP and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
bluetooth_UAP_sptr 
bluetooth_make_UAP (int LAP)
{
  return bluetooth_UAP_sptr (new bluetooth_UAP (LAP));
}

//private constructor
bluetooth_UAP::bluetooth_UAP (int LAP)
  : bluetooth_block ()
{
	d_LAP = LAP;

	printf("Bluetooth UAP sniffer\nUsing LAP:0x%06x\n\n", LAP);

	/* ensure that we are always given at least 3125 symbols (5 time slots) */
	set_history(3125);

	d_cumulative_count = 0;

	d_piconet = bluetooth_make_piconet(d_LAP);
}

//virtual destructor.
bluetooth_UAP::~bluetooth_UAP ()
{
}

int 
bluetooth_UAP::work (int noutput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{
	char *in = (char *) input_items[0];
	int retval, consumed;
	uint32_t clkn; /* native (local) clock in 625 us */

	retval = bluetooth_packet::sniff_ac(in, noutput_items);
	if(-1 == retval) {
		consumed = noutput_items;
	} else {
		consumed = retval;
		clkn = ((d_cumulative_count + consumed) / 625) & 0x7ffffff;

		bluetooth_packet_sptr packet = bluetooth_make_packet(
				&in[retval], noutput_items + history() - retval,
				clkn, 0);

		if (packet->get_LAP() == d_LAP && packet->header_present()) {
			if (d_piconet->UAP_from_header(packet))
				exit(0);
		}
		consumed += 126;
	}
	d_cumulative_count += consumed;

	// Tell runtime system how many output items we produced.
	return consumed;
}
