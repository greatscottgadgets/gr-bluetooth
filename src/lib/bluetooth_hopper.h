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
#ifndef INCLUDED_BLUETOOTH_HOPPER_H
#define INCLUDED_BLUETOOTH_HOPPER_H

#include <bluetooth_UAP.h>

static const int sequence_length = 134217728;
static const int channels = 79;

class bluetooth_hopper;
typedef boost::shared_ptr<bluetooth_hopper> bluetooth_hopper_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_hopper.
 */
bluetooth_hopper_sptr bluetooth_make_hopper (int LAP);

/*!
 * \brief Sniff Bluetooth packets, determine UAP, reverse hopping sequence
 * \ingroup block
 */
class bluetooth_hopper : public bluetooth_UAP
{
private:
	// The friend declaration allows bluetooth_make_hopper to
	// access the private constructor.

	friend bluetooth_hopper_sptr bluetooth_make_hopper (int LAP);

	bluetooth_hopper (int LAP);	// private constructor

	int *d_clock_candidates;
	/* these values for hop() can be precalculated */
	int d_b, d_e;
	/* these values for hop() can be precalculated in part (e.g. a1 is the
	 * precalculated part of a) */
	int d_a1, d_c1, d_d1;
	/* frequency register bank */
	int d_bank[channels];
	/* speed up the perm5 function with a lookup table */
	char d_perm_table[0x20][0x20][0x200];
	/* this holds the entire hopping sequence */
	char *d_sequence;


	/* do all the precalculation that can be done before knowing the address */
	void precalc();

	/* do precalculation that requires the address */
	void address_precalc(int address);

	/* drop-in replacement for perm5() using lookup table */
	int fast_perm(int z, int p_high, int p_low);

	/* 5 bit permutation */
	/* assumes z is constrained to 5 bits, p_high to 5 bits, p_low to 9 bits */
	int perm5(int z, int p_high, int p_low);

	/* generate the complete hopping sequence */
	void gen_hops(char *sequence);

	/* determine channel for a particular hop */
	/* replaced with gen_hops() for a complete sequence but could still come in handy */
	char hop(int clock);

	/* create list of initial candidate clock values (hops with same channel as first observed hop) */
	int init_candidates(char *sequence, int *candidates, char channel, int known_clock_bits);

	/* narrow a list of candidate clock values based on a single observed hop */
	int winnow(char *sequence, int *candidates, int count, int offset, char channel);

public:
  ~bluetooth_hopper ();	// public destructor

  // Where all the action really happens

  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_HOPPER_H */
