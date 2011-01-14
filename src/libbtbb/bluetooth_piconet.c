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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bluetooth_piconet.h"
#include <stdlib.h>

void init_piconet(piconet *pnet)
{
	pnet->got_first_packet = 0;
	pnet->packets_observed = 0;
	pnet->total_packets_observed = 0;
	pnet->hop_reversal_inited = 0;
	pnet->afh = 0;
	pnet->looks_like_afh = 0;
	pnet->have_UAP = 0;
	pnet->have_NAP = 0;
	pnet->have_clk6 = 0;
	pnet->have_clk27 = 0;
}

/* initialize the hop reversal process */
int init_hop_reversal(int aliased, piconet *pnet)
{
	int max_candidates;
	uint32_t clock;

	printf("\nCalculating complete hopping sequence.\n");

	if(aliased)
		max_candidates = (SEQUENCE_LENGTH / ALIASED_CHANNELS) / 32;
	else
		max_candidates = (SEQUENCE_LENGTH / CHANNELS) / 32;
		
	/* this can hold twice the approximate number of initial candidates */
	pnet->clock_candidates = (uint32_t*) malloc(sizeof(uint32_t) * max_candidates);

	/* this holds the entire hopping sequence */
	pnet->sequence = (char*) malloc(SEQUENCE_LENGTH);

	precalc(pnet);
	address_precalc(((pnet->UAP<<24) | pnet->LAP) & 0xfffffff, pnet);
	gen_hops(pnet);
	clock = (pnet->clk_offset + pnet->first_pkt_time) & 0x3f;
	pnet->num_candidates = init_candidates(pnet->pattern_channels[0], clock, pnet);
	pnet->winnowed = 0;
	pnet->hop_reversal_inited = 1;
	pnet->have_clk27 = 0;
	pnet->aliased = aliased;

	printf("%d initial CLK1-27 candidates\n", pnet->num_candidates);

	return pnet->num_candidates;
}

/* do all the precalculation that can be done before knowing the address */
void precalc(piconet *pnet)
{
	int i, z, p_high, p_low;

	/* populate frequency register bank*/
	for (i = 0; i < CHANNELS; i++)
			pnet->bank[i] = ((i * 2) % CHANNELS);
	/* actual frequency is 2402 + pnet->bank[i] MHz */

	/* populate perm_table for all possible inputs */
	for (z = 0; z < 0x20; z++)
		for (p_high = 0; p_high < 0x20; p_high++)
			for (p_low = 0; p_low < 0x200; p_low++)
				pnet->perm_table[z][p_high][p_low] = perm5(z, p_high, p_low);
}

/* do precalculation that requires the address */
void address_precalc(int address, piconet *pnet)
{
	/* precalculate some of single_hop()/gen_hop()'s variables */
	pnet->a1 = (address >> 23) & 0x1f;
	pnet->b = (address >> 19) & 0x0f;
	pnet->c1 = ((address >> 4) & 0x10) +
		((address >> 3) & 0x08) +
		((address >> 2) & 0x04) +
		((address >> 1) & 0x02) +
		(address & 0x01);
	pnet->d1 = (address >> 10) & 0x1ff;
	pnet->e = ((address >> 7) & 0x40) +
		((address >> 6) & 0x20) +
		((address >> 5) & 0x10) +
		((address >> 4) & 0x08) +
		((address >> 3) & 0x04) +
		((address >> 2) & 0x02) +
		((address >> 1) & 0x01);
}

/* drop-in replacement for perm5() using lookup table */
int fast_perm(int z, int p_high, int p_low, piconet *pnet)
{
	return(pnet->perm_table[z][p_high][p_low]);
}

/* 5 bit permutation */
/* assumes z is constrained to 5 bits, p_high to 5 bits, p_low to 9 bits */
int perm5(int z, int p_high, int p_low)
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
void gen_hops(piconet *pnet)
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
			a = pnet->a1 ^ i;
			for (j = 0; j < 0x20; j++) { /* clock bits 16-20 */
				c = pnet->c1 ^ j;
				c_flipped = c ^ 0x1f;
				for (k = 0; k < 0x200; k++) { /* clock bits 7-15 */
					d = pnet->d1 ^ k;
					for (x = 0; x < 0x20; x++) { /* clock bits 2-6 */
						perm_in = ((x + a) % 32) ^ pnet->b;
						/* y1 (clock bit 1) = 0, y2 = 0 */
						perm_out = fast_perm(perm_in, c, d, pnet);
						pnet->sequence[index] = pnet->bank[(perm_out + pnet->e + f) % CHANNELS];
						if (pnet->afh) {
							pnet->sequence[index + 1] = pnet->sequence[index];
						} else {
							/* y1 (clock bit 1) = 1, y2 = 32 */
							perm_out = fast_perm(perm_in, c_flipped, d, pnet);
							pnet->sequence[index + 1] = pnet->bank[(perm_out + pnet->e + f + 32) % CHANNELS];
						}
						index += 2;
					}
					f += 16;
				}
			}
		}
	}
}

