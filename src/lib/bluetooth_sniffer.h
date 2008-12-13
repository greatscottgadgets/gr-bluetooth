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
#ifndef INCLUDED_BLUETOOTH_SNIFFER_H
#define INCLUDED_BLUETOOTH_SNIFFER_H

#include <bluetooth_block.h>
#include <stdio.h>
#include <stdlib.h>

class bluetooth_sniffer;
typedef boost::shared_ptr<bluetooth_sniffer> bluetooth_sniffer_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_sniffer.
 */
bluetooth_sniffer_sptr bluetooth_make_sniffer (int lap, int uap);

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class bluetooth_sniffer : public bluetooth_block
{
private:
  // The friend declaration allows bluetooth_make_sniffer to
  // access the private constructor.

  friend bluetooth_sniffer_sptr bluetooth_make_sniffer (int lap, int uap);

  bluetooth_sniffer (int lap, int uap);  	// private constructor

int payload_header(uint8_t *stream);

int long_payload_header(uint8_t *stream);

/* Converts 8 bytes of grformat to a single byte */
char gr_to_normal(char *stream);

char gr_to_normal(uint8_t *stream);

/* Dump the information to the screen */
void print_out();

int UAP_from_hec(uint8_t *packet);

/* For unwhitened headers */
void header();

/* Create the CRC */
uint16_t crcgen(uint8_t *packet, int length, int UAP);

int payload();

/* Check that the Access Code is correct for the given LAP */
int check_ac(char *stream);

int sniff_ac();

char *unfec13(char *stream, uint8_t *output, int length);
/* removes the whitening from the header */

void unwhiten(uint8_t *input, uint8_t *output, int clock, int length, int skip);

void unwhiten_char(char *input, uint8_t *output, int clock, int length, int skip);

/* To deal with whitened data */
void new_header();

int DM1(int size, int clock);

int DM5(int size, int clock);


 public:
  ~bluetooth_sniffer ();	// public destructor

  // Where all the action really happens

  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_SNIFFER_H */
