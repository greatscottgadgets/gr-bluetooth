/* -*- c -*- */
/*
 * Copyright 2007 - 2010 Dominic Spill, Michael Ossmann                                                                                            
 * Copyright 2005, 2006 Free Software Foundation, Inc.
 * 
 * This file is part of libbtbb
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with libbtbb; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_BLUETOOTH_PACKET_H
#define INCLUDED_BLUETOOTH_PACKET_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* maximum number of symbols */
#define MAX_SYMBOLS 3125

/* minimum header bit errors to indicate that this is an ID packet */
static const int ID_THRESHOLD = 5;

/* index into whitening data array */
static const uint8_t INDICES[] = {99, 85, 17, 50, 102, 58, 108, 45, 92, 62, 32, 118, 88, 11, 80, 2, 37, 69, 55, 8, 20, 40, 74, 114, 15, 106, 30, 78, 53, 72, 28, 26, 68, 7, 39, 113, 105, 77, 71, 25, 84, 49, 57, 44, 61, 117, 10, 1, 123, 124, 22, 125, 111, 23, 42, 126, 6, 112, 76, 24, 48, 43, 116, 0};

/* whitening data */
static const uint8_t WHITENING_DATA[] = {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1};

/* lookup table for preamble hamming distance */
static const uint8_t PREAMBLE_DISTANCE[] = {2,2,1,2,2,1,2,2,1,2,0,1,2,2,1,2,2,1,2,2,1,0,2,1,2,2,1,2,2,1,2,2};

/* lookup table for trailer hamming distance */
static const uint8_t TRAILER_DISTANCE[] = {3,3,3,2,3,2,2,1,2,3,3,3,3,3,3,2,2,3,3,3,3,3,3,2,1,2,2,3,2,3,3,3,3,2,2,1,2,1,1,0,3,3,3,2,3,2,2,1,3,3,3,2,3,2,2,1,2,3,3,3,3,3,3,2,2,3,3,3,3,3,3,2,1,2,2,3,2,3,3,3,1,2,2,3,2,3,3,3,0,1,1,2,1,2,2,3,3,3,3,2,3,2,2,1,2,3,3,3,3,3,3,2,2,3,3,3,3,3,3,2,1,2,2,3,2,3,3,3};

/* string representations of packet type */
static const char *TYPE_NAMES[] = {
	"NULL", "POLL", "FHS", "DM1", "DH1/2-DH1", "HV1", "HV2/2-EV3", "HV3/EV3/3-EV3",
	"DV/3-DH1", "AUX1", "DM3/2-DH3", "DH3/3-DH3", "EV4/2-EV5", "EV5/3-EV5", "DM5/2-DH5", "DH5/3-DH5"
};

typedef struct packet {
	/* the raw symbol stream, one bit per char */
	//FIXME maybe this should be a vector so we can grow it only to the size
	//needed and later shrink it if we find we have more symbols than necessary
	char symbols[MAX_SYMBOLS];

	/* Bluetooth channel */
	uint8_t channel;

	/* lower address part found in access code */
	uint32_t LAP;
	
	/* upper address part */
	uint8_t UAP;
	
	/* non-significant address part */
	uint8_t NAP;
	
	/* number of symbols */
	int length;
	
	/* packet type */
	int packet_type;
	
	/* packet header, one bit per char */
	char packet_header[18];
	
	/* payload header, one bit per char */
	/* the packet may have a payload header of 0, 1, or 2 bytes, reserving 2 */
	char payload_header[16];
	
	/* number of payload header bytes */
	/* set to 0, 1, 2, or -1 for unknown */
	int payload_header_length;
	
	/* LLID field of payload header (2 bits) */
	uint8_t payload_llid;
	
	/* flow field of payload header (1 bit) */
	uint8_t payload_flow;
	
	/* payload length: the total length of the asynchronous data in bytes.
	* This does not include the length of synchronous data, such as the voice
	* field of a DV packet.
	* If there is a payload header, this payload length is payload body length
	* (the length indicated in the payload header's length field) plus
	* payload_header_length plus 2 bytes CRC (if present).
	*/
	int payload_length;
	
	/* The actual payload data in host format
	* Ready for passing to wireshark
	* 2744 is the maximum length, but most packets are shorter.
	* Dynamic allocation would probably be better in the long run but is
	* problematic in the short run.
	*/
	char payload[2744];
	
	/* is the packet whitened? */
	int whitened;
	
	/* do we know the UAP/NAP? */
	int have_UAP;
	int have_NAP;
	
	/* do we know the master clock? */
	int have_clk6;
	int have_clk27;
	
	int have_payload;
	
	/* CLK1-27 of master */
	uint32_t clock;

	/* native (local) clock */
	uint32_t clkn;
} packet;