/* determine channel for a particular hop */
/* replaced with gen_hops() for a complete sequence but could still come in handy */
char single_hop(int clock, piconet *pnet)
{
	int a, c, d, f, x, y1, y2;

	/* following variable names used in section 2.6 of the spec */
	x = (clock >> 2) & 0x1f;
	y1 = (clock >> 1) & 0x01;
	y2 = y1 << 5;
	a = (pnet->a1 ^ (clock >> 21)) & 0x1f;
	/* b is already defined */
	c = (pnet->c1 ^ (clock >> 16)) & 0x1f;
	d = (pnet->d1 ^ (clock >> 7)) & 0x1ff;
	/* e is already defined */
	f = (clock >> 3) & 0x1fffff0;

	/* hop selection */
	return(pnet->bank[(fast_perm(((x + a) % 32) ^ pnet->b, (y1 * 0x1f) ^ c, d, pnet) + pnet->e + f + y2) % CHANNELS]);
}

/* look up channel for a particular hop */
char hop(int clock, piconet *pnet)
{
	return pnet->sequence[clock];
}

/* create list of initial candidate clock values (hops with same channel as first observed hop) */
int init_candidates(char channel, int known_clock_bits, piconet *pnet)
{
	int i;
	int count = 0; /* total number of candidates */
	char observable_channel; /* accounts for aliasing if necessary */

	/* only try clock values that match our known bits */
	for (i = known_clock_bits; i < SEQUENCE_LENGTH; i += 0x40) {
		if (pnet->aliased)
			observable_channel = aliased_channel(pnet->sequence[i]);
		else
			observable_channel = pnet->sequence[i];
		if (observable_channel == channel)
			pnet->clock_candidates[count++] = i;
		//FIXME ought to throw exception if count gets too big
	}
	return count;
}

/* narrow a list of candidate clock values based on a single observed hop */
int channel_winnow(int offset, char channel, piconet *pnet)
{
	int i;
	int new_count = 0; /* number of candidates after winnowing */
	char observable_channel; /* accounts for aliasing if necessary */

	/* check every candidate */
	for (i = 0; i < pnet->num_candidates; i++) {
		if (pnet->aliased)
			observable_channel = aliased_channel(pnet->sequence[(pnet->clock_candidates[i] + offset) % SEQUENCE_LENGTH]);
		else
			observable_channel = pnet->sequence[(pnet->clock_candidates[i] + offset) % SEQUENCE_LENGTH];
		if (observable_channel == channel) {
			/* this candidate matches the latest hop */
			/* blow away old list of candidates with new one */
			/* safe because new_count can never be greater than i */
			pnet->clock_candidates[new_count++] = pnet->clock_candidates[i];
		}
	}
	pnet->num_candidates = new_count;

	if (new_count == 1) {
		pnet->clk_offset = (pnet->clock_candidates[0] - pnet->first_pkt_time)
				& 0x7ffffff;
		pnet->have_clk27 = 1;
		printf("\nAcquired CLK1-27 offset = 0x%07x\n", pnet->clk_offset);
	} else if (new_count == 0) {
		reset(pnet);
	} else {
		printf("%d CLK1-27 candidates remaining\n", new_count);
	}

	return new_count;
}

/* narrow a list of candidate clock values based on all observed hops */
int winnow(piconet *pnet)
{
	int new_count = pnet->num_candidates;
	int index, last_index;
	uint8_t channel, last_channel;

	for (; pnet->winnowed < pnet->packets_observed; pnet->winnowed++) {
		index = pnet->pattern_indices[pnet->winnowed];
		channel = pnet->pattern_channels[pnet->winnowed];
		new_count = channel_winnow(index, channel, pnet);

		if (pnet->packets_observed > 0) {
			last_index = pnet->pattern_indices[pnet->winnowed - 1];
			last_channel = pnet->pattern_channels[pnet->winnowed - 1];
			/*
			 * Two packets in a row on the same channel should only
			 * happen if adaptive frequency hopping is in use.
			 * There can be false positives, though, especially if
			 * there is aliasing.
			 */
			if (!pnet->looks_like_afh && (index == last_index + 1)
					&& (channel == last_channel))
				pnet->looks_like_afh = 1;
		}
	}
	
	return new_count;
}

