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
	d_stream_length = 0;
	d_consumed = 0;

	printf("Bluetooth UAP sniffer\nUsing LAP:0x%06x\n\n", LAP);

	/* ensure that we are always given at least 3125 symbols (5 time slots) */
	set_history(3125);

	d_cumulative_count = 0;
	d_first_packet_time = 0;
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
	d_stream = (char *) input_items[0];
	d_stream_length = noutput_items;
	int retval;

	retval = bluetooth_packet::sniff_ac(d_stream, noutput_items);
	if(-1 == retval) {
		d_consumed = noutput_items;
	} else {
		d_consumed = retval;
		bluetooth_packet_sptr packet = bluetooth_make_packet(&d_stream[retval], 3125 + noutput_items - retval);
		if(packet->get_LAP() == d_LAP) {
			if(d_first_packet_time == 0)
			{
				d_previous_packet_time = d_cumulative_count + d_consumed;
				UAP_from_header(packet);
				d_first_packet_time = d_previous_packet_time;
			} else {
				if (UAP_from_header(packet))
					exit(0);
				d_previous_packet_time = d_cumulative_count + d_consumed;
			}
		}
		d_consumed += 126;
	}
	d_cumulative_count += d_consumed;

	// Tell runtime system how many output items we produced.
	return d_consumed;
}

bool bluetooth_UAP::UAP_from_header(bluetooth_packet_sptr packet)
{
	char *stream = d_stream + d_consumed + 72;
	char header[18];
	char unwhitened[18];
	uint8_t unwhitened_air[3]; // more than one bit per byte but in air order
	uint8_t UAP, type;
	int count, retval, first_clock;
	int crc_match = -1;
	int starting = 0;
	int remaining = 0;

	unfec13(stream, header, 18);

	/* number of samples elapsed since previous packet */
	int difference = (d_cumulative_count + d_consumed) - d_previous_packet_time;

	/* number of time slots elapsed since previous packet */
	int interval = (difference + 312) / 625;
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
		if(d_clock6_candidates[count] > -1 || d_first_packet_time == 0)
		{
			/* clock value for the current packet assuming count was the clock of the first packet */
			int clock = (count + d_previous_clock_offset + interval) % 64;

			unwhiten(header, unwhitened, clock, 18, 0);
			unwhitened_air[0] = unwhitened[0] << 7 | unwhitened[1] << 6 | unwhitened[2] << 5 | unwhitened[3] << 4 | unwhitened[4] << 3 | unwhitened[5] << 2 | unwhitened[6] << 1 | unwhitened[7];
			unwhitened_air[1] = unwhitened[8] << 1 | unwhitened[9];
			unwhitened_air[2] = unwhitened[10] << 7 | unwhitened[11] << 6 | unwhitened[12] << 5 | unwhitened[13] << 4 | unwhitened[14] << 3 | unwhitened[15] << 2 | unwhitened[16] << 1 | unwhitened[17];

			UAP = bluetooth_packet::UAP_from_hec(unwhitened_air);
			type = air_to_host8(&unwhitened[3], 4);
			retval = -1;
			starting++;

			/* if this is the first packet: populate the candidate list */
			/* if not: check CRCs if UAPs match */
			if(d_first_packet_time == 0 || UAP == d_clock6_candidates[count])
				retval = packet->crc_check(type, clock, UAP);
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
	printf("reduced from %d to %d CLK1-6 candidates\n", starting, remaining);
	if(0 == remaining)
	{
		printf("no candidates remaining! starting over . . .\n");
		d_first_packet_time = 0;
		d_previous_packet_time = 0;
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
