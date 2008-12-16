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
bluetooth_make_UAP (int LAP, int pkts)
{
  return bluetooth_UAP_sptr (new bluetooth_UAP (LAP, pkts));
}

//private constructor
bluetooth_UAP::bluetooth_UAP (int LAP, int pkts)
  : bluetooth_block ()
{
	d_LAP = LAP;
	d_stream_length = 0;
	d_consumed = 0;
	d_limit = pkts;

	int count, counter;
	for(count = 0; count < 256; count++)
	{
		for(counter = 0; counter < 8; counter++)
		{
			d_UAPs[count][counter][0] = 0;
			d_UAPs[count][counter][1] = 0;
			d_UAPs[count][counter][2] = 0;
			d_UAPs[count][counter][3] = 0;
		}
	}
	printf("Bluetooth UAP sniffer\nUsing LAP:0x%06x and %d packets\n\n", LAP, pkts);

	/* ensure that we are always given at least 3125 symbols (5 time slots) */
	set_history(3125);

	d_cumulative_count = 0;
	d_first_packet_time = 0;
	d_previous_packet_time = 0;
	d_previous_clock_offset = 0;
	for(count = 0; count < 64; count++)
		d_clock_candidates[count] = 1;
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
			d_first_packet_time = d_cumulative_count + d_consumed;
			printf("first packet at %d\n", d_first_packet_time);
		}
		header();
		d_previous_packet_time = d_cumulative_count + d_consumed;
		if(d_limit == 0)
			exit(0);
			//print_out();
		d_consumed += 126;
	}
	d_cumulative_count += d_consumed;

	// Tell runtime system how many output items we produced.
	return d_consumed;
}

void bluetooth_UAP::print_out()
{
	int count, counter, max, localmax;
	int nibbles[16];
	max = 0;
	printf("Possible UAPs within 20%% of max value\n");
	for(count = 0; count < 16; count++)
		nibbles[count] = 0;

	for(count = 0; count < 256; count++)
	{
		for(counter = 1; counter < 8; counter++)
		{
			localmax = 0;

			if(d_UAPs[count][counter][1] > localmax)
				localmax = d_UAPs[count][counter][1];
			if(d_UAPs[count][counter][2] > localmax)
				localmax = d_UAPs[count][counter][2];
			if(d_UAPs[count][counter][3] > localmax)
				localmax = d_UAPs[count][counter][3];

			d_UAPs[count][counter][0] += localmax;
		}

		for(counter = 1; counter < 8; counter++)
		{
			if(d_UAPs[count][counter][0] > d_UAPs[count][0][0])
				{d_UAPs[count][0][0] = d_UAPs[count][counter][0]; d_UAPs[count][0][1] = counter;}
		}

		if(d_UAPs[count][0][0] > max)
			max = d_UAPs[count][0][0];
	}
	printf("max value=%d\n\n", max);
	counter = 4;
	for(count = 0; count < 256; count++)
	{
		if(max - d_UAPs[count][0][0] <= max/5)
		{
			d_UAPs[count][0][1] = (d_UAPs[count][0][1] & 4) >> 2 | (d_UAPs[count][0][1] & 2) | (d_UAPs[count][0][1] & 1) << 2;
			printf("%02x -> %d votes -> LT_ADDR %d\n", count, d_UAPs[count][0][0], d_UAPs[count][0][1]);
		}
	}
	exit(0);
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
	int jump, count;
	uint16_t trailer; // barker code plus trailer
	char *stream;
	int jumps[16] = {3,2,1,3,3,0,2,3,3,2,0,3,3,1,2,3};

	for(count = 0; count < d_stream_length; count += jump)
	{
		stream = &d_stream[count];
		jump = jumps[stream[0] << 3 | stream[1] << 2 | stream[2] << 1 | stream[3]];
		if(0 == jump)
		{
			/* Found the start, now check the end... */
			trailer = air_to_host16(&stream[61], 11);
			/* stream[4] should probably be used in the jump trick instead of here.
			 * Then again, an even better solution would have some error tolerance,
			 * but we would probably have to abandon the jump trick. */
			if((stream[4] == stream[0]) && ((0x558 == trailer) || (0x2a7 == trailer)))
			{
				if(check_ac(stream, d_LAP) || check_ac(stream, general_inquiry_LAP))
					return count;
			}
			jump = 1;
		}
	}
	return -1;
}

