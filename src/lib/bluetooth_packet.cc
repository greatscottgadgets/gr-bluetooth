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

#include "bluetooth_packet.h"
#include <stdlib.h>

/* initialize constant class member arrays */
/* isn't it silly that this can't be done in the class declaration? */
const uint8_t bluetooth_packet::INDICES[] = {99, 85, 17, 50, 102, 58, 108, 45, 92, 62, 32, 118, 88, 11, 80, 2, 37, 69, 55, 8, 20, 40, 74, 114, 15, 106, 30, 78, 53, 72, 28, 26, 68, 7, 39, 113, 105, 77, 71, 25, 84, 49, 57, 44, 61, 117, 10, 1, 123, 124, 22, 125, 111, 23, 42, 126, 6, 112, 76, 24, 48, 43, 116, 0};

const uint8_t bluetooth_packet::WHITENING_DATA[] = {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1};

const uint8_t bluetooth_packet::PREAMBLE_DISTANCE[] = {2,2,1,2,2,1,2,2,1,2,0,1,2,2,1,2,2,1,2,2,1,0,2,1,2,2,1,2,2,1,2,2};

const uint8_t bluetooth_packet::TRAILER_DISTANCE[] = {
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,3,3,2,3,2,2,1,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,3,3,2,3,2,2,1,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,3,3,2,3,2,2,1,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,3,2,2,1,2,1,1,0,4,3,3,2,3,2,2,1,4,3,3,2,3,2,2,1,5,4,4,3,4,3,3,2,
	5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,3,3,2,3,2,2,1,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,3,3,2,3,2,2,1,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,0,1,1,2,1,2,2,3,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,3,3,2,3,2,2,1,5,4,4,3,4,3,3,2,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,
	3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,1,2,2,3,2,3,3,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,
	5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,5,4,4,3,4,3,3,2,5,5,5,4,5,4,4,3,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,
	4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5,3,4,4,5,4,5,5,5,2,3,3,4,3,4,4,5,5,5,5,4,5,4,4,3,4,5,5,5,5,5,5,4,4,5,5,5,5,5,5,4,3,4,4,5,4,5,5,5
};

/*
 * Create a new instance of bluetooth_packet and return
 * a boost shared_ptr.  This is effectively the public constructor.
 */
bluetooth_packet_sptr
bluetooth_make_packet(char *stream, int length)
{
       return bluetooth_packet_sptr (new bluetooth_packet (stream, length));
}

/* constructor */
bluetooth_packet::bluetooth_packet(char *stream, int length)
{
	int i;

	if(length > MAX_SYMBOLS)
		length = MAX_SYMBOLS;
	for(i = 0; i < length; i++)
		d_symbols[i] = stream[i];

	//FIXME maybe should verify LAP
	d_LAP = air_to_host32(&d_symbols[38], 24);
	d_length = length;
}

/* search a symbol stream to find a packet, return index */
int bluetooth_packet::sniff_ac(char *stream, int stream_length)
{
	/* Looks for an AC in the stream */
	int count;
	uint8_t preamble; // start of sync word (includes LSB of LAP)
	uint16_t trailer; // end of sync word: barker sequence and trailer (includes MSB of LAP)
	int max_distance = 2; // maximum number of bit errors to tolerate in preamble + trailer
	uint32_t LAP;
	char *symbols;

	for(count = 0; count < stream_length; count ++)
	{
		symbols = &stream[count];
		preamble = air_to_host8(&symbols[0], 5);
		trailer = air_to_host16(&symbols[61], 11);
		if((PREAMBLE_DISTANCE[preamble] + TRAILER_DISTANCE[trailer]) <= max_distance)
		{
			LAP = air_to_host32(&symbols[38], 24);
			if(check_ac(symbols, LAP))
			{
				return count;
			}
		}
	}
	return -1;
}

/* destructor */
bluetooth_packet::~bluetooth_packet()
{
}

/* A linear feedback shift register */
uint8_t *bluetooth_packet::lfsr(uint8_t *data, int length, int k, uint8_t *g)
/*
 * A linear feedback shift register
 * used for the syncword in the access code
 * and the fec2/3 encoding (could also be used for the HEC/CRC)
 * Although I'm not sure how to run it backwards for HEC->UAP
 */
{
	int    i, j;
	uint8_t *cw, feedback;
	cw = (uint8_t *) calloc(length - k, 1);

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
	return cw;
}

