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
}

//virtual destructor.
bluetooth_hopper::~bluetooth_hopper ()
{
}

int bluetooth_hopper::work (int noutput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{
	d_stream = (char *) input_items[0];
	d_stream_length = noutput_items;
	int retval, i;

	retval = sniff_ac();
	if(-1 == retval) {
		d_consumed = noutput_items;
	} else {
		d_consumed = retval;
		if(d_first_packet_time == 0) {
			/* this is our first packet to consider for CLK1-6/UAP discovery */
			d_previous_packet_time = d_cumulative_count + d_consumed;
			d_have_clock6 = UAP_from_header();
			d_first_packet_time = d_previous_packet_time;
		} else if(!d_have_clock6) {
			/* still working on CLK1-6/UAP discoery */
			d_have_clock6 = UAP_from_header();
			d_previous_packet_time = d_cumulative_count + d_consumed;
			if(d_have_clock6) {
				/* got CLK1-6/UAP, start working on CLK1-27 */
				printf("\nCalculating complete hopping sequence.\n");
				d_piconet = bluetooth_make_piconet(d_LAP, d_UAP, d_clock6, d_channel);
				//FIXME need d_num_candidates accessor
				//printf("%d initial CLK1-27 candidates\n", d_num_candidates);
				/* use previously observed packets to eliminate candidates */
				for(i = 1; i < d_packets_observed; i++) {
					d_num_candidates = d_piconet->winnow(d_pattern_indices[i], d_channel);
					printf("%d CLK1-27 candidates remaining\n", d_num_candidates);
				}
			}
		} else {
			/* continue working on CLK1-27 */
			/* we need timing information from an additional packet, so run through UAP_from_header() again */
			d_have_clock6 = UAP_from_header();
			d_num_candidates = d_piconet->winnow(d_pattern_indices[d_packets_observed-1], d_channel);
			printf("%d CLK1-27 candidates remaining\n", d_num_candidates);
		}
		/* CLK1-27 results */
		if(d_num_candidates == 1) {
			/* win! */
			//FIXME need clock accessor
			printf("\nAcquired CLK1-27\n");
			//printf("\nAcquired CLK1-27 = 0x%07x\n", d_clock_candidates[0]);
			exit(0);
		} else if(d_num_candidates == 0) {
			/* fail! */
			printf("Failed to acquire clock. starting over . . .\n\n");
			/* start everything over, even CLK1-6/UAP discovery, because we can't trust what we have */
			d_first_packet_time = 0;
			d_previous_packet_time = 0;
			d_previous_clock_offset = 0;
			d_have_clock6 = false;
			d_num_candidates = sequence_length;
			d_packets_observed = 0;
		}
		d_consumed += 126;
	}
	d_cumulative_count += d_consumed;

	// Tell runtime system how many output items we produced.
	return d_consumed;
}
