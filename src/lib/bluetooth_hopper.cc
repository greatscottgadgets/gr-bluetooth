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

#include "bluetooth_hopper.h"
#include "bluetooth_packet.h"

/*
 * Create a new instance of bluetooth_hopper and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
bluetooth_hopper_sptr 
bluetooth_make_hopper (int LAP, int channel)
{
  return bluetooth_hopper_sptr (new bluetooth_hopper (LAP, channel));
}

//private constructor
bluetooth_hopper::bluetooth_hopper (int LAP, int channel)
  : bluetooth_UAP (LAP)
{
	printf("Bluetooth hopper\n\n");

	d_channel = channel;

	d_piconet = bluetooth_make_piconet(d_LAP);
}

//virtual destructor.
bluetooth_hopper::~bluetooth_hopper ()
{
}

int bluetooth_hopper::work (int noutput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{
	char *in = (char *) input_items[0];
	int retval, consumed;
	int num_candidates = -1;
	uint32_t clkn=0; /* native (local) clock in 625 us */

	retval = bluetooth_packet::sniff_ac(in, noutput_items);
	if(-1 == retval) {
		consumed = noutput_items;
	} else {
		consumed = retval;
		bluetooth_packet_sptr packet = bluetooth_make_packet(
				&in[retval], noutput_items + history() - retval,
				clkn, d_channel);
		if (packet->get_LAP() == d_LAP && packet->header_present()) {
			clkn = ((d_cumulative_count + consumed) / 625) & 0x7ffffff;
			if (!d_piconet->have_clk6()) {
				/* working on CLK1-6/UAP discovery */
				d_piconet->UAP_from_header(packet);
				if (d_piconet->have_clk6()) {
					/* got CLK1-6/UAP, start working on CLK1-27 */
					d_piconet->init_hop_reversal(false);
					/* use previously observed packets to eliminate candidates */
					num_candidates = d_piconet->winnow();
					printf("%d CLK1-27 candidates remaining\n", num_candidates);
				}
			} else {
				/* continue working on CLK1-27 */
				/* we need timing information from an additional packet, so run through UAP_from_header() again */
				d_piconet->UAP_from_header(packet);
				if (d_piconet->have_clk6()) {
					num_candidates = d_piconet->winnow();
				}
			}
			/* CLK1-27 results */
			if (num_candidates == 1) {
				/* win! */
				exit(0);
			}
		}
		consumed += 126;
	}
	d_cumulative_count += consumed;

	// Tell runtime system how many output items we produced.
	return consumed;
}