/* Reverse the bits in a byte */
uint8_t bluetooth_packet::reverse(char byte)
{
	return (byte & 0x80) >> 7 | (byte & 0x40) >> 5 | (byte & 0x20) >> 3 | (byte & 0x10) >> 1 | (byte & 0x08) << 1 | (byte & 0x04) << 3 | (byte & 0x02) << 5 | (byte & 0x01) << 7;
}

/* Generate Access Code from an LAP */
uint8_t *bluetooth_packet::acgen(int LAP)
{
	/* Endianness - Assume LAP is MSB first, rest done LSB first */
	uint8_t *retval, count, *cw, *data;
	retval = (uint8_t *) calloc(9,1);
	data = (uint8_t *) malloc(30);
	// pseudo-random sequence to XOR with LAP and syncword
	uint8_t pn[] = {0x03,0xF2,0xA3,0x3D,0xD6,0x9B,0x12,0x1C,0x10};
	// generator polynomial for the access code
	uint8_t g[] = {1,0,0,1,0,1,0,1,1,0,1,1,1,1,0,0,1,0,0,0,1,1,1,0,1,0,1,0,0,0,0,1,1,0,1};

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

	for(count = 4; count < 9; count++)
		retval[count] ^= pn[count];

	data[0] = (retval[4] & 0x02) >> 1;
	data[1] = (retval[4] & 0x01);
	host_to_air(reverse(retval[5]), (char *) data+2, 8);
	host_to_air(reverse(retval[6]), (char *) data+10, 8);
	host_to_air(reverse(retval[7]), (char *) data+18, 8);
	host_to_air(reverse(retval[8]), (char *) data+26, 4);

	cw = lfsr(data, 64, 30, g);
	free(data);

	retval[0] = cw[0] << 3 | cw[1] << 2 | cw[2] << 1 | cw[3];
	retval[1] = cw[4] << 7 | cw[5] << 6 | cw[6] << 5 | cw[7] << 4 | cw[8] << 3 | cw[9] << 2 | cw[10] << 1 | cw[11];
	retval[2] = cw[12] << 7 | cw[13] << 6 | cw[14] << 5 | cw[15] << 4 | cw[16] << 3 | cw[17] << 2 | cw[18] << 1 | cw[19];
	retval[3] = cw[20] << 7 | cw[21] << 6 | cw[22] << 5 | cw[23] << 4 | cw[24] << 3 | cw[25] << 2 | cw[26] << 1 | cw[27];
	retval[4] = cw[28] << 7 | cw[29] << 6 | cw[30] << 5 | cw[31] << 4 | cw[32] << 3 | cw[33] << 2 | (retval[4] & 0x3);
	free(cw);

	for(count = 0; count < 9; count++)
		retval[count] ^= pn[count];

	/* Preamble */
	if(retval[0] & 0x08)
		retval[0] |= 0xa0;
	else
		retval[0] |= 0x50;

	return retval;
}

/* Convert from normal bytes to one-LSB-per-byte format */
void bluetooth_packet::convert_to_grformat(uint8_t input, uint8_t *output)
{
	int count;
	for(count = 0; count < 8; count++)
	{
		output[count] = (input & 0x80) >> 7;
		input <<= 1;
	}
}

/* Decode 1/3 rate FEC, three like symbols in a row */
char *bluetooth_packet::unfec13(char *stream, char *output, int length)
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

