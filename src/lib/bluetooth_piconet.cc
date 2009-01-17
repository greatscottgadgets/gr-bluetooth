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

#include <bluetooth_piconet.h>
#include <stdlib.h>

/* constructor */
bluetooth_piconet::bluetooth_piconet(uint32_t LAP, uint8_t UAP, uint8_t clock6, char channel)
{
	/* this can hold twice the approximate number of initial candidates */
	d_clock_candidates = (uint32_t*) malloc(sizeof(uint32_t) * (SEQUENCE_LENGTH / CHANNELS)/32);

	/* this holds the entire hopping sequence */
	d_sequence = (char*) malloc(SEQUENCE_LENGTH);

	d_LAP = LAP;
	d_UAP = UAP;
	precalc();
	address_precalc(((d_UAP<<24) | d_LAP) & 0xfffffff);
	gen_hops();
	d_num_candidates = init_candidates(channel, clock6);
}

/* do all the precalculation that can be done before knowing the address */
void bluetooth_piconet::precalc()
{
	int i;
	int z, p_high, p_low;

	/* populate frequency register bank*/
	for (i = 0; i < CHANNELS; i++)
		d_bank[i] = (i * 2) % CHANNELS;
	/* actual frequency is 2402 + bank[i] MHz */

	/* populate perm_table for all possible inputs */
	for (z = 0; z < 0x20; z++)
		for (p_high = 0; p_high < 0x20; p_high++)
			for (p_low = 0; p_low < 0x200; p_low++)
				d_perm_table[z][p_high][p_low] = perm5(z, p_high, p_low);
}

/* do precalculation that requires the address */
void bluetooth_piconet::address_precalc(int address)
{
	/* precalculate some of hop()'s variables */
	d_a1 = (address >> 23) & 0x1f;
	d_b = (address >> 19) & 0x0f;
	d_c1 = ((address >> 4) & 0x10) +
		((address >> 3) & 0x08) +
		((address >> 2) & 0x04) +
		((address >> 1) & 0x02) +
		(address & 0x01);
	d_d1 = (address >> 10) & 0x1ff;
	d_e = ((address >> 7) & 0x40) +
		((address >> 6) & 0x20) +
		((address >> 5) & 0x10) +
		((address >> 4) & 0x08) +
		((address >> 3) & 0x04) +
		((address >> 2) & 0x02) +
		((address >> 1) & 0x01);
}

/* drop-in replacement for perm5() using lookup table */
int bluetooth_piconet::fast_perm(int z, int p_high, int p_low)
{
	return(d_perm_table[z][p_high][p_low]);
}

/* 5 bit permutation */
/* assumes z is constrained to 5 bits, p_high to 5 bits, p_low to 9 bits */
int bluetooth_piconet::perm5(int z, int p_high, int p_low)
{
	int i, tmp, output, z_bit[5], p[14];
	int index1[] = {0, 2, 1, 3, 0, 1, 0, 3, 1, 0, 2, 1, 0, 1};
	int index2[] = {1, 3, 2, 4, 4, 3, 2, 4, 4, 3, 4, 3, 3, 2};

	/* bits of p_low and p_high are control signals */
	for (i = 0; i < 9; i++)
		p[i] = (p_low >> i) & 0x01;
	for (i = 0; i < 5; i++)
		p[i+9] = (p_high >> i) & 0x01;

	/* bit swapping will be easier with an array of bits */
	for (i = 0; i < 5; i++)
		z_bit[i] = (z >> i) & 0x01;

	/* butterfly operations */
	for (i = 13; i >= 0; i--) {
		/* swap bits according to index arrays if control signal tells us to */
		if (p[i]) {
			tmp = z_bit[index1[i]];
			z_bit[index1[i]] = z_bit[index2[i]];
			z_bit[index2[i]] = tmp;
		}
	}

	/* reconstruct output from rearranged bits */
	output = 0;
	for (i = 0; i < 5; i++)
		output += z_bit[i] << i;

	return(output);
}

