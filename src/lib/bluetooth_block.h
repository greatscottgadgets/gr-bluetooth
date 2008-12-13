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
#ifndef INCLUDED_BLUETOOTH_BLOCK_H
#define INCLUDED_BLUETOOTH_BLOCK_H

#include <gr_sync_block.h>
#include <stdint.h>

static const int IN = 1;
static const int OUT = 0;

/* index into whitening data array */
static const uint8_t d_indicies[64] = {99, 85, 17, 50, 102, 58, 108, 45, 92, 62, 32, 118, 88, 11, 80, 2, 37, 69, 55, 8, 20, 40, 74, 114, 15, 106, 30, 78, 53, 72, 28, 26, 68, 7, 39, 113, 105, 77, 71, 25, 84, 49, 57, 44, 61, 117, 10, 1, 123, 124, 22, 125, 111, 23, 42, 126, 6, 112, 76, 24, 48, 43, 116, 0};

/* whitening data */
static const uint8_t d_whitening_data[127] = {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1};

/* LAP reserved for general inquiry */
static const int general_inquiry_LAP = 0x9E8B33;

/*!
 * \brief Bluetooth parent class.
 * \ingroup block
 */
class bluetooth_block : public gr_sync_block
{
protected:

	bluetooth_block ();

	int d_stream_length;
	char *d_stream;
	uint16_t d_consumed;
	// could be 32 bits, but then it would roll over after about 70 minutes
	uint64_t d_cumulative_count;
	bool flag;
	int d_LAP;
	int d_UAP;
	uint8_t d_payload_size;
	int d_packet_type;

	/* Error correction coding for Access Code */
	uint8_t *codeword(uint8_t *data, int length, int k);

	/* Reverse the bits in a byte */
	uint8_t reverse(char byte);

	/* Generate Access Code from an LAP */
	uint8_t *acgen(int LAP);

	/* Convert from normal bytes to one-LSB-per-byte format */
	void convert_to_grformat(uint8_t input, uint8_t *output);

	/* Decode 2/3 rate FEC, a (15,10) shortened Hamming code */
	char *unfec23(char *stream, int length);

	/* Create an Access Code from LAP and check it against stream */
	bool check_ac(char *stream, int LAP);

	/* Extract an LAP from a stream */
	int get_LAP(char *stream);

	/* Print general information about a frame */
	void print_out();

//FIXME more stuff to consider moving here:
/* To deal with whitened data */
//void header();
//int sniff_ac();
/* stream points to the stream of data, length is length in bits */
//char *unfec13(char *stream, uint8_t *output, int length); // UAP and sniffer same, dump different
/* Create the CRC */
//uint16_t crcgen(uint8_t *packet, int length, int UAP); // UAP and dump same, sniffer different
//int payload_header(char *stream);
//int long_payload_header(char *stream);
// and more. . .

};

#endif /* INCLUDED_BLUETOOTH_BLOCK_H */