/* Decode 2/3 rate FEC, a (15,10) shortened Hamming code */
char *bluetooth_packet::unfec23(char *input, int length)
{
	/* input points to the input data
	 * length is length in bits of the data
	 * before it was encoded with fec2/3 */
	char *output;
	int iptr, optr, blocks;
	uint8_t difference, count, *codeword;
	uint8_t fecgen[] = {1,1,0,1,0,1};

	iptr = -15;
	optr = -10;
	difference = length % 10;
	// padding at end of data
	if(0!=difference)
		length += (10 - difference);

	blocks = length/10;
	output = (char *) malloc(length);

	while(blocks) {
		iptr += 15;
		optr += 10;
		blocks--;

		// copy data to output
		for(count=0;count<10;count++)
			output[optr+count] = input[iptr+count];

		// call fec23gen on data to generate the codeword
		//codeword = fec23gen(input+iptr);
		codeword = lfsr((uint8_t *) input+iptr, 15, 10, fecgen);

		// compare codeword to the 5 received bits
		difference = 0;
		for(count=0;count<5;count++)
			if(codeword[count]!=input[iptr+10+count])
				difference++;

		/* no errors or single bit errors (errors in the parity bit):
		 * (a strong hint it's a real packet) */
		if((0==difference) || (1==difference)) {
		    free(codeword);
		    continue;
		}

		// multiple different bits in the codeword
		for(count=0;count<5;count++) {
			difference |= codeword[count] ^ input[iptr+10+count];
			difference <<= 1;
		}
		free(codeword);

		switch (difference) {
		/* comments are the bit that's wrong and the value
		 * of difference in binary, from the BT spec */
			// 1000000000 11010
			case 26: output[optr] ^= 1; break;
			// 0100000000 01101
			case 13: output[optr+1] ^= 1; break;
			// 0010000000 11100
			case 28: output[optr+2] ^= 1; break;
			// 0001000000 01110
			case 14: output[optr+3] ^= 1; break;
			// 0000100000 00111
			case 7: output[optr+4] ^= 1; break;
			// 0000010000 11001
			case 25: output[optr+5] ^= 1; break;
			// 0000001000 10110
			case 22: output[optr+6] ^= 1; break;
			// 0000000100 01011
			case 11: output[optr+7] ^= 1; break;
			// 0000000010 11111
			case 31: output[optr+8] ^= 1; break;
			// 0000000001 10101
			case 21: output[optr+9] ^= 1; break;
			/* not one of these errors, probably multiple bit errors
			 * or maybe not a real packet, safe to drop it? */
			default: free(output); return NULL;
		}
	}
	return output;
}

/* Create an Access Code from LAP and check it against stream */
bool bluetooth_packet::check_ac(char *stream, int LAP)
{
	int count, aclength, biterrors;
	uint8_t *ac, *grdata;
	aclength = 72;
	biterrors = 0;

	/* Generate AC */
	ac = acgen(LAP);

	/* Check AC */
	/* Convert it to grformat, 1 bit per byte, in the LSB */
	grdata = (uint8_t *) malloc(aclength);

	for(count = 0; count < 9; count++)
		convert_to_grformat(ac[count], &grdata[count*8]);
	free(ac);

	for(count = 0; count < aclength; count++)
	{
		if(grdata[count] != stream[count])
			biterrors++;
			//FIXME do error correction instead of detection
		if(biterrors>=10)
		{
			free(grdata);
			return false;
		}
	}
	if(biterrors)
	{
		printf("POSSIBLE PACKET, LAP = %06x with %d errors\n", LAP, biterrors);
		free(grdata);
		//return false;
		return true;
	}

	free(grdata);
	return true;
}

/* Convert some number of bits of an air order array to a host order integer */
uint8_t bluetooth_packet::air_to_host8(char *air_order, int bits)
{
	int i;
	uint8_t host_order = 0;
	for (i = 0; i < bits; i++)
		host_order |= (air_order[i] << i);
	return host_order;
}
uint16_t bluetooth_packet::air_to_host16(char *air_order, int bits)
{
	int i;
	uint16_t host_order = 0;
	for (i = 0; i < bits; i++)
		host_order |= (air_order[i] << i);
	return host_order;
}
uint32_t bluetooth_packet::air_to_host32(char *air_order, int bits)
{
	int i;
	uint32_t host_order = 0;
	for (i = 0; i < bits; i++)
		host_order |= (air_order[i] << i);
	return host_order;
}

/* Convert some number of bits in a host order integer to an air order array */
void bluetooth_packet::host_to_air(uint8_t host_order, char *air_order, int bits)
{
    int i;
    for (i = 0; i < bits; i++)
        air_order[i] = (host_order >> i) & 0x01;
}

/* Remove the whitening from an air order array */
void bluetooth_packet::unwhiten(char* input, char* output, int clock, int length, int skip)
{
	int count, index;
	index = INDICES[clock & 0x3f];
	index += skip;
	index %= 127;

	for(count = 0; count < length; count++)
	{
		output[count] = input[count] ^ WHITENING_DATA[index];
		index += 1;
		index %= 127;
	}
}

