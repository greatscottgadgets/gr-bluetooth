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
	d_consumed = 0;

	printf("Bluetooth UAP sniffer\nUsing LAP:0x%06x\n\n", LAP);

	/* ensure that we are always given at least 3125 symbols (5 time slots) */
	set_history(3125);

	d_cumulative_count = 0;
	d_got_first_packet = false;
	d_previous_packet_time = 0;
	d_previous_clock_offset = 0;
	d_packets_observed = 0;
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
	int retval, difference, interval, current_time;

	retval = bluetooth_packet::sniff_ac(in, noutput_items);
	if(-1 == retval) {
		d_consumed = noutput_items;
	} else {
		d_consumed = retval;
		bluetooth_packet_sptr packet = bluetooth_make_packet(&in[retval], 3125 + noutput_items - retval);
		if(packet->get_LAP() == d_LAP) {
			current_time = d_cumulative_count + d_consumed;
			/* number of samples elapsed since previous packet */
			difference = current_time - d_previous_packet_time;
			/* number of time slots elapsed since previous packet */
			interval = (difference + 312) / 625;
			if (UAP_from_header(packet, interval))
				exit(0);
			d_previous_packet_time = current_time;
		}
		d_consumed += 126;
	}
	d_cumulative_count += d_consumed;

	// Tell runtime system how many output items we produced.
	return d_consumed;
}

//FIXME move to bluetooth_piconet
bool bluetooth_UAP::UAP_from_header(bluetooth_packet_sptr packet, int interval)
{
	uint8_t UAP;
	int count, retval, first_clock;
	int crc_match = -1;
	int starting = 0;
	int remaining = 0;

	if(!d_got_first_packet)
		interval = 0;
	if(d_packets_observed < max_pattern_length)
		d_pattern_indices[d_packets_observed] = interval + d_previous_clock_offset;
	else
	{
		printf("Oops. More hops than we can remember.\n");
		return false; //FIXME ought to throw exception
	}
	d_packets_observed++;
	d_total_packets_observed++;

	/* try every possible first packet clock value */
	for(count = 0; count < 64; count++)
	{
		/* skip eliminated candidates unless this is our first time through */
		if(d_clock6_candidates[count] > -1 || !d_got_first_packet)
		{
			/* clock value for the current packet assuming count was the clock of the first packet */
			int clock = (count + d_previous_clock_offset + interval) % 64;
			starting++;
			UAP = packet->try_clock(clock);
			retval = -1;

			/* if this is the first packet: populate the candidate list */
			/* if not: check CRCs if UAPs match */
			if(!d_got_first_packet || UAP == d_clock6_candidates[count])
				retval = packet->crc_check(clock);
			switch(retval)
			{
				case -1: /* UAP mismatch */
				case 0: /* CRC failure */
					d_clock6_candidates[count] = -1;
					break;

				case 1: /* inconclusive result */
					d_clock6_candidates[count] = UAP;
					/* remember this count because it may be the correct clock of the first packet */
					first_clock = count;
					remaining++;
					break;

				default: /* CRC success */
					/* It is very likely that this is the correct clock/UAP, but I have seen a false positive */
					printf("Correct CRC! UAP = 0x%x Awaiting confirmation. . .\n", UAP);
					d_clock6_candidates[count] = UAP;
					first_clock = count;
					crc_match = count;
					break;
			}
		}
	}
	if(crc_match > -1)
	{
		/* set things up for one additional packet to confirm */
		remaining = 1;
		/* eliminate all other candidates */
		for(count = 0; count < 64; count++)
			if(count != crc_match)
					d_clock6_candidates[count] = -1;
	}
	d_previous_clock_offset += interval;
	d_got_first_packet = true;
	printf("reduced from %d to %d CLK1-6 candidates\n", starting, remaining);
	if(0 == remaining)
	{
		printf("no candidates remaining! starting over . . .\n");
		d_got_first_packet = false;
		d_previous_clock_offset = 0;
		d_packets_observed = 0;
	} else if(1 == starting && 1 == remaining)
	{
		/* we only trust this result if two packets in a row agree on the winner */
		printf("We have a winner! UAP = 0x%x found after %d total packets.\n", UAP, d_total_packets_observed);
		d_clock6 = first_clock;
		d_UAP = UAP;
		return true;
	}
	return false;
}
