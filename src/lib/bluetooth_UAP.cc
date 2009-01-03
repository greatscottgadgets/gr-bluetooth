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

#include <bluetooth_UAP.h>

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

	retval = sniff_ac();
	if(-1 == retval) {
		d_consumed = noutput_items;
	} else {
		d_consumed = retval;
		if(d_first_packet_time == 0)
		{
			d_previous_packet_time = d_cumulative_count + d_consumed;
			UAP_from_header();
			d_first_packet_time = d_previous_packet_time;
		} else {
			if (UAP_from_header())
				exit(0);
			d_previous_packet_time = d_cumulative_count + d_consumed;
		}
		d_consumed += 126;
	}
	d_cumulative_count += d_consumed;

	// Tell runtime system how many output items we produced.
	return d_consumed;
}

/* Pointer to start of header, UAP */
int bluetooth_UAP::UAP_from_hec(uint8_t *packet)
{
	char byte;
	int count;
	uint8_t hec;

	hec = *(packet + 2);
	byte = *(packet + 1);

	for(count = 0; count < 10; count++)
	{
		if(2==count)
			byte = *packet;

		/*Bit 1*/
		hec ^= ((hec & 0x01)<<1);
		/*Bit 2*/
		hec ^= ((hec & 0x01)<<2);
		/*Bit 5*/
		hec ^= ((hec & 0x01)<<5);
		/*Bit 7*/
		hec ^= ((hec & 0x01)<<7);

		hec = (hec >> 1) | (((hec & 0x01) ^ (byte & 0x01)) << 7);
		byte >>= 1;
	}
	return hec;
}

/* Looks for an AC in the stream */
int bluetooth_UAP::sniff_ac()
{
	int count;
	uint8_t preamble; // start of sync word (includes LSB of LAP)
	uint16_t trailer; // end of sync word: barker sequence and trailer (includes MSB of LAP)
	int max_distance = 2; // maximum number of bit errors to tolerate in preamble + trailer
	char *stream;

	for(count = 0; count < d_stream_length; count ++)
	{
		stream = &d_stream[count];
		preamble = air_to_host8(&stream[0], 5);
		trailer = air_to_host16(&stream[61], 11);
		if((preamble_distance[preamble] + trailer_distance[trailer]) <= max_distance)
		{
			if(check_ac(stream, d_LAP) || check_ac(stream, general_inquiry_LAP))
				return count;
		}
	}
	return -1;
}

bool bluetooth_UAP::UAP_from_header()
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

			UAP = UAP_from_hec(unwhitened_air);
			type = air_to_host8(&unwhitened[3], 4);
			retval = -1;
			starting++;

			/* if this is the first packet: populate the candidate list */
			/* if not: check CRCs if UAPs match */
			if(d_first_packet_time == 0 || UAP == d_clock6_candidates[count])
				retval = crc_check(stream+54, type, 3125+d_stream_length-(d_consumed + 126), clock, UAP);
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

int bluetooth_UAP::crc_check(char *stream, int type, int size, int clock, uint8_t UAP)
{
	/* return value of 1 represents inconclusive result (default) */
	int retval = 1;
	/* number of bytes in the payload header */
	int header_bytes = 2;

	switch(type)
	{
		case 2:/* FHS packets are sent with a UAP of 0x00 in the HEC */
			retval = fhs(stream, clock, UAP, size);
			break;

		case 8:/* DV - skip 80bits for voice then treat like a DM1 */
			stream += 80;
		case 3:/* DM1 */
			header_bytes = 1;
		case 10:/* DM3 */
		case 14:/* DM5 */
			retval = DM(stream, clock, UAP, header_bytes, size);
			break;

		case 4:/* DH1 */
			header_bytes = 1;
		case 11:/* DH3 */
		case 15:/* DH5 */
			retval = DH(stream, clock, UAP, header_bytes, size);
			break;

		case 7:/* EV3 */
		case 12:/* EV4 */
		case 13:/* EV5 */
			/* Unknown length, need to cycle through it until CRC matches */
			retval = EV(stream, clock, UAP, type, size);
			break;
	}
	/* never return a zero result unless this ia a FHS or DM1 */
	/* any other type could have actually been something else */
	if(retval == 0 && (type < 2 || type > 3))
		return 1;
	return retval;
}

int bluetooth_UAP::fhs(char *stream, int clock, uint8_t UAP, int size)
{
	char *corrected;
	char payload[144];
	uint8_t fhsuap;
	uint32_t fhslap;

	/* FHS packets are sent with a UAP of 0x00 in the HEC */
	if(UAP != 0)
		return 0;

	if(size < 225)
		return 1; //FIXME should throw exception

	corrected = unfec23(stream, 144);
	if(NULL == corrected)
		return 0;
	unwhiten(corrected, payload, clock, 144, 18);
	free(corrected);

	fhsuap = air_to_host8(&payload[64], 8);

	fhslap = air_to_host32(&payload[34], 24);

	if((fhsuap == UAP) && (fhslap == d_LAP))
		return 1000;
	else
		return 0;
}