/* Pointer to start of packet, length of packet in bits, UAP */
uint16_t bluetooth_packet::crcgen(char *payload, int length, int UAP)
{
	char byte;
	uint16_t reg, count;

	reg = (reverse(UAP) << 8) & 0xff00;
	for(count = 0; count < length; count++)
	{
		byte = payload[count];

		reg = (reg >> 1) | (((reg & 0x0001) ^ (byte & 0x01))<<15);

		/*Bit 5*/
		reg ^= ((reg & 0x8000)>>5);

		/*Bit 12*/
		reg ^= ((reg & 0x8000)>>12);
	}
	return reg;
}

/* return the packet's LAP */
uint32_t bluetooth_packet::get_LAP()
{
	return d_LAP;
}

/* return the packet's UAP */
uint8_t bluetooth_packet::get_UAP()
{
	return d_UAP;
}

/* set the packet's UAP */
void bluetooth_packet::set_UAP(uint8_t UAP)
{
	d_UAP = UAP;
}

/* extract UAP by reversing the HEC computation */
int bluetooth_packet::UAP_from_hec(uint8_t *packet)
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

/* check if the packet's CRC is correct for a given clock */
int bluetooth_packet::crc_check(int type, int clock, uint8_t UAP)
{
	/* return value of 1 represents inconclusive result (default) */
	int retval = 1;
	/* number of bytes in the payload header */
	int header_bytes = 2;
	/* skip the packet header */
	char *stream = d_symbols + 54;

	switch(type)
	{
		case 2:/* FHS packets are sent with a UAP of 0x00 in the HEC */
			retval = fhs(stream, clock, UAP, d_length);
			break;

		case 8:/* DV - skip 80bits for voice then treat like a DM1 */
			stream += 80;
			//FIXME d_length does not reflect += 80
		case 3:/* DM1 */
			header_bytes = 1;
		case 10:/* DM3 */
		case 14:/* DM5 */
			retval = DM(stream, clock, UAP, header_bytes, d_length);
			break;

		case 4:/* DH1 */
			header_bytes = 1;
		case 11:/* DH3 */
		case 15:/* DH5 */
			retval = DH(stream, clock, UAP, header_bytes, d_length);
			break;

		case 7:/* EV3 */
		case 12:/* EV4 */
		case 13:/* EV5 */
			/* Unknown length, need to cycle through it until CRC matches */
			retval = EV(stream, clock, UAP, type, d_length);
			break;
	}
	/* never return a zero result unless this ia a FHS or DM1 */
	/* any other type could have actually been something else */
	if(retval == 0 && (type < 2 || type > 3))
		return 1;
	return retval;
}

int bluetooth_packet::fhs(char *stream, int clock, uint8_t UAP, int size)
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
int bluetooth_packet::DM(char *stream, int clock, uint8_t UAP, int header_bytes, int size)
{
	int bitlength, length;
	uint16_t crc, check;
	char *corrected;

	if(header_bytes == 2)
	{
		if(size < 30)
			return 1; //FIXME should throw exception
		corrected = unfec23(stream, 16);
		if(NULL == corrected)
			return 0;
		unwhiten(corrected, d_payload_header, clock, 16, 18);
		free(corrected);
		/* payload length is payload body length + 2 bytes payload header + 2 bytes CRC */
		length = air_to_host16(&d_payload_header[3], 10) + 4;
		/* check that the length is within range allowed by specification */
		if(length > 228) //FIXME should be 125 if DM3
			return 0;
	} else {
		if(size < 8)
			return 1; //FIXME should throw exception
		corrected = unfec23(stream, 8);
		if(NULL == corrected)
			return 0;
		unwhiten(corrected, d_payload_header, clock, 8, 18);
		free(corrected);
		/* payload length is payload body length + 1 byte payload header + 2 bytes CRC */
		length = air_to_host8(&d_payload_header[3], 5) + 3;
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

	if(crc == check) {
		d_payload_crc = crc;
		return 10;
	}

	return 0;
}

/* DH 1/3/5 packet */
/* similar to DM 1/3/5 but without FEC */
int bluetooth_packet::DH(char *stream, int clock, uint8_t UAP, int header_bytes, int size)
{
	int bitlength, length;
	uint16_t crc, check;

	if(size < (header_bytes * 8))
		return 1; //FIXME should throw exception

	if(header_bytes == 2)
	{
		unwhiten(stream, d_payload_header, clock, 16, 18);
		/* payload length is payload body length + 2 bytes payload header + 2 bytes CRC */
		length = air_to_host16(&d_payload_header[3], 10) + 4;
		/* check that the length is within range allowed by specification */
		if(length > 343) //FIXME should be 187 if DH3
			return 0;
	} else {
		unwhiten(stream, d_payload_header, clock, 8, 18);
		/* payload length is payload body length + 1 byte payload header + 2 bytes CRC */
		length = air_to_host8(&d_payload_header[3], 5) + 3;
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

	if(crc == check) {
		d_payload_crc = crc;
		return 10;
	}

	return 0;
}

int bluetooth_packet::EV(char *stream, int clock, uint8_t UAP, int type, int size)
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
		if(crc == check) {
			d_payload_crc = crc;
			return 10;
		}
	}
	return 0;
}

