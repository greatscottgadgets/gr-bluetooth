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

#include <bluetooth_UAP2.h>

/*
 * Create a new instance of bluetooth_UAP2 and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
bluetooth_UAP2_sptr 
bluetooth_make_UAP2 (int LAP, int pkts)
{
  return bluetooth_UAP2_sptr (new bluetooth_UAP2 (LAP, pkts));
}

//private constructor
bluetooth_UAP2::bluetooth_UAP2 (int LAP, int pkts)
  : bluetooth_block ()
{
	d_LAP = LAP;
	d_stream_length = 0;
	d_consumed = 0;

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
	d_packets_observed = 0;
	d_UAP_confirmation = false;
}

//virtual destructor.
bluetooth_UAP2::~bluetooth_UAP2 ()
{
}

int 
bluetooth_UAP2::work (int noutput_items,
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
			header();
			d_first_packet_time = d_previous_packet_time;
			printf("first packet was at sample %d\n", d_first_packet_time);
		} else {
			header();
			d_previous_packet_time = d_cumulative_count + d_consumed;
		}
		d_consumed += 126;
	}
	d_cumulative_count += d_consumed;

	// Tell runtime system how many output items we produced.
	return d_consumed;
}

void bluetooth_UAP2::print_out()
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
int bluetooth_UAP2::UAP_from_hec(uint8_t *packet)
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
int bluetooth_UAP2::sniff_ac()
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

void bluetooth_UAP2::header()
{
	char *stream = d_stream + d_consumed + 72;
	char header[18];
	char unwhitened[18];
	uint8_t unwhitened_air[3]; // more than one bit per byte but in air order
	uint8_t UAP, ltadr, type;
	int count, group, retval, first_clock;
	int starting = 0;
	int eliminated = 0;
	int ending = 0;

	unfec13(stream, header, 18);

	/* number of samples elapsed since previous packet */
	int difference = (d_cumulative_count + d_consumed) - d_previous_packet_time;

	/* number of time slots elapsed since previous packet */
	int interval = (difference + 312) / 625;
	printf("Packet received after %d time slots.\n", interval + d_previous_clock_offset);

	/* the remainder is an indicator of how far off we have drifted */
	int remainder = difference % 625;
	printf("remainder = %d/625\n", remainder);
	if((remainder > 156) && (remainder < 468))
	{
		printf("terrible wandering\n");
		//exit(0);
	}

	for(count = 0; count < 64; count++)
	{
		if(d_clock_candidates[count] > -1 || d_first_packet_time == 0)
		{

			/* clock value for the current packet assuming count was the clock of the first packet */
			int clock = (count + d_previous_clock_offset + interval) % 64;

			unwhiten(header, unwhitened, clock, 18, 0);
			unwhitened_air[0] = unwhitened[0] << 7 | unwhitened[1] << 6 | unwhitened[2] << 5 | unwhitened[3] << 4 | unwhitened[4] << 3 | unwhitened[5] << 2 | unwhitened[6] << 1 | unwhitened[7];
			unwhitened_air[1] = unwhitened[8] << 1 | unwhitened[9];
			unwhitened_air[2] = unwhitened[10] << 7 | unwhitened[11] << 6 | unwhitened[12] << 5 | unwhitened[13] << 4 | unwhitened[14] << 3 | unwhitened[15] << 2 | unwhitened[16] << 1 | unwhitened[17];

			UAP = UAP_from_hec(unwhitened_air);

			ltadr = (unwhitened_air[0] & 0xe0) >> 5;
			type = (unwhitened_air[0] & 0x1e) >> 1;
			retval = -1;

			if(d_first_packet_time == 0)
			{
				/* this is the first packet: populate the candidate list */
				retval = crc_check(stream+54, type, 3125+d_stream_length-(d_consumed + 126), clock, UAP);
				if(0 == retval)
				{
					d_clock_candidates[count] = -1;
					eliminated++;
				} else
					d_clock_candidates[count] = UAP;
			} else if(UAP != d_clock_candidates[count])
			{
				/* UAP mismatch: eliminate this candidate */
				//printf("new UAP %x doesn't match original UAP %x, eliminating %d\n", UAP, d_clock_candidates[count], count);
				d_clock_candidates[count] = -1;
				eliminated++;
			} else
			{
				retval = crc_check(stream+54, type, 3125+d_stream_length-(d_consumed + 126), clock, UAP);
				if(0 == retval)
				{
					//printf("CRC check failed, eliminating %d\n", count);
					d_clock_candidates[count] = -1;
					eliminated++;
				} else
					/* remember this count because it may be the correct clock of the first packet */
					first_clock = count;
			}
			//printf("tried clock = %d, UAP = 0x%x, ltadr = %d, type = %d, retval = %d\n", clock, UAP, ltadr, type, retval);
			starting++;
		}
	}
	d_previous_clock_offset += interval;
	ending = starting - eliminated;
	printf("reduced from %d to %d\n", starting, ending);
	d_packets_observed++;
	if(0 == ending)
	{
		printf("no candidates remaining! starting over . . .\n");
		d_first_packet_time = 0;
		d_previous_packet_time = 0;
		d_previous_clock_offset = 0;
		d_UAP_confirmation = false;
	} else if(1 == ending)
		/* don't trust this result until an additional packet corroborates */
		if(d_UAP_confirmation)
		{
			//for(count = 0; count < 64; count++)
				//if(d_clock_candidates[count] > -1)
					//UAP = d_clock_candidates[count];
			printf("We have a winner! UAP = 0x%x found after %d packets.\n", UAP, d_packets_observed);
			printf("CLK1-6 at first packet was 0x%x.\n", first_clock);
			//exit(0);
		} else
		{
			printf("awaiting confirmation . . .\n");
			d_UAP_confirmation = true;
		}
}

