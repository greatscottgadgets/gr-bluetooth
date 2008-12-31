/*
 * hopsearch.c v0.3 bluetooth hop sequence generation and search
 *
 * Copyright 2008 Michael Ossmann <mike@ossmann.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>

/* length of hopping sequence (2^27) */
#define LENGTH 134217728

/* number of 1MHz channels */
#define CHANNELS 79

#define TRIALS 100000
#define MAX_PATTERN_LENGTH 5000

int pattern_indices[MAX_PATTERN_LENGTH];
char pattern_channels[MAX_PATTERN_LENGTH];
int pattern_length, pattern_start;

/* candidate clock values */
int *candidates;

/* these values for hop() can be precalculated */
int b, e;

/* these values for hop() can be precalculated in part (e.g. a1 is the
 * precalculated part of a) */
int a1, c1, d1;

/* frequency register bank */
int bank[CHANNELS];

/* speed up the perm5 function with a lookup table */
char perm_table[0x20][0x20][0x200];

/* do all the precalculation that can be done before knowing the address */
void precalc()
{
	int i;
	int z, p_high, p_low;

	/* populate frequency register bank*/
	for (i = 0; i < CHANNELS; i++)
		bank[i] = (i * 2) % CHANNELS;
	/* actual frequency is 2402 + bank[i] MHz */

	/* populate perm_table for all possible inputs */
	for (z = 0; z < 0x20; z++)
		for (p_high = 0; p_high < 0x20; p_high++)
			for (p_low = 0; p_low < 0x200; p_low++)
				perm_table[z][p_high][p_low] = perm5(z, p_high, p_low);
}

/* do precalculation that requires the address */
void address_precalc(int address) {
	/* precalculate some of hop()'s variables */
	a1 = (address >> 23) & 0x1f;
	b = (address >> 19) & 0x0f;
	c1 =((address >> 4) & 0x10) +
		((address >> 3) & 0x08) +
		((address >> 2) & 0x04) +
		((address >> 1) & 0x02) +
		(address & 0x01);
	d1 = (address >> 10) & 0x1ff;
	e = ((address >> 7) & 0x40) +
		((address >> 6) & 0x20) +
		((address >> 5) & 0x10) +
		((address >> 4) & 0x08) +
		((address >> 3) & 0x04) +
		((address >> 2) & 0x02) +
		((address >> 1) & 0x01);
}