void bluetooth_UAP::header()
{
	char *stream = d_stream + d_consumed + 72;
	char header[18];
	char unwhitened[18];
	uint8_t unwhitened_air[3]; // more than one bit per byte but in air order
	uint8_t UAP, ltadr, type;
	int count, group, retval;
	int starting = 0;
	int eliminated = 0;
	int ending = 0;

	unfec13(stream, header, 18);

	for(count = 0; count < 64; count++)
	{
		int difference = (d_cumulative_count + d_consumed) - d_previous_packet_time;
		int quotient = difference / 625;
		int remainder = difference % 625;
		printf("remainder = %d/625\n", remainder);
		if(remainder > 312)
			quotient++;
		if(d_clock_candidates[count] > 0) {
			int clock = (count + d_previous_clock_offset + quotient) % 64;

			unwhiten(header, unwhitened, clock, 18, 0);
			unwhitened_air[0] = unwhitened[0] << 7 | unwhitened[1] << 6 | unwhitened[2] << 5 | unwhitened[3] << 4 | unwhitened[4] << 3 | unwhitened[5] << 2 | unwhitened[6] << 1 | unwhitened[7];
			unwhitened_air[1] = unwhitened[8] << 1 | unwhitened[9];
			unwhitened_air[2] = unwhitened[10] << 7 | unwhitened[11] << 6 | unwhitened[12] << 5 | unwhitened[13] << 4 | unwhitened[14] << 3 | unwhitened[15] << 2 | unwhitened[16] << 1 | unwhitened[17];

			UAP = UAP_from_hec(unwhitened_air);

			/* Make sure we only count it once per packet */
			ltadr = (unwhitened_air[0] & 0xe0) >> 5;
			type = (unwhitened_air[0] & 0x1e) >> 1;
			//printf("trying clock = %d, UAP = %d, ltadr = %d, type = %d\n", count, UAP, ltadr, type);

			retval = crc_check(stream+54, type, 3125+d_stream_length-(d_consumed + 126), clock, UAP);
			//printf("trying clock = %d, UAP = %d, ltadr = %d, type = %d, retval = %d\n", clock, UAP, ltadr, type, retval);
			if(0==retval)
			{
				d_clock_candidates[count] = 0;
				eliminated++;
			}
			starting++;
		} else
			printf("skipped %d\n", d_clock_candidates[count]);
		d_previous_clock_offset += quotient;
	}
	ending = starting - eliminated;
	printf("reduced from %d to %d\n", starting, ending);
	d_limit--;
}

int bluetooth_UAP::crc_check(char *stream, int type, int size, int clock, uint8_t UAP)
{
	switch(type)
	{
		case 12:/* DM1 */
			if(size >= 8)
				return DM(stream, clock, UAP, 0, size);
			else
				{
				printf("size\n");
				return 1;
				}

		default :/* All without CRCs */
			{
			printf("default\n");
			return 1;
			}
	}
	return 1;
}

int bluetooth_UAP::fhs(char *stream, int clock, uint8_t UAP)
{
	char corrected[144];
	char payload[144];
	uint8_t fhsuap;
	uint32_t fhslap;

	unfec23(stream, corrected, 144);
	unwhiten(corrected, payload, clock, 144, 18);

	fhsuap = air_to_host8(&payload[64], 8);

	fhslap = air_to_host32(&payload[34], 24);

	if((fhsuap == UAP) && (fhslap == d_LAP))
		return 1000;
	else
		return 0;
}

