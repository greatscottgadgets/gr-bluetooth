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

#include <bluetooth_block.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

class bluetooth_UAP;
typedef boost::shared_ptr<bluetooth_UAP> bluetooth_UAP_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_UAP.
 */
bluetooth_UAP_sptr bluetooth_make_UAP (int LAP);

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class bluetooth_UAP : public bluetooth_block
{
private:
	// The friend declaration allows bluetooth_make_UAP to
	// access the private constructor.

	friend bluetooth_UAP_sptr bluetooth_make_UAP (int LAP);

	bluetooth_UAP (int LAP);	// private constructor

	int d_limit;
	uint64_t d_first_packet_time;
	int d_clock_candidates[64];
	int d_previous_packet_time;
	int d_previous_clock_offset;
	int d_packets_observed;

	/* Converts 8 bytes of grformat to a single byte */
	char gr_to_normal(char *stream);

	int UAP_from_hec(uint8_t *packet);

	int sniff_ac();

	/* Use packet headers to determine UAP */
	void UAP_from_header();

	int crc_check(char *stream, int type, int size, int clock, uint8_t UAP);

	int fhs(char *stream, int clock, uint8_t UAP, int size);

	int DM(char *stream, int clock, uint8_t UAP, int header_bytes, int size);

	int DH(char *stream, int clock, uint8_t UAP, int header_bytes, int size);

	int EV(char *stream, int clock, uint8_t UAP, int type, int size);

public:
  ~bluetooth_UAP ();	// public destructor

  // Where all the action really happens

  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_UAP_H */