/* drop-in replacement for perm5() using lookup table */
int fast_perm(int z, int p_high, int p_low)
{
	return(perm_table[z][p_high][p_low]);
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
int gen_hops(char *sequence)
{
	/* a, b, c, d, e, f, x, y1, y2 are variable names used in section 2.6 of the spec */
	/* b is already defined */
	/* e is already defined */
	int a, c, d, f, x, y1, y2;
	int h, i, j, k, c_flipped, perm_in, perm_out;

	/* sequence index = clock >> 1 */
	/* (hops only happen at every other clock value) */
	int index = 0;
	f = 0;

	/* nested loops for optimization (not recalculating every variable with every clock tick) */
	for (h = 0; h < 0x04; h++) { /* clock bits 26-27 */
		for (i = 0; i < 0x20; i++) { /* clock bits 21-25 */
			a = a1 ^ i;
			for (j = 0; j < 0x20; j++) { /* clock bits 16-20 */
				c = c1 ^ j;
				c_flipped = c ^ 0x1f;
				for (k = 0; k < 0x200; k++) { /* clock bits 7-15 */
					d = d1 ^ k;
					for (x = 0; x < 0x20; x++) { /* clock bits 2-6 */
						perm_in = ((x + a) % 32) ^ b;
						/* y1 (clock bit 1) = 0, y2 = 0 */
						perm_out = fast_perm(perm_in, c, d);
						sequence[index++] = bank[(perm_out + e + f) % CHANNELS];
						/* y1 (clock bit 1) = 1, y2 = 32 */
						perm_out = fast_perm(perm_in, c_flipped, d);
						sequence[index++] = bank[(perm_out + e + f + 32) % CHANNELS];
					}
					f += 16;
				}
			}
		}
	}
}

/* determine channel for a particular hop */
/* replaced with gen_hops() for a complete sequence but could still come in handy */
char hop(int clock)
{
	int a, c, d, f, x, y1, y2, y3;

	/* following variable names used in section 2.6 of the spec */
	x = (clock >> 2) & 0x1f;
	y1 = (clock >> 1) & 0x01;
	y2 = y1 << 5;
	a = (a1 ^ (clock >> 21)) & 0x1f;
	/* b is already defined */
	c = (c1 ^ (clock >> 16)) & 0x1f;
	d = (d1 ^ (clock >> 7)) & 0x1ff;
	/* e is already defined */
	f = (clock >> 3) & 0x1fffff0;

	/* hop selection */
	return(bank[(fast_perm(((x + a) % 32) ^ b, (y1 * 0x1f) ^ c, d) + e + f + y2) % CHANNELS]);
}

/* print out enough of the sequence for verification against sample data in spec */
print_table(char *sequence)
{
	int i;

	for (i = 8; i < 520; i++)
		printf("0x%04x, %d\n", i << 1, sequence[i]);
}

/* print out hop intervals for certain channels */
/* helpful for manually coming up with patterns for test searches */
print_intervals(char *sequence)
{
	int i;
	int last = 0;

	for (i = 0; i < LENGTH; i++) {
		if (sequence[i] < 5) { /* pick channels here */
			printf("%d, %d\n", i - last, sequence[i]);
			last = i;
		}
	}
}

/* find a pattern in the hop sequence */
/* return number of matches */
int search(char *sequence)
{
	int i, j;
	int count = 0;
	/* we have acquired the low 6 bits of the index (bits 1-6 of the clock -
	 * bit 0 is also known to be 0 at the start of a slot) in the same manner
	 * that we got the 28 bits of address */
	int known_clock_bits = pattern_start & 0x3f;

	/* only try clock values that match our known bits */
	for (i = known_clock_bits; i < LENGTH; i += 0x40) {
		for (j = 0; j < pattern_length; j++)
			if (sequence[(i + pattern_indices[j]) % LENGTH] != pattern_channels[j])
				break;
		if (j == pattern_length) {
			/* printf("found match at index %d\n", i); */
			count++;
		}
	}
	return(count);
}

/* return a random integer */
unsigned int random_int()
{
	FILE *urandom = NULL;
	unsigned int integer = 0;

	if ((urandom = fopen("/dev/urandom", "r")) == NULL) {
		printf("Failed to open /dev/urandom\n");
		exit(1);
	}
	fread(&integer, 1, 4, urandom); /* assuming int is 32 bits */
	fclose(urandom);

	return(integer);
}

/* return a random address (just the low 28 bits of bd_addr) */
int random_address()
{
	return(random_int() & 0xfffffff);
}

/* return a random index of the hopping sequence */
int random_index()
{
	return(random_int() & 0x7ffffff);
}

/* return the first of several (width) randomly selected adjacent channels */
char random_channel(char width)
{
	/* slightly biased */
	return(random_int() % (80 - width));
}

/* randomly select a pattern of hops limited by observation period (in 1/1600 seconds)
 * and width (number of adjacent channels) */
void select_pattern(char *sequence, int period, char width)
{
	int first, index;
	int start = random_index();
	int i = 0;
	char low_channel = random_channel(width);
	char high_channel = low_channel + width - 1;

	pattern_length = 0;
	for (i = 0; i < period; i++) {
		index = (start + i) % LENGTH;
		if (sequence[index] >= low_channel && sequence[index] <= high_channel) {
			if (pattern_length == 0) {
				first = i;
				pattern_start = index;
			}
			pattern_indices[pattern_length] = i - first;
			pattern_channels[pattern_length++] = sequence[index];
		}
	}
}

/* do a bunch of random pattern matches for statistical analysis */
void simulate(char *sequence)
{
	int address;
	int i, j, period;

	/* how many channels wide is our observation */
	char widths[] = {1, 8, 79};

	for (j = 0; j < TRIALS; j++) {
		for (i = 0; i < 3; i++) {
			for (period = 10; period <= 1600; period += 10) {
				address = random_address();
				address_precalc(address);
				gen_hops(sequence);
				select_pattern(sequence, period, widths[i]);
				/* printf("selected pattern starting at %d\n", pattern_start); */
				printf("%d, %d, %d, %d\n", widths[i], period, search(sequence), pattern_length);
			}
		}
	}
}

/* create list of initial candidate clock values (hops with same channel as first observed hop) */
int init_candidates(char *sequence, int *candidates, char channel, int known_clock_bits)
{
	int i;
	int count = 0; /* total number of candidates */

	/* only try clock values that match our known bits */
	for (i = known_clock_bits; i < LENGTH; i += 0x40) {
		if (sequence[i] == channel) {
			candidates[count++] = i;
		}
	}
	return count;
}

/* narrow a list of candidate clock values based on a single observed hop */
int winnow(char *sequence, int *candidates, int count, int offset, char channel)
{
	int i;
	int new_count = 0; /* number of candidates after winnowing */

	/* check every candidate */
	for (i = 0; i < count; i++) {
		if (sequence[(candidates[i] + offset) % LENGTH] == channel) {
			/* this candidate matches the latest hop */
			/* blow away old list of candidates with new one */
			/* safe because new_count can never be greater than i */
			candidates[new_count++] = candidates[i];
		}
	}
	return new_count;
}

/* testing a specific data set */
void keyboard_test(char *sequence)
{
	int address = 0x14831dd;
	address_precalc(address);
	char low_channel = 72;
	gen_hops(sequence);
	int known_clock_bits = 0x04;
	int count = init_candidates(sequence, candidates, 78, known_clock_bits);
	printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 94, 76);
	printf("count: %d\n", count);
	//count = winnow(sequence, candidates, count, 95, 76);
	//printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 414, 74);
	printf("count: %d\n", count);
	//count = winnow(sequence, candidates, count, 957, 72);
	//printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 960, 74);
	printf("count: %d\n", count);
	//count = winnow(sequence, candidates, count, 961, 74);
	//printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 1151, 73);
	printf("count: %d\n", count);
}

