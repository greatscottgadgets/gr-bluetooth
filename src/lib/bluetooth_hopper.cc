/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
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

	d_have_clock6 = false;
	//FIXME should support more than one channel
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
	int retval, interval, difference, current_time, consumed;
	int num_candidates = -1;

	retval = bluetooth_packet::sniff_ac(in, noutput_items);
	if(-1 == retval) {
		consumed = noutput_items;
	} else {
		consumed = retval;
		bluetooth_packet_sptr packet = bluetooth_make_packet(&in[retval], 3125 + noutput_items - retval);
		if(packet->get_LAP() == d_LAP) {
			current_time = d_cumulative_count + consumed;
			/* number of samples elapsed since previous packet */
			difference = current_time - d_previous_packet_time;
			/* number of time slots elapsed since previous packet */
			interval = (difference + 312) / 625;
			if(!d_have_clock6) {
				/* working on CLK1-6/UAP discoery */
				d_have_clock6 = d_piconet->UAP_from_header(packet, interval, d_channel);
				if(d_have_clock6) {
					/* got CLK1-6/UAP, start working on CLK1-27 */
					printf("\nCalculating complete hopping sequence.\n");
					printf("%d initial CLK1-27 candidates\n", d_piconet->init_hop_reversal(d_channel));
					/* use previously observed packets to eliminate candidates */
					num_candidates = d_piconet->winnow();
					printf("%d CLK1-27 candidates remaining\n", num_candidates);
				}
			} else {
				/* continue working on CLK1-27 */
				/* we need timing information from an additional packet, so run through UAP_from_header() again */
				d_have_clock6 = d_piconet->UAP_from_header(packet, interval, d_channel);
				//FIXME what if !d_have_clock6?
				num_candidates = d_piconet->winnow();
				printf("%d CLK1-27 candidates remaining\n", num_candidates);
			}
			d_previous_packet_time = current_time;
			/* CLK1-27 results */
			if(num_candidates == 1) {
				/* win! */
				printf("\nAcquired CLK1-27 = 0x%07x\n", d_piconet->get_clock());
				exit(0);
			} else if(num_candidates == 0) {
				/* fail! */
				printf("Failed to acquire clock. starting over . . .\n\n");
				/* start everything over, even CLK1-6/UAP discovery, because we can't trust what we have */
				//FIXME maybe ought to just reset the existing piconet
				d_piconet = bluetooth_make_piconet(d_LAP);
				d_have_clock6 = false;
			}
		}
		consumed += 126;
	}
	d_cumulative_count += consumed;

	// Tell runtime system how many output items we produced.
	return consumed;
}
