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
#ifndef INCLUDED_BLUETOOTH_UAP_H
#define INCLUDED_BLUETOOTH_UAP_H

#include <gr_sync_block.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

class bluetooth_UAP;
typedef boost::shared_ptr<bluetooth_UAP> bluetooth_UAP_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_UAP.
 */
bluetooth_UAP_sptr bluetooth_make_UAP (int LAP, int pkts);

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class bluetooth_UAP : public gr_sync_block
{
private:
  // The friend declaration allows bluetooth_make_UAP to
  // access the private constructor.

  friend bluetooth_UAP_sptr bluetooth_make_UAP (int LAP, int pkts);

  bluetooth_UAP (int LAP, int pkts);	// private constructor

  int d_LAP;
  int d_UAPs[256][8][4];
  int d_stream_length;
  int d_limit;
  char *d_stream;
  uint16_t d_consumed;
  bool flag;

/* Converts 8 bytes of grformat to a single byte */
char gr_to_normal(char *stream);

/* stream points to the stream of data, length is length in bits */
char *unfec13(char *stream, uint8_t *output, int length);

/* Dump the information to the screen */
void print_out();

uint8_t *codeword(uint8_t *data, int length, int k);

uint8_t reverse(char byte);

uint8_t *acgen(int LAP);

int UAP_from_hec(uint8_t *packet);

void convert_to_grformat(uint8_t input, uint8_t *output);

/* Check that the Access Code is correct for the given LAP */
bool check_ac(char *stream);

int sniff_ac();

/* removes the whitening from the header */
void unwhiten_header(uint8_t *input, uint8_t *output, int clock);

/* To deal with whitened data */
void header();

int crc_check(char *stream, int type, int size, int clock, uint8_t UAP);

int fhs(char *stream, int clock, uint8_t UAP);

int DM(char *stream, int clock, uint8_t UAP, bool pkthdr, int size);

int DH(char *stream, int clock, uint8_t UAP, bool pkthdr, int size);

int EV(char *stream, int clock, uint8_t UAP, int type, int size);

int long_payload_length(uint8_t *payload);

int payload_length(uint8_t *payload);

uint16_t crcgen(char *packet, int length, int UAP);


 public:
  ~bluetooth_UAP ();	// public destructor

  // Where all the action really happens

  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_UAP_H */
