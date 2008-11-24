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
#ifndef INCLUDED_BLUETOOTH_dump_H
#define INCLUDED_BLUETOOTH_dump_H

#include <gr_sync_block.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

class bluetooth_dump;
typedef boost::shared_ptr<bluetooth_dump> bluetooth_dump_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_dump.
 */
bluetooth_dump_sptr bluetooth_make_dump ();

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class bluetooth_dump : public gr_sync_block
{
private:
  // The friend declaration allows bluetooth_make_dump to
  // access the private constructor.

  friend bluetooth_dump_sptr bluetooth_make_dump ();

  bluetooth_dump ();  	// private constructor

  int d_LAP;
  int d_UAP;
  uint8_t d_payload_size;
  int d_packet_type;
  int d_stream_length;
  char *d_stream;
  uint16_t d_consumed;
  bool flag;


int payload_header(char *stream);

int long_payload_header(char *stream);

/* Converts 8 bytes of grformat to a single byte */
char gr_to_normal(char *stream);

/* stream points to the stream of data
   length is length in bits of the data
   before it was encoded with fec2/3 */
char *unfec23(char *stream, int length);

/* stream points to the stream of data, length is length in bits */
char *unfec13(char *stream, int length);

/* HV1 packet */
int HV1(char *stream);

/* HV2 packet */
int HV2(char *stream);

/* HV3 packet */
int HV3(char *stream);

/* DV packet */
int DV(char *stream, int UAP, int size);

/* EV3 packet */
int EV3(char *stream, int UAP, int size);

/* EV4 packet */
int EV4(char *stream, int UAP, int size);

/* EV5 packet */
int EV5(char *stream, int UAP, int size);

/* DM1 packet */
int DM1(char *stream, int UAP, int size);

/* DH1 packet */
int DH1(char *stream, int UAP, int size);

/* DM3 packet */
int DM3(char *stream, int UAP, int size);

/* DH3 packet */
int DH3(char *stream, int UAP, int size);

/* AUX1 packet */
int AUX1(char *stream, int size);

/* POLL packet */
int POLL();

/* FHS packet */
int FHS(char *stream, int UAP);

/* NULL packet */
int null_packet();

void print_out();

uint8_t *codeword(uint8_t *data, int length, int k);

uint8_t reverse(char byte);

uint8_t *acgen(int LAP);

int UAP_from_hec(char *packet, uint8_t hec);

void header();

uint16_t crcgen(char *packet, int length, int UAP);

int payload();

void convert_to_grformat(uint8_t input, uint8_t *output);

int check_ac(char *stream);

int sniff_ac();

 public:
  ~bluetooth_dump ();	// public destructor

  // Where all the action really happens

  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_dump_H */