/* DM 1/3/5 packet (and DV)*/
int bluetooth_UAP::DM(char *stream, int clock, uint8_t UAP, int header_bytes, int size)
{
	int bitlength, length;
	uint16_t crc, check;
	char *corrected;

	if(header_bytes == 2)
	{
		char hdr[16];
		if(size < 30)
			return 1; //FIXME should throw exception
		corrected = unfec23(stream, 16);
		if(NULL == corrected)
			return 0;
		unwhiten(corrected, hdr, clock, 16, 18);
		free(corrected);
		/* payload length is payload body length + 2 bytes payload header + 2 bytes CRC */
		length = air_to_host16(&hdr[3], 10) + 4;
		/* check that the length is within range allowed by specification */
		if(length > 228) //FIXME should be 125 if DM3
			return 0;
	} else {
		char hdr[8];
		if(size < 8)
			return 1; //FIXME should throw exception
		corrected = unfec23(stream, 8);
		if(NULL == corrected)
			return 0;
		unwhiten(corrected, hdr, clock, 8, 18);
		free(corrected);
		/* payload length is payload body length + 1 byte payload header + 2 bytes CRC */
		length = air_to_host8(&hdr[3], 5) + 3;
		/* check that the length is within range allowed by specification */
		if(length > 20)
			return 0;
	}
	bitlength = length*8;
	if(bitlength > size)
		return 1; //FIXME should throw exception

	char payload[bitlength];
	corrected = unfec23(stream, bitlength);
	if(NULL == corrected)
		return 0;
	unwhiten(corrected, payload, clock, bitlength, 18);
	free(corrected);
	crc = crcgen(payload, (length-2)*8, UAP);
	check = air_to_host16(&payload[(length-2)*8], 16);

	if(crc == check)
		return 10;

	return 0;
}

/* DH 1/3/5 packet */
/* similar to DM 1/3/5 but without FEC */
int bluetooth_UAP::DH(char *stream, int clock, uint8_t UAP, int header_bytes, int size)
{
	int bitlength, length;
	uint16_t crc, check;

	if(size < (header_bytes * 8))
		return 1; //FIXME should throw exception

	if(header_bytes == 2)
	{
		char hdr[16];
		unwhiten(stream, hdr, clock, 16, 18);
		/* payload length is payload body length + 2 bytes payload header + 2 bytes CRC */
		length = air_to_host16(&hdr[3], 10) + 4;
		/* check that the length is within range allowed by specification */
		if(length > 343) //FIXME should be 187 if DH3
			return 0;
	} else {
		char hdr[8];
		unwhiten(stream, hdr, clock, 8, 18);
		/* payload length is payload body length + 1 byte payload header + 2 bytes CRC */
		length = air_to_host8(&hdr[3], 5) + 3;
		/* check that the length is within range allowed by specification */
		if(length > 30)
			return 0;
	}
	bitlength = length*8;
	if(bitlength > size)
		return 1; //FIXME should throw exception

	char payload[bitlength];
	unwhiten(stream, payload, clock, bitlength, 18);
	crc = crcgen(payload, (length-2)*8, UAP);
	check = air_to_host16(&payload[(length-2)*8], 16);

	if(crc == check)
		return 10;

	return 0;
}

int bluetooth_UAP::EV(char *stream, int clock, uint8_t UAP, int type, int size)
{
	int count;
	uint16_t crc, check;
	char *corrected;
	/* EV5 has a maximum of 180 bytes + 2 bytes CRC */
	int maxlength = 182;
	char payload[maxlength*8];

	switch (type)
	{
		case 12:/* EV4 */
			if(size < 1470)
				return 1; //FIXME should throw exception
			/* 120 bytes + 2 bytes CRC */
			maxlength = 122;
			corrected = unfec23(stream, maxlength * 8);
			if(NULL == corrected)
				return 0;
			unwhiten(corrected, payload, clock, maxlength * 8, 18);
			free(corrected);
			break;

		case 7:/* EV3 */
			/* 30 bytes + 2 bytes CRC */
			maxlength = 32;
		case 13:/* EV5 */
			if(size < (maxlength * 8))
				return 1; //FIXME should throw exception
			unwhiten(stream, payload, clock, maxlength * 8, 18);
			break;

		default:
			return 0;
	}

	/* Check crc for any integer byte length up to maxlength */
	for(count = 1; count < (maxlength-1); count++)
	{
		crc = crcgen(payload, count*8, UAP);
		check = air_to_host16(&payload[count*8], 16);

		/* Check CRC */
		if(crc == check)
			return 10;
	}
	return 0;
}
