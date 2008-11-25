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
#ifndef INCLUDED_BLUETOOTH_LAP_H
#define INCLUDED_BLUETOOTH_LAP_H

#include <gr_sync_block.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

class bluetooth_LAP;
typedef boost::shared_ptr<bluetooth_LAP> bluetooth_LAP_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_LAP.
 */
bluetooth_LAP_sptr bluetooth_make_LAP (int x);

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class bluetooth_LAP : public gr_sync_block
{
private:
  // The friend declaration allows bluetooth_make_LAP to
  // access the private constructor.

  friend bluetooth_LAP_sptr bluetooth_make_LAP (int x);

  bluetooth_LAP (int x);  	// private constructor

  struct LAP_struct
  {
        int LAP;
	int count;
        LAP_struct *next;
  };

  int d_LAP;
  int d_stream_length;
  char *d_stream;
  uint16_t d_consumed;
  bool d_flag;
  LAP_struct *d_LAPs;
  int d_LAP_count;
  int d_x;
  // could be 32 bits, but then it would roll over after about 70 minutes
  uint64_t d_cumulative_count;

void keep_track_of_LAPs();

void printout();

uint8_t *codeword(uint8_t *data, int length, int k);

uint8_t reverse(char byte);

uint8_t *acgen(int LAP);

void convert_to_grformat(uint8_t input, uint8_t *output);

int check_ac(char *stream);

int sniff_ac();

 public:
  ~bluetooth_LAP ();	// public destructor

  // Where all the action really happens

  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_LAP_H */