/* generate the complete hopping sequence */
void bluetooth_piconet::gen_hops()
{
	/* a, b, c, d, e, f, x, y1, y2 are variable names used in section 2.6 of the spec */
	/* b is already defined */
	/* e is already defined */
	int a, c, d, f, x;
	int h, i, j, k, c_flipped, perm_in, perm_out;

	/* sequence index = clock >> 1 */
	/* (hops only happen at every other clock value) */
	int index = 0;
	f = 0;

	/* nested loops for optimization (not recalculating every variable with every clock tick) */
	for (h = 0; h < 0x04; h++) { /* clock bits 26-27 */
		for (i = 0; i < 0x20; i++) { /* clock bits 21-25 */
			a = d_a1 ^ i;
			for (j = 0; j < 0x20; j++) { /* clock bits 16-20 */
				c = d_c1 ^ j;
				c_flipped = c ^ 0x1f;
				for (k = 0; k < 0x200; k++) { /* clock bits 7-15 */
					d = d_d1 ^ k;
					for (x = 0; x < 0x20; x++) { /* clock bits 2-6 */
						perm_in = ((x + a) % 32) ^ d_b;
						/* y1 (clock bit 1) = 0, y2 = 0 */
						perm_out = fast_perm(perm_in, c, d);
						d_sequence[index++] = d_bank[(perm_out + d_e + f) % CHANNELS];
						/* y1 (clock bit 1) = 1, y2 = 32 */
						perm_out = fast_perm(perm_in, c_flipped, d);
						d_sequence[index++] = d_bank[(perm_out + d_e + f + 32) % CHANNELS];
					}
					f += 16;
				}
			}
		}
	}
}

/* determine channel for a particular hop */
/* replaced with gen_hops() for a complete sequence but could still come in handy */
char bluetooth_piconet::hop(int clock)
{
	int a, c, d, f, x, y1, y2;

	/* following variable names used in section 2.6 of the spec */
	x = (clock >> 2) & 0x1f;
	y1 = (clock >> 1) & 0x01;
	y2 = y1 << 5;
	a = (d_a1 ^ (clock >> 21)) & 0x1f;
	/* b is already defined */
	c = (d_c1 ^ (clock >> 16)) & 0x1f;
	d = (d_d1 ^ (clock >> 7)) & 0x1ff;
	/* e is already defined */
	f = (clock >> 3) & 0x1fffff0;

	/* hop selection */
	return(d_bank[(fast_perm(((x + a) % 32) ^ d_b, (y1 * 0x1f) ^ c, d) + d_e + f + y2) % CHANNELS]);
}

/* create list of initial candidate clock values (hops with same channel as first observed hop) */
int bluetooth_piconet::init_candidates(char channel, int known_clock_bits)
{
	int i;
	int count = 0; /* total number of candidates */

	/* only try clock values that match our known bits */
	for (i = known_clock_bits; i < SEQUENCE_LENGTH; i += 0x40) {
		if (d_sequence[i] == channel) {
			d_clock_candidates[count++] = i;
		}
		//FIXME ought to throw exception if count gets too big
	}
	return count;
}

/* narrow a list of candidate clock values based on a single observed hop */
int bluetooth_piconet::winnow(int count, int offset, char channel)
{
	int i;
	int new_count = 0; /* number of candidates after winnowing */

	/* check every candidate */
	for (i = 0; i < count; i++) {
		if (d_sequence[(d_clock_candidates[i] + offset) % SEQUENCE_LENGTH] == channel) {
			/* this candidate matches the latest hop */
			/* blow away old list of candidates with new one */
			/* safe because new_count can never be greater than i */
			d_clock_candidates[new_count++] = d_clock_candidates[i];
		}
	}
	//FIXME maybe do something if count == 1
	//FIXME maybe do something if count == 0
	return new_count;
}