/* DM 1/3/5 packet (and DV)*/
int bluetooth_UAP::DM(char *stream, int clock, uint8_t UAP, bool pkthdr, int size)
{
	int count, index, bitlength, length;
	uint16_t crc, check;

	if(pkthdr)
	{
		char corrected[16];
		char hdr[16];
		unfec23(stream, corrected, 16);
		unwhiten(corrected, hdr, clock, 16, 18);
		length = air_to_host16(&hdr[3], 10) + 4;
	} else {
		char hdr[8];
		//unfec23 not needed because we are only looking at the first 8 symbols
		unwhiten(stream, hdr, clock, 8, 18);
		length = air_to_host8(&hdr[3], 5) + 3;
	}
	bitlength = length*8;
	if(bitlength > size)
		return 1;

	char corrected[bitlength];
	char payload[bitlength];
	unfec23(stream, corrected, bitlength);
	unwhiten(corrected, payload, clock, bitlength, 18);

	//Pack the bits into bytes
	for(count = 0; count < length; count++)
	{
		index = 8 * count;
		payload[count] = payload[index] << 7 | payload[index+1] << 6 | payload[index+2] << 5 | payload[index+3] << 4 | payload[index+4] << 3 | payload[index+5] << 2 | payload[index+6] << 1 | payload[index+7];
		//FIXME payload now breaks the host/air rules
	}

	crc = crcgen(payload, length-2, UAP);

	check = payload[length-1] | payload[length-2] << 8;

	if(crc == check)
		return 10;

	return 0;
}

/* DH 1/3/5 packet */
/* similar to DM 1/3/5 but without FEC */
int bluetooth_UAP::DH(char *stream, int clock, uint8_t UAP, bool pkthdr, int size)
{
	int count, index, bitlength, length;
	uint16_t crc, check;

	if(pkthdr)
	{
		char hdr[16];
		unwhiten(stream, hdr, clock, 16, 18);
		length = air_to_host16(&hdr[3], 10) + 4;
	} else {
		char hdr[8];
		unwhiten(stream, hdr, clock, 8, 18);
		length = air_to_host8(&hdr[3], 5) + 3;
	}
	bitlength = length*8;
	if(bitlength > size)
		return 1;

	char payload[bitlength];
	unwhiten(stream, payload, clock, bitlength, 18);

	//Pack the bits into bytes
	for(count = 0; count < length; count++)
	{
		index = 8 * count;
		payload[count] = payload[index] << 7 | payload[index+1] << 6 | payload[index+2] << 5 | payload[index+3] << 4 | payload[index+4] << 3 | payload[index+5] << 2 | payload[index+6] << 1 | payload[index+7];
		//FIXME payload now breaks the host/air rules
	}

	crc = crcgen(payload, length-2, UAP);

	check = payload[length-1] | payload[length-2] << 8;

	if(crc == check)
		return 10;

	return 0;
}

int bluetooth_UAP::EV(char *stream, int clock, uint8_t UAP, int type, int size)
{
	int index, maxlength, count;
	uint16_t crc, check;
	bool fec;

	switch (type)
	{
		case 3: maxlength = 120; fec = 1; break;
		case 11: maxlength = 180; fec = 0; break;
		case 14: maxlength = 30; fec = 0; break;
		default: return 0;
	}

	char corrected[(maxlength+2)*8];
	char payload[(maxlength+2)*8];

	unfec23(stream, corrected, maxlength * 8);
	unwhiten(corrected, payload, clock, maxlength * 8, 18);

	/* Check crc for any integer byte length up to maxlength */
	for(count = 0; count < maxlength+2; count++)
	{
		index = 8 * count;
		payload[count] = payload[index] << 7 | payload[index+1] << 6 | payload[index+2] << 5 | payload[index+3] << 4 | payload[index+4] << 3 | payload[index+5] << 2 | payload[index+6] << 1 | payload[index+7];
		//FIXME payload now breaks the host/air rules
		if(count > 1)
		{
			crc = crcgen(payload, count-1, UAP);
	
			check = payload[count] | payload[count-1] << 8;
	
			/* Check CRC */
			if(crc == check)
				return 10;
		}
	}
	return 0;
}

/* Pointer to start of packet, length of packet in bytes, UAP */
uint16_t bluetooth_UAP::crcgen(char *packet, int length, int UAP)
{
	char byte;
	uint16_t reg, count, counter;

	reg = UAP & 0xff;
	for(count = 0; count < length; count++)
	{
		byte = *packet++;
		for(counter = 0; counter < 8; counter++)
		{
			reg = (reg << 1) | (((reg & 0x8000)>>15) ^ ((byte & 0x80) >> 7));
			byte <<= 1;

			/*Bit 5*/
			reg ^= ((reg & 0x0001)<<5);

			/*Bit 12*/
			reg ^= ((reg & 0x0001)<<12);
		}
	}
	return reg;
}