int bluetooth_UAP2::crc_check(char *stream, int type, int size, int clock, uint8_t UAP)
{
	switch(type)
	{
		case 4:/* FHS packets are sent with a UAP of 0x00 in the HEC */
			if((size >= 240) && (UAP == 0x00))
				return fhs(stream, clock, UAP);
			else
				return 0;

		case 12:/* DM1 */
			if(size >= 8)
				return DM(stream, clock, UAP, 0, size);
			else
				return 1;

		default :/* All without CRCs */
			return 1;
	}
	return 1;
}

int bluetooth_UAP2::fhs(char *stream, int clock, uint8_t UAP)
{
	char *corrected;
	char payload[144];
	uint8_t fhsuap;
	uint32_t fhslap;

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
int bluetooth_UAP2::DM(char *stream, int clock, uint8_t UAP, bool pkthdr, int size)
{
	int count, index, bitlength, length;
	uint16_t crc, check;

	if(pkthdr)
	{
		char *corrected;
		char hdr[16];
		corrected = unfec23(stream, 16);
		if(NULL == corrected)
			return 0;
		unwhiten(corrected, hdr, clock, 16, 18);
		free(corrected);
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

	char *corrected;
	char payload[bitlength];
	corrected = unfec23(stream, bitlength);
	if(NULL == corrected)
		return 0;
	unwhiten(corrected, payload, clock, bitlength, 18);
	free(corrected);

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
int bluetooth_UAP2::DH(char *stream, int clock, uint8_t UAP, bool pkthdr, int size)
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

int bluetooth_UAP2::EV(char *stream, int clock, uint8_t UAP, int type, int size)
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

	char *corrected;
	char payload[(maxlength+2)*8];

	corrected = unfec23(stream, maxlength * 8);
	if(NULL == corrected)
		return 0;
	unwhiten(corrected, payload, clock, maxlength * 8, 18);
	free(corrected);

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
uint16_t bluetooth_UAP2::crcgen(char *packet, int length, int UAP)
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