/* use packet headers to determine UAP */
int UAP_from_header(packet *pkt, piconet *pnet)
{
	uint8_t UAP;
	int count, retval, first_clock = 0;

	int starting = 0;
	int remaining = 0;
	uint32_t clkn = pkt->clkn;

	if (!pnet->got_first_packet)
		pnet->first_pkt_time = clkn;

	if (pnet->packets_observed < MAX_PATTERN_LENGTH) {
		pnet->pattern_indices[pnet->packets_observed] = clkn - pnet->first_pkt_time;
		pnet->pattern_channels[pnet->packets_observed] = pkt->channel;
	} else {
		printf("Oops. More hops than we can remember.\n");
		reset(pnet);
		return 0; //FIXME ought to throw exception
	}
	pnet->packets_observed++;
	pnet->total_packets_observed++;

	/* try every possible first packet clock value */
	for (count = 0; count < 64; count++) {
		/* skip eliminated candidates unless this is our first time through */
		if (pnet->clock6_candidates[count] > -1 || !pnet->got_first_packet) {
			/* clock value for the current packet assuming count was the clock of the first packet */
			int clock = (count + clkn - pnet->first_pkt_time) % 64;
			starting++;
			UAP = try_clock(clock, pkt);
			retval = -1;

			/* if this is the first packet: populate the candidate list */
			/* if not: check CRCs if UAPs match */
			if (!pnet->got_first_packet || UAP == pnet->clock6_candidates[count])
				retval = crc_check(clock, pkt);

			// debugging with a particular UAP
			//if (UAP == 0x61 || pnet->clock6_candidates[count] == 0x61)
				//printf("%u: UAP %02x, %02x, type %u, clock %u, clkn %u, first %u, result %d\n", count, UAP, pnet->clock6_candidates[count], pkt->packet_type, clock, clkn, pnet->first_pkt_time, retval);

			switch(retval) {

			case -1: /* UAP mismatch */
			case 0: /* CRC failure */
				pnet->clock6_candidates[count] = -1;
				break;

			case 1: /* inconclusive result */
				pnet->clock6_candidates[count] = UAP;
				/* remember this count because it may be the correct clock of the first packet */
				first_clock = count;
				remaining++;
				break;

			default: /* CRC success */
				printf("Correct CRC! UAP = 0x%x found after %d total packets.\n",
						UAP, pnet->total_packets_observed);
				pnet->clk_offset = (count - (pnet->first_pkt_time & 0x3f)) & 0x3f;
				pnet->UAP = UAP;
				pnet->have_clk6 = 1;
				pnet->have_UAP = 1;
				pnet->total_packets_observed = 0;
				return 1;
			}
		}
	}

	pnet->got_first_packet = 1;

	printf("reduced from %d to %d CLK1-6 candidates\n", starting, remaining);

	if (remaining == 1) {
		pnet->clk_offset = (first_clock - (pnet->first_pkt_time & 0x3f)) & 0x3f;
		pnet->UAP = pnet->clock6_candidates[first_clock];
		pnet->have_clk6 = 1;
		pnet->have_UAP = 1;
		printf("We have a winner! UAP = 0x%x found after %d total packets.\n",
				pnet->UAP, pnet->total_packets_observed);
		pnet->total_packets_observed = 0;
		return 1;
	}

	if (remaining == 0)
		reset(pnet);

	return 0;
}

/* return the observable channel (26-50) for a given channel (0-78) */
char aliased_channel(char channel)
{
		return ((channel + 24) % ALIASED_CHANNELS) + 26;
}

/* reset UAP/clock discovery */
void reset(piconet *pnet)
{
	printf("no candidates remaining! starting over . . .\n");

	if(pnet->hop_reversal_inited) {
		free(pnet->clock_candidates);
		free(pnet->sequence);
	}
	pnet->got_first_packet = 0;
	pnet->packets_observed = 0;
	pnet->hop_reversal_inited = 0;
	pnet->have_UAP = 0;
	pnet->have_clk6 = 0;
	pnet->have_clk27 = 0;

	/*
	 * If we have recently observed two packets in a row on the same
	 * channel, try AFH next time.  If not, don't.
	 */
	pnet->afh = pnet->looks_like_afh;
	pnet->looks_like_afh = 0;
}

/* add a packet to the queue */
void enqueue(packet *pkt, piconet *pnet)
{
	pkt_queue *head;
	//pkt_queue item;
	
	pkt_queue item = {pkt, NULL};
	head = pnet->queue;
	
	if (head == NULL) {
		pnet->queue = &item;
	} else {
		for(; head->next != NULL; head = head->next)
		  ;
		head->next = &item;
	}
}

/* pull the first packet from the queue (FIFO) */
packet *dequeue(piconet *pnet)
{
	packet *pkt;

	if (pnet->queue == NULL) {
		pkt = NULL;
	} else {
		pkt = pnet->queue->pkt;
		pnet->queue = pnet->queue->next;
	}

	return pkt;
}
