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
#include <gr_io_signature.h>

/*
 * Create a new instance of bluetooth_UAP and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
bluetooth_UAP_sptr 
bluetooth_make_UAP (int LAP, int pkts)
{
  return bluetooth_UAP_sptr (new bluetooth_UAP (LAP, pkts));
}


static const int IN = 1;	// number of input streams
static const int OUT = 0;	// size and number of output streams

  /* index into whitening data array */
static const uint8_t d_indicies[64] = {99, 85, 17, 50, 102, 58, 108, 45, 92, 62, 32, 118, 88, 11, 80, 2, 37, 69, 55, 8, 20, 40, 74, 114, 15, 106, 30, 78, 53, 72, 28, 26, 68, 7, 39, 113, 105, 77, 71, 25, 84, 49, 57, 44, 61, 117, 10, 1, 123, 124, 22, 125, 111, 23, 42, 126, 6, 112, 76, 24, 48, 43, 116, 0};
  /* whitening data */
static const uint8_t d_whitening_data[127] = {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1};

//private constructor
bluetooth_UAP::bluetooth_UAP (int LAP, int pkts)
  : gr_sync_block ("UAP",
	      gr_make_io_signature (IN, IN, sizeof (char)),
	      gr_make_io_signature (OUT, OUT, OUT))
{
	d_LAP = LAP;
	d_stream_length = 0;
	d_consumed = 0;
	d_limit = pkts;
	flag = 0;

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
	d_consumed = 0;
	d_stream_length = noutput_items;
	int retval = 0;

	while(d_stream_length) {
		if((noutput_items - d_consumed) > 71)
			retval = sniff_ac();
		else {
			//The flag is used to avoid being stuck with <71 input bits when using a file as input
			if(flag)
				d_consumed = noutput_items;
			flag = !flag;
			break;
		}
	
		if(-1 == retval) {
			d_consumed = noutput_items;
			break;
		}
	
		d_consumed += retval;
	
		if(126 <= noutput_items - d_consumed) {
			header();
			
			if(d_limit == 0)
				print_out();//exit(0);
		} else { //Drop out and wait to be run again
			break;
		}
	
		d_consumed += 126;
		d_stream_length = noutput_items - d_consumed;
	}
  // Tell runtime system how many output items we produced.
  if(d_consumed >= noutput_items)
	return noutput_items;
  else
	return d_consumed;
}

/* Converts 8 bytes of grformat to a single byte */
char bluetooth_UAP::gr_to_normal(char *stream)
{
	return stream[0] << 7 | stream[1] << 6 | stream[2] << 5 | stream[3] << 4 | stream[4] << 3 | stream[5] << 2 | stream[6] << 1 | stream[7];
}