/* type-specific CRC checks and decoding */
int fhs(int clock, packet* p);
int DM(int clock, packet* p);
int DH(int clock, packet* p);
int EV3(int clock, packet* p);
int EV4(int clock, packet* p);
int EV5(int clock, packet* p);
int HV(int clock, packet* p);

/* decode payload header, return value indicates success */
int decode_payload_header(char *stream, int clock, int header_bytes, int size, int fec, packet* p);

/* Remove the whitening from an air order array */
void unwhiten(char* input, char* output, int clock, int length, int skip, packet* p);

/* verify the payload CRC */
int payload_crc(packet* p);

/* search a symbol stream to find a packet, return index */
int sniff_ac(char *stream, int stream_length);

/* Error correction coding for Access Code */
uint8_t *lfsr(uint8_t *data, int length, int k, uint8_t *g);

/* Reverse the bits in a byte */
uint8_t reverse(char byte);

/* Generate Access Code from an LAP */
uint8_t *acgen(int LAP);

/* Convert from normal bytes to one-LSB-per-byte format */
void convert_to_grformat(uint8_t input, uint8_t *output);

/* Decode 1/3 rate FEC, three like symbols in a row */
int unfec13(char *input, char *output, int length);

/* Decode 2/3 rate FEC, a (15,10) shortened Hamming code */
char *unfec23(char *input, int length);

/* When passed 10 bits of data this returns a pointer to a 5 bit hamming code */
//char *fec23gen(char *data);

/* Create an Access Code from LAP and check it against stream */
int check_ac(char *stream, int LAP);

/* Convert some number of bits of an air order array to a host order integer */
uint8_t air_to_host8(char *air_order, int bits);
uint16_t air_to_host16(char *air_order, int bits);
uint32_t air_to_host32(char *air_order, int bits);
// hmmm, maybe these should have pointer output so they can be overloaded

/* Convert some number of bits in a host order integer to an air order array */
void host_to_air(uint8_t host_order, char *air_order, int bits);

/* Create the 16bit CRC for packet payloads - input air order stream */
uint16_t crcgen(char *payload, int length, int UAP);

/* extract UAP by reversing the HEC computation */
int UAP_from_hec(uint16_t data, uint8_t hec);

/* check if the packet's CRC is correct for a given clock (CLK1-6) */
int crc_check(int clock, packet* p);

/* decode the packet header */
int decode_header(packet* p);

/* decode the packet header */
void decode_payload(packet* p);

/* decode the whole packet */
void decode(packet* p);

/* print packet information */
void print(packet* p);

/* format payload for tun interface */
char *tun_format(packet* p);

/* try a clock value (CLK1-6) to unwhiten packet header,
 * sets resultant d_packet_type and d_UAP, returns UAP.
 */
uint8_t try_clock(int clock, packet* p);

/* check to see if the packet has a header */
int header_present(packet* p);

/* extract LAP from FHS payload */
uint32_t lap_from_fhs(packet* p);

/* extract UAP from FHS payload */
uint8_t uap_from_fhs(packet* p);

/* extract NAP from FHS payload */
uint16_t nap_from_fhs(packet* p);

/* extract clock from FHS payload */
uint32_t clock_from_fhs(packet* p);

void init_packet(packet *p, char *syms, int len);

#endif /* INCLUDED_BLUETOOTH_PACKET_H */