/* decode the packet header */
void bluetooth_packet::decode_header()
{
	char *stream = d_symbols + 72;
	char header[18];
	char unwhitened[18];
	uint8_t unwhitened_air[3]; // more than one bit per byte but in air order
	uint8_t UAP, ltadr;
	int count, size;

	size = d_length - 126;

	unfec13(stream, header, 18);

	for(count = 0; count < 64; count++)
	{
		unwhiten(header, unwhitened, count, 18, 0);

		unwhitened_air[0] = unwhitened[0] << 7 | unwhitened[1] << 6 | unwhitened[2] << 5 | unwhitened[3] << 4 | unwhitened[4] << 3 | unwhitened[5] << 2 | unwhitened[6] << 1 | unwhitened[7];
		unwhitened_air[1] = unwhitened[8] << 1 | unwhitened[9];
		unwhitened_air[2] = unwhitened[10] << 7 | unwhitened[11] << 6 | unwhitened[12] << 5 | unwhitened[13] << 4 | unwhitened[14] << 3 | unwhitened[15] << 2 | unwhitened[16] << 1 | unwhitened[17];

		UAP = bluetooth_packet::UAP_from_hec(unwhitened_air);

		//FIXME should verify we have d_UAP
		if(UAP != d_UAP)
			continue;

		uint8_t ltadrs[8] = {0, 4, 2, 6, 1, 5, 3, 7};
		ltadr = ltadrs[(unwhitened_air[0] & 0xe0) >> 5];

		//FIXME assuming that the lt_addr can only be 1
		if(1 != ltadr)
			continue;

		d_packet_type = air_to_host8(&unwhitened[3], 4);
		//FIXME this probably ought to be in a separate decode_payload():
		switch(d_packet_type)
		{
			case 0: /* NULL */
				/* no payload to decode */
				break;
			case 1: /* POLL */
				/* no payload to decode */
				break;
			case 2: /* FHS */
				fhs(stream, count, UAP, size);
				break;
			case 3: /* DM1 */
				DM(stream, count, UAP, 1, size);
				break;
			case 4: /* DH1 */
				/* assuming DH1 but could be 2-DH1 */
				DH(stream, count, UAP, 1, size);
				break;
			case 5: /* HV1 */
				/* don't know how to decode */
				break;
			case 6: /* HV2 */
				/* assuming HV2 but could be 2-EV3 */
				/* don't know how to decode */
				break;
			case 7: /* EV3 */
				/* assuming EV3 but could be HV3 or 3-EV3 */
				EV(stream, count, UAP, d_packet_type, size);
				break;
			case 8: /* DV */
				/* assuming DV but could be 3-DH1 */
				/* skip 80bits for voice then treat like a DM1 */
				DM(stream + 80, count, UAP, 1, size);
				break;
			case 9: /* AUX1 */
				/* don't know how to decode */
				break;
			case 10: /* DM3 */
				/* assuming DM3 but could be 2-DH3 */
				DM(stream, count, UAP, 2, size);
				break;
			case 11: /* DH3 */
				/* assuming DH3 but could be 3-DH3 */
				DH(stream, count, UAP, 2, size);
				break;
			case 12: /* EV4 */
				/* assuming EV4 but could be 2-EV5 */
				EV(stream, count, UAP, d_packet_type, size);
				break;
			case 13: /* EV5 */
				/* assuming EV5 but could be 3-EV5 */
				EV(stream, count, UAP, d_packet_type, size);
			case 14: /* DM5 */
				/* assuming DM5 but could be 2-DH5 */
				DM(stream, count, UAP, 2, size);
				break;
			case 15: /* DH5 */
				/* assuming DH5 but could be 3-DH5 */
				DH(stream, count, UAP, 2, size);
				break;
		}
		//FIXME move to separate method that prints more info
		printf("packet type = %d\n", d_packet_type);
	}
}
