/* -*- c++ -*- */
/*
 * Copyright 2008, 2009 Dominic Spill, Michael Ossmann                                                                                            
 * Copyright 2007 Dominic Spill                                                                                                                   
 * Copyright 2005, 2006 Free Software Foundation, Inc.
 * 
 * This file is part of gr-bluetooth
 * 
 * gr-bluetooth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * gr-bluetooth is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with gr-bluetooth; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_BLUETOOTH_PACKET_H
#define INCLUDED_BLUETOOTH_PACKET_H

#include <stdint.h>
#include <string>
#include <boost/enable_shared_from_this.hpp>

using namespace std;

class bluetooth_packet;
typedef boost::shared_ptr<bluetooth_packet> bluetooth_packet_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_packet.
 */
bluetooth_packet_sptr bluetooth_make_packet(char *stream, int length);

class bluetooth_packet
{
private:
	/* allow bluetooth_make_packet to access the private constructor. */
	friend bluetooth_packet_sptr bluetooth_make_packet(char *stream, int length);

	/* constructor */
	bluetooth_packet(char *stream, int length);

	/* maximum number of symbols */
	static const int MAX_SYMBOLS = 3125;

	/* index into whitening data array */
	static const uint8_t INDICES[64];

	/* whitening data */
	static const uint8_t WHITENING_DATA[127];

	/* lookup table for preamble hamming distance */
	static const uint8_t PREAMBLE_DISTANCE[32];

	/* lookup table for trailer hamming distance */
	static const uint8_t TRAILER_DISTANCE[2048];

	/* string representations of packet type */
	static const string TYPE_NAMES[16];

	/* the raw symbol stream, one bit per char */
	//FIXME maybe this should be a vector so we can grow it only to the size
	//needed and later shrink it if we find we have more symbols than necessary
	char d_symbols[MAX_SYMBOLS];

	/* lower address part found in access code */
	uint32_t d_LAP;

	/* upper address part */
	uint8_t d_UAP;

	/* number of symbols */
	int d_length;

	/* packet type */
	int d_packet_type;

	/* packet header, one bit per char */
	char d_packet_header[18];

	/* payload header, one bit per char */
	/* the packet may have a payload header of 0, 1, or 2 bytes, reserving 2 */
	char d_payload_header[16];

	/* number of payload header bytes */
	/* set to 0, 1, 2, or -1 for unknown */
	int d_payload_header_length;

	/* LLID field of payload header (2 bits) */
	uint8_t d_payload_llid;

	/* flow field of payload header (1 bit) */
	uint8_t d_payload_flow;

	/* payload length: the total length of the asynchronous data in bytes.
	 * This does not include the length of synchronous data, such as the voice
	 * field of a DV packet.
	 * If there is a payload header, this payload length is payload body length
	 * (the length indicated in the payload header's length field) plus
	 * d_payload_header_length plus 2 bytes CRC (if present).
	 */
	int d_payload_length;

	/* The actual payload data in host format
	 * Ready for passing to wireshark
	 * 2744 is the maximum length, but most packets are shorter.
	 * Dynamic allocation would probably be better in the long run but is
	 * problematic in the short run.
	 */
	char d_payload[2744];

	/* CRC of packet payload */
	uint16_t d_payload_crc;

	/* is the packet whitened? */
	bool d_whitened;

	/* do we know the UAP? */
	bool d_have_UAP;

	/* do we know the clock (master CLK1-27)? */
	bool d_have_clock;

	bool d_have_payload;
	/* CLK1-27 of master */
	uint32_t d_clock;

	uint8_t d_lower_clock;

	/* type-specific CRC checks */
	//FIXME probably ought to use d_symbols, d_length
	int fhs(char *stream, int clock, uint8_t UAP, int size);
	int DM(char *stream, int clock, uint8_t UAP, int type, int size);
	int DH(char *stream, int clock, uint8_t UAP, int type, int size);
	int EV(char *stream, int clock, uint8_t UAP, int type, int size);
	int HV(char *stream, int clock, uint8_t UAP, int type, int size);

	/* decode payload header, return value indicates success */
	bool decode_payload_header(char *stream, int clock, int header_bytes, int size, bool fec);

	/* Remove the whitening from an air order array */
	void unwhiten(char* input, char* output, int clock, int length, int skip);

public:
	/* search a symbol stream to find a packet, return index */
	static int sniff_ac(char *stream, int stream_length);

	/* Error correction coding for Access Code */
	static uint8_t *lfsr(uint8_t *data, int length, int k, uint8_t *g);

	/* Reverse the bits in a byte */
	static uint8_t reverse(char byte);

	/* Generate Access Code from an LAP */
	static uint8_t *acgen(int LAP);

	/* Convert from normal bytes to one-LSB-per-byte format */
	static void convert_to_grformat(uint8_t input, uint8_t *output);

	/* Decode 1/3 rate FEC, three like symbols in a row */
	static char *unfec13(char *stream, char *output, int length);

	/* Decode 2/3 rate FEC, a (15,10) shortened Hamming code */
	static char *unfec23(char *input, int length);

	/* When passed 10 bits of data this returns a pointer to a 5 bit hamming code */
	static char *fec23gen(char *data);

	/* Create an Access Code from LAP and check it against stream */
	static bool check_ac(char *stream, int LAP);

	/* Convert some number of bits of an air order array to a host order integer */
	static uint8_t air_to_host8(char *air_order, int bits);
	static uint16_t air_to_host16(char *air_order, int bits);
	static uint32_t air_to_host32(char *air_order, int bits);
	// hmmm, maybe these should have pointer output so they can be overloaded

	/* Convert some number of bits in a host order integer to an air order array */
	static void host_to_air(uint8_t host_order, char *air_order, int bits);

	/* Create the 16bit CRC for packet payloads - input air order stream */
	static uint16_t crcgen(char *payload, int length, int UAP);

	/* extract UAP by reversing the HEC computation */
	static int UAP_from_hec(uint8_t *packet);

	/* check if the packet's CRC is correct for a given clock (CLK1-6) */
	int crc_check(int clock);

	/* decode the packet header */
	void decode_header();

	/* decode the packet header */
	void decode_payload();

	/* print packet information */
	void print();

	/* format payload for tun interface */
	char *tun_format();

	/* return the packet's LAP */
	uint32_t get_LAP();

	/* return the packet's UAP */
	uint8_t get_UAP();

	/* set the packet's UAP */
	void set_UAP(uint8_t UAP);

	/* is the packet whitened? */
	bool get_whitened();

	/* set the packet's whitened flag */
	void set_whitened(bool whitened);

	/* have we decoded the payload yet? */
	bool got_payload();

	/* return the packet's clock (CLK1-27) */
	uint32_t get_clock();

	/* set the packet's clock (CLK1-27) */
	void set_clock(uint32_t clock);

	/* Retrieve the length of the payload data */
	int get_payload_length();

	int get_type();

	/* try a clock value (CLK1-6) to unwhiten packet header,
	 * sets resultant d_packet_type and d_UAP, returns UAP.
	 */
	//FIXME should this be merged with set_clock()?
	uint8_t try_clock(int clock);

	/* destructor */
	~bluetooth_packet();
};

#endif /* INCLUDED_BLUETOOTH_PACKET_H */