/* stream points to the stream of data, length is length in bits */
char *bluetooth_UAP::unfec13(char *stream, uint8_t *output, int length)
{
	int count, a, b, c;

	for(count = 0; count < length; count++)
	{
		a = 3*count;
		b = a + 1;
		c = a + 2;
		output[count] = ((stream[a] & stream[b]) | (stream[b] & stream[c]) | (stream[c] & stream[a]));
	}
	return stream;
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

//This is all imported from packet_UAP.c
//It has been converted to C++
uint8_t *bluetooth_UAP::codeword(uint8_t *data, int length, int k)
/*
 * Compute redundacy cw[], the coefficients of b(x). The redundancy
 * polynomial b(x) is the remainder after dividing x^(length-k)*data(x)
 * by the generator polynomial g(x).
 */
{
	int    i, j;
	uint8_t *g, *cw, feedback;

	g = (uint8_t *) malloc(35);
	g[0] = 1;
	g[1] = 0;
	g[2] = 0;
	g[3] = 1;
	g[4] = 0;
	g[5] = 1;
	g[6] = 0;
	g[7] = 1;
	g[8] = 1;
	g[9] = 0;
	g[10] = 1;
	g[11] = 1;
	g[12] = 1;
	g[13] = 1;
	g[14] = 0;
	g[15] = 0;
	g[16] = 1;
	g[17] = 0;
	g[18] = 0;
	g[19] = 0;
	g[20] = 1;
	g[21] = 1;
	g[22] = 1;
	g[23] = 0;
	g[24] = 1;
	g[25] = 0;
	g[26] = 1;
	g[27] = 0;
	g[28] = 0;
	g[29] = 0;
	g[30] = 0;
	g[31] = 1;
	g[32] = 1;
	g[33] = 0;
	g[34] = 1;

	cw = (uint8_t *) malloc(34);

	for (i = 0; i < length - k; i++)
		cw[i] = 0;
	for (i = k - 1; i >= 0; i--) {
		feedback = data[i] ^ cw[length - k - 1];
		if (feedback != 0) {
			for (j = length - k - 1; j > 0; j--)
				if (g[j] != 0)
					cw[j] = cw[j - 1] ^ feedback;
				else
					cw[j] = cw[j - 1];
			cw[0] = g[0] && feedback;
		} else {
			for (j = length - k - 1; j > 0; j--)
				cw[j] = cw[j - 1];
			cw[0] = 0;
		}
	}
	free(g);
	return cw;
}

/* Reverse the bits in a byte */
uint8_t bluetooth_UAP::reverse(char byte)
{
	return (byte & 0x80) >> 7 | (byte & 0x40) >> 5 | (byte & 0x20) >> 3 | (byte & 0x10) >> 1 | (byte & 0x08) << 1 | (byte & 0x04) << 3 | (byte & 0x02) << 5 | (byte & 0x01) << 7;
}

/* Endianness - Assume LAP is MSB first, rest done LSB first */
uint8_t *bluetooth_UAP::acgen(int LAP)
{
	uint8_t *retval, *pn, count, *cw, *data;
	retval = (uint8_t *) malloc(9);
	pn = (uint8_t *) malloc(9);
	data = (uint8_t *) malloc(30);

	LAP = reverse((LAP & 0xff0000)>>16) | (reverse((LAP & 0x00ff00)>>8)<<8) | (reverse(LAP & 0x0000ff)<<16);

	retval[4] = (LAP & 0xc00000)>>22;
	retval[5] = (LAP & 0x3fc000)>>14;
	retval[6] = (LAP & 0x003fc0)>>6;
	retval[7] = (LAP & 0x00003f)<<2;

	/* Trailer */
	if(LAP & 0x1)
	{	retval[7] |= 0x03;
		retval[8] = 0x2a;
	} else
		retval[8] = 0xd5;

	pn[0] = 0x03;
	pn[1] = 0xF2;
	pn[2] = 0xA3;
	pn[3] = 0x3D;
	pn[4] = 0xD6;
	pn[5] = 0x9B;
	pn[6] = 0x12;
	pn[7] = 0x1C;
	pn[8] = 0x10;

	for(count = 4; count < 9; count++)
		retval[count] ^= pn[count];

	/* Codeword */
	//g(d) = 0x585713DA9
	data[0] = (retval[4] & 0x02) >> 1;
	data[1] = (retval[4] & 0x01);
	data[2] = (retval[5] & 0x80) >> 7;
	data[3] = (retval[5] & 0x40) >> 6;
	data[4] = (retval[5] & 0x20) >> 5;
	data[5] = (retval[5] & 0x10) >> 4;
	data[6] = (retval[5] & 0x08) >> 3;
	data[7] = (retval[5] & 0x04) >> 2;
	data[8] = (retval[5] & 0x02) >> 1;
	data[9] = (retval[5] & 0x01);
	data[10] = (retval[6] & 0x80) >> 7;
	data[11] = (retval[6] & 0x40) >> 6;
	data[12] = (retval[6] & 0x20) >> 5;
	data[13] = (retval[6] & 0x10) >> 4;
	data[14] = (retval[6] & 0x08) >> 3;
	data[15] = (retval[6] & 0x04) >> 2;
	data[16] = (retval[6] & 0x02) >> 1;
	data[17] = (retval[6] & 0x01);
	data[18] = (retval[7] & 0x80) >> 7;
	data[19] = (retval[7] & 0x40) >> 6;
	data[20] = (retval[7] & 0x20) >> 5;
	data[21] = (retval[7] & 0x10) >> 4;
	data[22] = (retval[7] & 0x08) >> 3;
	data[23] = (retval[7] & 0x04) >> 2;
	data[24] = (retval[7] & 0x02) >> 1;
	data[25] = (retval[7] & 0x01);
	data[26] = (retval[8] & 0x80) >> 7;
	data[27] = (retval[8] & 0x40) >> 6;
	data[28] = (retval[8] & 0x20) >> 5;
	data[29] = (retval[8] & 0x10) >> 4;

	cw = codeword(data, 64, 30);

	retval[0] = cw[0] << 3 | cw[1] << 2 | cw[2] << 1 | cw[3];
	retval[1] = cw[4] << 7 | cw[5] << 6 | cw[6] << 5 | cw[7] << 4 | cw[8] << 3 | cw[9] << 2 | cw[10] << 1 | cw[11];
	retval[2] = cw[12] << 7 | cw[13] << 6 | cw[14] << 5 | cw[15] << 4 | cw[16] << 3 | cw[17] << 2 | cw[18] << 1 | cw[19];
	retval[3] = cw[20] << 7 | cw[21] << 6 | cw[22] << 5 | cw[23] << 4 | cw[24] << 3 | cw[25] << 2 | cw[26] << 1 | cw[27];
	retval[4] = cw[28] << 7 | cw[29] << 6 | cw[30] << 5 | cw[31] << 4 | cw[32] << 3 | cw[33] << 2 | (retval[4] & 0x3);;
	free(cw);

	for(count = 0; count < 9; count++)
		retval[count] ^= pn[count];
	free(pn);

	/* Preamble */
	if(retval[0] & 0x08)
		retval[0] |= 0xa0;
	else
		retval[0] |= 0x50;

	return retval;
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

void bluetooth_UAP::convert_to_grformat(uint8_t input, uint8_t *output)
{
	int count;
	for(count = 0; count < 8; count++)
	{
		output[count] = (input & 0x80) >> 7;
		input <<= 1;
	}
}

/* Create an AC and check it*/
bool bluetooth_UAP::check_ac(char *stream)
{
	int count, aclength;
	uint8_t *ac, *grdata;
	aclength = 72;
	bool retval = 1;

	/* Generate AC */
	ac = acgen(d_LAP);

	/* Check AC */
	/* Convert it to grformat, 1 bit per byte, in the LSB */
	grdata = (uint8_t *) malloc(aclength);

	for(count = 0; count < 9; count++)
		convert_to_grformat(ac[count], &grdata[count*8]);

	for(count = 0; count < aclength; count++)
	{
		if(grdata[count] != stream[count])
			retval = 0;
	}
	if(retval)
		return retval;

	/* Check for inquiry access code */
	/* Generate AC */
	ac = acgen(0x9E8B33);
	
	/* Check AC */
	/* Convert it to grformat, 1 bit per byte, in the LSB */
	for(count = 0; count < 9; count++)
		convert_to_grformat(ac[count], &grdata[count*8]);

	for(count = 0; count < aclength; count++)
	{
		if(grdata[count] != stream[count])
			return 0;
	}
	return 1;
}

/* Looks for an AC in the stream */
int bluetooth_UAP::sniff_ac()
{
	int jump, count, counter, size;
	char *stream = d_stream;
	int jumps[16] = {3,2,1,3,3,0,2,3,3,2,0,3,3,1,2,3};
	size = d_stream_length;
	count = 0;

	while(size > 72)
	{
		jump = jumps[stream[0] << 3 | stream[1] << 2 | stream[2] << 1 | stream[3]];
		if(0 == jump)
		{
			/* Found the start, now check the end... */
			counter = stream[62] << 9 | stream[63] << 8 | stream[64] << 7 | stream[65] << 6 | stream[66] << 5 | stream[67] << 4 | stream[68] << 3 | stream[69] << 2 | stream[70] << 1 | stream[71];

			if((0x0d5 == counter) || (0x32a == counter))
			{
				if(check_ac(stream))
					return count;
			}
			jump = 1;
		}
		count += jump;
		stream += jump;
		size -= jump;
	}
	return -1;
}

void bluetooth_UAP::unwhiten_header(uint8_t *input, uint8_t *output, int clock)
{
	int count, index;
	index = d_indicies[clock & 0x3f];

	for(count = 0; count < 18; count++)
	{
		output[count] = input[count] ^ d_whitening_data[index];
		index += 1;
		index %= 127;
	}
}

void bluetooth_UAP::header()
{
	char *stream = d_stream + d_consumed + 72;
	uint8_t header[18];
	uint8_t unwhitened[18];
	uint8_t UAP, ltadr, type;
	int count, group, retval;

	unfec13(stream, header, 18);

	for(count = 1; count < 64; count++)
	{
		unwhiten_header(header, unwhitened, count);

		unwhitened[0] = unwhitened[0] << 7 | unwhitened[1] << 6 | unwhitened[2] << 5 | unwhitened[3] << 4 | unwhitened[4] << 3 | unwhitened[5] << 2 | unwhitened[6] << 1 | unwhitened[7];
		unwhitened[1] = unwhitened[8] << 1 | unwhitened[9];
		unwhitened[2] = unwhitened[10] << 7 | unwhitened[11] << 6 | unwhitened[12] << 5 | unwhitened[13] << 4 | unwhitened[14] << 3 | unwhitened[15] << 2 | unwhitened[16] << 1 | unwhitened[17];

		UAP = UAP_from_hec(unwhitened);

		/* Make sure we only count it once per packet */
		ltadr = (unwhitened[0] & 0xe0) >> 5;
		type = (unwhitened[0] & 0x1e) >> 1;

		retval = crc_check(stream+54, type, d_stream_length-(d_consumed + 126), count, UAP);

	/* Group is one of control packets, sco connection
	 * esco connection or acl connection.
	 * If a link is being used it is likely to be of
	 * one conenction type, therefore the packets will
	 * more often be in the same group.  The control
	 * packets are added to the highest group */
		switch(type)
		{
			case 0:group = 0; break;
			case 1:group = 1; break;
			case 2:group = 3; break;
			case 3:group = 2; break;
			case 4:group = 0; break;
			case 5:group = 3; break;
			case 6:group = 1; break;
			case 7:group = 3; break;
			case 8:group = 0; break;
			case 9:group = 3; break;
			case 10:group = 1; break;
			case 11:group = 2; break;
			case 12:group = 3; break;
			case 13:group = 3; break;
			/* Can represent two different types */
			case 14:d_UAPs[UAP][ltadr][1] += retval; group = 2; break;
			case 15:group = 3; break;
			default:group = 0;
		}
		d_UAPs[UAP][ltadr][group] += retval;
	}
	d_limit--;
}

int bluetooth_UAP::crc_check(char *stream, int type, int size, int clock, uint8_t UAP)
{
	switch(type)
	{
		case 1:/* DV - skip 80bits for voice then treat like a DM1 */
			stream += 80;
			if(size >= 8)
				return DM(stream, clock, UAP, 0, size);
			else
				return 1;

		case 2:/* DH1 */
			if(size >= 8)
				return DH(stream, clock, UAP, 0, size);
			else
				return 1;

		case 3:/* EV4 Unknown length, need to cycle through it until CRC matches */
			return EV(stream, clock, UAP, type, size);

		case 4:/* FHS packets are sent with a UAP of 0x00 in the HEC */
			if((size >= 240) && (UAP == 0x00))
				return fhs(stream, clock, UAP);
			else
				return 0;

		case 5:/* DM3 */
			if(size >= 20)
				return DM(stream, clock, UAP, 1, size);
			else
				return 1;


		case 7:/* DM5 */
			if(size >= 20)
				return DM(stream, clock, UAP, 1, size);
			else
				return 1;

		case 11:/* EV5 Unknown length, need to cycle through it until CRC matches */
			return EV(stream, clock, UAP, type, size);

		case 12:/* DM1 */
			if(size >= 8)
				return DM(stream, clock, UAP, 0, size);
			else
				return 1;

		case 13:/* DH3 */
			if(size >= 16)
				return DH(stream, clock, UAP, 1, size);
			else
				return 1;

		case 14:/* EV3 Unknown length, need to cycle through it until CRC matches */
			return EV(stream, clock, UAP, type, size);

		case 15:/* DH5 */
			if(size >= 16)
				return DH(stream, clock, UAP, 1, size);
			else
				return 1;

		default :/* All without CRCs */
			return 1;
	}
	return 1;
}

int bluetooth_UAP::fhs(char *stream, int clock, uint8_t UAP)
{
	int count, index, pointer;

	index = d_indicies[clock & 0x3f];
	index += 18; //skip header
	index %= 127;

	uint8_t payload[144];
	pointer = -5;
	for(count = 0; count < 144; count++)
	{
		if(0 == (count % 10))
		{
			pointer += 5;
		}

		payload[count] = stream[pointer] ^ d_whitening_data[index];
		index++;
		index %= 127;
		pointer++;
	}

	uint8_t fhsuap;
	int fhslap;

	fhsuap = payload[64] | payload[65] << 1 | payload[66] << 2 | payload[67] << 3 | payload[68] << 4 | payload[69] << 5 | payload[70] << 6 | payload[71] << 7;

	fhslap = payload[34] | payload[35] << 1 | payload[36] << 2 | payload[37] << 3 | payload[38] << 4 | payload[39] << 5 | payload[40] << 6 | payload[41] << 7 | payload[42] << 8 | payload[43] << 9 | payload[44] << 10 | payload[45] << 11 | payload[46] << 12 | payload[47] << 13 | payload[48] << 14 | payload[49] << 15 | payload[50] << 16 | payload[51] << 17 | payload[52] << 18 | payload[53] << 19 | payload[54] << 20 | payload[55] << 21 | payload[56] << 22 | payload[57] << 23;

	if((fhsuap == UAP) && (fhslap == d_LAP))
		return 1000;
	else
		return 0;
}

/* DM 1/3/5 packet (and DV)*/
int bluetooth_UAP::DM(char *stream, int clock, uint8_t UAP, bool pkthdr, int size)
{
	int count, index, bitlength, length, pointer;
	uint16_t crc, check;

	index = d_indicies[clock & 0x3f];
	index += 18; //skip header
	index %= 127;

	if(pkthdr)
	{
		uint8_t hdr[16];
		pointer = -5;
		for(count = 0; count < 16; count++)
		{
			if(0 == (count % 10))
				pointer += 5;

			hdr[count] = stream[count] ^ d_whitening_data[index];
			index++;
			index %= 127;
		}
		index -= 16;
		length = long_payload_length(hdr) + 4;
		bitlength = length*8;
		if(bitlength > size)
			return 1;
	} else {
		uint8_t hdr[8];
		pointer = 0;
		for(count = 0; count < 8; count++)
		{
			hdr[count] = stream[pointer] ^ d_whitening_data[index];
			index++;
			index %= 127;
			pointer++;
		}
		index -= 8;
		length = payload_length(hdr) + 3;
		bitlength = length*8;
		if(bitlength > size)
			return 1;
	}

	char payload[bitlength];
	pointer = -5;
	for(count = 0; count < bitlength; count++)
	{
		if(0 == (count % 10))
			pointer += 5;

		index %= 127;
		payload[count] = stream[pointer] ^ d_whitening_data[index];
		index++;
		pointer++;
	}

	//Pack the bits into bytes
	for(count = 0; count < length; count++)
	{
		index = 8 * count;
		payload[count] = payload[index] << 7 | payload[index+1] << 6 | payload[index+2] << 5 | payload[index+3] << 4 | payload[index+4] << 3 | payload[index+5] << 2 | payload[index+6] << 1 | payload[index+7];
	}

	crc = crcgen(payload, length-2, UAP);

	check = payload[length-1] | payload[length-2] << 8;

	if(crc == check)
		return 10;

	return 0;
}

/* DH 1/3/5 packet */
int bluetooth_UAP::DH(char *stream, int clock, uint8_t UAP, bool pkthdr, int size)
{
	int count, index, bitlength, length;
	uint16_t crc, check;

	index = d_indicies[clock & 0x3f];
	index += 18; //skip header
	index %= 127;

	if(pkthdr)
	{
		uint8_t hdr[16];
		for(count = 0; count < 16; count++)
		{
			hdr[count] = stream[count] ^ d_whitening_data[index];
			index++;
			index %= 127;
		}
		index -= 16;
		length = long_payload_length(hdr) + 4;
		bitlength = length*8;
		if(bitlength > size)
			return 1;
	} else {
		uint8_t hdr[8];
		for(count = 0; count < 8; count++)
		{
			hdr[count] = stream[count] ^ d_whitening_data[index];
			index++;
			index %= 127;
		}
		index -= 8;
		length = payload_length(hdr) + 3;
		bitlength = length*8;
		if(bitlength > size)
			return 1;
	}

	char payload[bitlength];
	for(count = 0; count < bitlength; count++)
	{
		index %= 127;
		payload[count] = stream[count] ^ d_whitening_data[index];
		index++;
	}

	//Pack the bits into bytes
	for(count = 0; count < length; count++)
	{
		index = 8 * count;
		payload[count] = payload[index] << 7 | payload[index+1] << 6 | payload[index+2] << 5 | payload[index+3] << 4 | payload[index+4] << 3 | payload[index+5] << 2 | payload[index+6] << 1 | payload[index+7];
	}

	crc = crcgen(payload, length-2, UAP);

	check = payload[length-1] | payload[length-2] << 8;

	if(crc == check)
		return 10;

	return 0;
}

int bluetooth_UAP::EV(char *stream, int clock, uint8_t UAP, int type, int size)
{
	int maxlength, count, counter, pointer, index;
	uint16_t crc, check;
	bool fec;

	index = d_indicies[clock & 0x3f];
	index += 18; //skip header
	index %= 127;

	switch (type)
	{
		case 3: maxlength = 120; fec = 1; break;
		case 11: maxlength = 180; fec = 0; break;
		case 14: maxlength = 30; fec = 0; break;
		default: return 0;
	}

	char payload[maxlength];
	char byte[8];

	// pack 1 byte at a time, check crc
	// keep checking through as long as size has 8 more bits
	for(count = 0; count < maxlength; count++)
	{
		pointer = 8*count;
		size -= 8;
		if(size <= 0)
			return 1;

		if((fec) && (0 == pointer))
			pointer = -5;

		for(counter = 0; counter < 8; counter++)
		{/* Pack byte */
			if((fec) && (0 == pointer %10))
				{
					pointer += 5;
					size -= 5;
					if(size <= 0)
						return 1;
				}

			byte[counter] = stream[pointer] ^ d_whitening_data[index];
			pointer++;
			index++;
			index %= 127;
		}
		payload[count] = byte[0] << 7 | byte[1] << 6 | byte[2] << 5 | byte[3] << 4 | byte[4] << 3 | byte[5] << 2 | byte[6] << 1 | byte[7];

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

int bluetooth_UAP::long_payload_length(uint8_t *payload)
{
	return payload[3] | payload[4] << 1 | payload[5] << 2 | payload[6] << 3 | payload[7] << 4 | payload[8] << 5 | payload[9] << 6 | payload[10] << 7 | payload[11] << 8;
}

int bluetooth_UAP::payload_length(uint8_t *payload)
{
	return payload[3] | payload[4] << 1 | payload[5] << 2 | payload[6] << 3 | payload[7] << 4;
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