/* testing a specific data set */
void headset_test(char *sequence)
{
	int i;
	int address = 0xf24d952;
	address_precalc(address);
	gen_hops(sequence);
	int known_clock_bits = 0x18;
	int count = init_candidates(sequence, candidates, 74, known_clock_bits);
	printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 499, 74);
	printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 636, 74);
	printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 942, 74);
	printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 1091, 74);
	printf("count: %d\n", count);
	count = winnow(sequence, candidates, count, 1435, 74);
	printf("count: %d\n", count);
	if (count == 1) {
		printf("clock: 0x%x\n", candidates[0]);
		/* print a few hops for verification */
		for (i = 0; i < 1500; i++)
			if(sequence[candidates[0]+i] > 72 && sequence[candidates[0]+i] < 76)
				printf("%d ms: channel %d\n", 32+i*625/1000, sequence[candidates[0]+i]);
	}
}

/* test winnowing method with simulated hop observations */
void winnow_test(char *sequence, int start, char width, char low_channel, int address)
{
	int i, index;
	int first; /* index (clock value) of first observed hop */
	char high_channel = low_channel + width - 1; /* highest observed channel */
	int count; /* number of candidate clock values */
	int seen = 0; /* total number of observed hops */
	int known_clock_bits; /* 6 bits of clock derived from unwhitening */

	//printf("channels %d - %d\n", low_channel, high_channel);
	//printf("starting at %d\n", start);

	gen_hops(sequence);

	/* find first observed hop after start */
	for (i = 0; i < LENGTH; i++) {
		index = (start + i) % LENGTH;
		if (sequence[index] >= low_channel && sequence[index] <= high_channel) {
			first = index;
			break;
		}
	}
	//printf("first hop at %d\n", first);

	known_clock_bits = first & 0x3f;
	count = init_candidates(sequence, candidates, sequence[first], known_clock_bits);

	//printf("%d initial candidates\n", count);

	/* find each observed hop chronologically and winnow the list of candidates until a winner emerges */
	/* if we get to half the total sequence length, we're in trouble (and have wasted nearly 12 hours) */
	for (i = 1; i < LENGTH / 2; i++) {
		index = (first + i) % LENGTH;
		//if (sequence[index] >= low_channel && sequence[index] <= high_channel) {
		if (sequence[index] >= low_channel && sequence[index] <= high_channel && random_int() < 858993459 ) {
			/* this is a hop we observe */
			seen++;
			count = winnow(sequence, candidates, count, i, sequence[index]);
			//printf("%d candidates after %d hops\n", count, i);
			if (count < 2) break;
		}
	}
	if (count == 1) {
		/* we have a winner */
		//printf("found match at index %d after %d hops\n", candidates[0], i);
		printf("%d, %d, 0x%0.7x, %d, %d, %d\n", width, low_channel, address, candidates[0], i, seen);
	} else {
		printf("oops! %d candidates after %d hops\n", count, i);
	}
}

/* perform winnow_test() for various random conditions */
void winnow_trials(char *sequence)
{
	int i;
	char width; /* number of adjacent channels observed */
	char low_channel; /* lowest observed channel */
	int address; /* low 28 bits of bd_addr */
	int start; /* clock value at start of observation period */

	for (i = 0; i < TRIALS; i++) {
		for (width = 1; width <= CHANNELS; width++) {
			address = random_address();
			address_precalc(address);
			low_channel = random_channel(width);
			start = random_index();
			winnow_test(sequence, start, width, low_channel, address);
		}
	}
}

main()
{
	/* this holds the entire hopping sequence */
	char *sequence;
	/* address = least significant 28 bits (or more, but more is ignored) of bd_addr */
	int address;

	/* populate the permutation lookup table and other precalculations */
	precalc();

	sequence = malloc(LENGTH);

	/* there should be no more than LENGTH/CHANNELS initial candidates */
	/* actually, the total number should be approximately (LENGTH/CHANNELS)/64, so this is very safe */
	candidates = malloc(sizeof(int) * LENGTH / CHANNELS);

	/* perfom simulations and print results to stdout */
	//simulate(sequence);

	//winnow_trials(sequence);
	//keyboard_test(sequence);
	headset_test(sequence);

	//winnow_test(sequence, 74501564, 54, 17, 0x0579b630);
	//winnow_test(sequence, 115241802, 72, 7, 0xe6ad430);
	//winnow_test(sequence, 82738031, 1, 60, 0x01c9e29);
	//winnow_test(sequence, 91351630, 79, 0, 0xf8f5dee);
	//winnow_test(sequence, 116916848, 62, 15, 0x02ddd10a);

	/* these addresses are used for the sample sequences in the spec:
	 *
	 * address = 0x00000000;
	 * address = 0x2a96ef25;
	 * address = 0x6587cba9;
	 *
	 */

	/* construct the complete hopping sequence for the given address */
	
	/* 
	 * address_precalc(address);
	 * gen_hops(sequence);
	 * print_table(sequence);
	 */
	free(sequence);
	free(candidates);
}
