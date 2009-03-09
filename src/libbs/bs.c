#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <err.h>

#include "bs.h"

#define AC_LEN			72
#define MAX_PATTERN_LENGTH	100
#define SEQUENCE_LENGTH		134217728
#define CHANNELS		79
#define MAX_PAYLOAD		1024

struct piconet {
        struct piconet  *p_next;

        /* lower address part (of master's BD_ADDR) */
        uint32_t        p_LAP;

        /* upper address part (of master's BD_ADDR) */
        int		p_UAP;

        /* have we collected the first packet in a UAP discovery attempt? */
        int		p_got_first_packet;

        /* number of packets observed during one attempt at UAP/clock discovery */
        int		p_packets_observed;

        /* total number of packets observed */
        int		p_total_packets_observed;

        /* CLK1-6 candidates */
        int		p_clock6_candidates[64];

        /* CLK1-6 */
        uint8_t		p_clock6;

        /* remember patterns of observed hops */
        int		p_pattern_indices[MAX_PATTERN_LENGTH];
        uint8_t		p_pattern_channels[MAX_PATTERN_LENGTH];

        /* number of time slots between first packet and previous packet */
        int 		p_previous_clock_offset;

	btclock_t	p_last_clock;
	btclock_t	p_clock;
	btclock_t	p_clock_offset;

        /* CLK1-27 candidates */
        uint32_t	*p_clock_candidates;

        /* this holds the entire hopping sequence */
        char		*p_sequence;

        /* frequency register bank */
        int		p_bank[CHANNELS];

        /* speed up the perm5 function with a lookup table */
        char		p_perm_table[0x20][0x20][0x200];

        /* these values for hop() can be precalculated */
        int		p_b, p_e;

        /* these values for hop() can be precalculated in part (e.g. a1 is the
         * precalculated part of a) */
        int		p_a1, p_c1, p_d1;

        /* number of candidates for CLK1-27 */
        int		p_num_candidates;

        /* number of observed packets that have been used to winnow the candidates */
        int		p_winnowed;
};

/* XXX not thread safe */
struct bs_t {
        struct piconet	bs_piconets;
	evcb_t		bs_ev_cb;
	void		*bs_ev_priv;

	/* XXX factor out "current packet info" into a struct. */
	RXINFO		*bs_rx;
	struct piconet	*bs_current;

        /* upper address part */
        uint8_t		bs_UAP;

        /* packet type */
        int		bs_packet_type;
        uint8_t		bs_lower_clock;

        /* payload length: the total length of the asynchronous data.
         * This does not include the length of synchronous data, such as the voice
         * field of a DV packet.
         * If there is a payload header, this payload length is payload body length
         * (the length indicated in the payload header's length field) plus
         * d_payload_header_length plus 2 bytes CRC (if present).
         */
        int		bs_payload_length;
	uint8_t		bs_payload[MAX_PAYLOAD];
	btclock_t	bs_clock;
	struct bthdr	bs_hdr;
	void		*bs_raw;
	int		bs_rlen;
};

static uint8_t INDICES[] = 
	{99, 85, 17, 50, 102, 58, 108, 45, 92, 62, 32, 118, 88, 11, 80, 2, 37,
	69, 55, 8, 20, 40, 74, 114, 15, 106, 30, 78, 53, 72, 28, 26, 68, 7, 39,
	113, 105, 77, 71, 25, 84, 49, 57, 44, 61, 117, 10, 1, 123, 124, 22, 125,
	111, 23, 42, 126, 6, 112, 76, 24, 48, 43, 116, 0};

static uint8_t WHITENING_DATA[] =
	{1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1,
	1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0,
	1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1,
	1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0,
	0, 0, 0, 1, 1, 1, 1};

static uint8_t PREAMBLE_DISTANCE[] =
	{2,2,1,2,2,1,2,2,1,2,0,1,2,2,1,2,2,1,2,2,1,0,2,1,2,2,1,2,2,1,2,2};

/* XXX generate at runtime */
const uint8_t TRAILER_DISTANCE[] = {
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 3, 3, 2, 3, 2, 2, 1,
	5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 3, 3, 2, 3, 2, 2, 1,
	5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 3, 3, 2, 3, 2, 2, 1,
	5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	3, 2, 2, 1, 2, 1, 1, 0, 4, 3, 3, 2, 3, 2, 2, 1, 4, 3, 3, 2, 3, 2, 2, 1,
	5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 3, 3, 2, 3, 2, 2, 1,
	5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 3, 3, 2, 3, 2, 2, 1,
	5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
	0, 1, 1, 2, 1, 2, 2, 3, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 3, 3, 2, 3, 2, 2, 1,
	5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 5, 4, 4, 3, 4, 3, 3, 2, 5, 5, 5, 4, 5, 4, 4, 3,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5, 3, 4, 4, 5, 4, 5, 5, 5, 2, 3, 3, 4, 3, 4, 4, 5,
	5, 5, 5, 4, 5, 4, 4, 3, 4, 5, 5, 5, 5, 5, 5, 4, 4, 5, 5, 5, 5, 5, 5, 4,
	3, 4, 4, 5, 4, 5, 5, 5 };

static void hexdump(void *data, size_t len)
{
	uint8_t *x = data;
	int did = 0;

	while (len--) {
		printf("%.2X ", *x++);

		if (++did == 16) {
			printf("\n");
			did = 0;
		}
	}

	if (did)
		printf("\n");
}

/* Convert some number of bits of an air order array to a host order integer */
static uint8_t air_to_host8(uint8_t *air_order, int bits)
{       
        int i;            
        uint8_t host_order = 0;

        for (i = 0; i < bits; i++)
                host_order |= (air_order[i] << i);

        return host_order;
}
                         
static uint16_t air_to_host16(uint8_t *air_order, int bits)
{       
        int i;            
        uint16_t host_order = 0;

        for (i = 0; i < bits; i++)
                host_order |= (air_order[i] << i);

        return host_order;
}
                         
static uint32_t air_to_host32(uint8_t *air_order, int bits)
{            
        int i;
        uint32_t host_order = 0;

        for (i = 0; i < bits; i++)
                host_order |= (air_order[i] << i);

        return host_order;
}

/* A linear feedback shift register */
static uint8_t *lfsr(uint8_t *data, int length, int k, uint8_t *g)
/*                         
 * A linear feedback shift register
 * used for the syncword in the access code
 * and the fec2/3 encoding (could also be used for the HEC/CRC)
 * Although I'm not sure how to run it backwards for HEC->UAP
 */
{                  
        int    i, j;       
        uint8_t *cw, feedback;
        cw = (uint8_t *) calloc(length - k, 1);
                           
        for (i = k - 1; i >= 0; i--) {
                feedback = data[i] ^ cw[length - k - 1];
                if (feedback != 0) {
                        for (j = length - k - 1; j > 0; j--)
                                if (g[j] != 0)
                                        cw[j] = cw[j - 1] ^ feedback;
                                else
                                        cw[j] = cw[j - 1];
                        cw[0] = g[0] && feedback;
                } else {
                        for (j = length - k - 1; j > 0; j--)
                                cw[j] = cw[j - 1];
                        cw[0] = 0;
                }
        }
        return cw;
}

/* Convert from normal bytes to one-LSB-per-byte format */
static void convert_to_grformat(uint8_t input, uint8_t *output)
{       
        int count;
        for(count = 0; count < 8; count++)
        {       
                output[count] = (input & 0x80) >> 7;
                input <<= 1;
        }
}

/* Convert some number of bits in a host order integer to an air order array */
static void host_to_air(uint8_t host_order, char *air_order, int bits)
{
    int i;
    for (i = 0; i < bits; i++)
        air_order[i] = (host_order >> i) & 0x01;
}

/* Reverse the bits in a byte */
static uint8_t reverse(char byte)
{       
        return (byte & 0x80) >> 7 | (byte & 0x40) >> 5 | (byte & 0x20) >> 3 | (byte & 0x10) >> 1 | (byte & 0x08) << 1 | (byte & 0x04) << 3 | (byte & 0x02) << 5 | (byte & 0x01) << 7;
}

/* Generate Access Code from an LAP */
static uint8_t *acgen(int LAP)
{                           
        /* Endianness - Assume LAP is MSB first, rest done LSB first */
        uint8_t *retval, count, *cw, *data;
        retval = (uint8_t *) calloc(9,1);
        data = (uint8_t *) malloc(30);
        // pseudo-random sequence to XOR with LAP and syncword
        uint8_t pn[] = {0x03,0xF2,0xA3,0x3D,0xD6,0x9B,0x12,0x1C,0x10};
        // generator polynomial for the access code
        uint8_t g[] = {1,0,0,1,0,1,0,1,1,0,1,1,1,1,0,0,1,0,0,0,1,1,1,0,1,0,1,0,0,0,0,1,1,0,1};
                            
        LAP = reverse((LAP & 0xff0000)>>16) | (reverse((LAP & 0x00ff00)>>8)<<8) | (reverse(LAP & 0x0000ff)<<16);
                            
        retval[4] = (LAP & 0xc00000)>>22;
        retval[5] = (LAP & 0x3fc000)>>14;
        retval[6] = (LAP & 0x003fc0)>>6;
        retval[7] = (LAP & 0x00003f)<<2;

        /* Trailer */
        if(LAP & 0x1)
        {       retval[7] |= 0x03;
                retval[8] = 0x2a;
        } else
                retval[8] = 0xd5;

        for(count = 4; count < 9; count++)
                retval[count] ^= pn[count];

        data[0] = (retval[4] & 0x02) >> 1;
        data[1] = (retval[4] & 0x01);
        host_to_air(reverse(retval[5]), (char *) data+2, 8);
        host_to_air(reverse(retval[6]), (char *) data+10, 8);
        host_to_air(reverse(retval[7]), (char *) data+18, 8);
        host_to_air(reverse(retval[8]), (char *) data+26, 4);

        cw = lfsr(data, 64, 30, g);
        free(data);

        retval[0] = cw[0] << 3 | cw[1] << 2 | cw[2] << 1 | cw[3];
        retval[1] = cw[4] << 7 | cw[5] << 6 | cw[6] << 5 | cw[7] << 4 | cw[8] << 3 | cw[9] << 2 | cw[10] << 1 | cw[11];
        retval[2] = cw[12] << 7 | cw[13] << 6 | cw[14] << 5 | cw[15] << 4 | cw[16] << 3 | cw[17] << 2 | cw[18] << 1 | cw[19];
        retval[3] = cw[20] << 7 | cw[21] << 6 | cw[22] << 5 | cw[23] << 4 | cw[24] << 3 | cw[25] << 2 | cw[26] << 1 | cw[27];
        retval[4] = cw[28] << 7 | cw[29] << 6 | cw[30] << 5 | cw[31] << 4 | cw[32] << 3 | cw[33] << 2 | (retval[4] & 0x3);
        free(cw);
        
        for(count = 0; count < 9; count++)
                retval[count] ^= pn[count];

        /* Preamble */
        if(retval[0] & 0x08)
                retval[0] |= 0xa0;
        else    
                retval[0] |= 0x50;

        return retval;
}

/* Create an Access Code from LAP and check it against stream */
static int check_ac(uint8_t *stream, int LAP)
{                       
        int count, aclength, biterrors;
        uint8_t *ac, *grdata;
        aclength = 72;
        biterrors = 0;
                        
        /* Generate AC */
        ac = acgen(LAP);
                     
        /* Check AC */  
        /* Convert it to grformat, 1 bit per byte, in the LSB */
        grdata = (uint8_t *) malloc(aclength);
                        
        for(count = 0; count < 9; count++)
                convert_to_grformat(ac[count], &grdata[count*8]);
        free(ac);
                        
        for(count = 0; count < aclength; count++)
        {
                if(grdata[count] != stream[count])
                        biterrors++;
                        //FIXME do error correction instead of detection
                if(biterrors>=7)
                {
                        free(grdata);
                        return 0;
                }
        }
        if(biterrors)
        {
                //printf("POSSIBLE PACKET, LAP = %06x with %d errors\n", LAP, biterrors);
                free(grdata);
                //return false;
                return 1;
        }

        free(grdata);
        return 1;
}

static void notify_event(BS *bs, int event)
{
	struct btevent be;
	struct piconet *p = bs->bs_current;
	int rc;
	uint32_t lap;

	if (!bs->bs_ev_cb)
		return;

	assert(p); /* XXX doesn't have to be true, but makes code simpler */

	memset(&be, 0, sizeof(be));
	lap = htobe32(p->p_LAP);
	memcpy(be.be_lap, ((uint8_t*) &lap) + 1, sizeof(be.be_lap));
	be.be_type   = event;
	be.be_uap    = p->p_UAP;
	be.be_rclock = p->p_clock;
	be.be_lclock = bs->bs_clock;
	be.be_flags  = 0;
	be.be_hdr    = bs->bs_hdr;
	be.be_len    = bs->bs_payload_length;
	be.be_data   = bs->bs_payload;
	be.be_priv   = bs->bs_ev_priv;
	be.be_raw    = bs->bs_raw;
	be.be_rlen   = bs->bs_rlen;

	rc = bs->bs_ev_cb(&be);
	/* XXX handle rc */
}

static struct piconet *find_piconet(BS *bs, uint32_t lap)
{
	struct piconet *p = bs->bs_piconets.p_next;

	/* XXX optimize */
	while (p) {
		if (p->p_LAP == lap)
			return p;

		p = p->p_next;
	}

	return NULL;
}

static void do_load_piconet(BS *bs, uint32_t lap, int notify)
{
	struct piconet *p;

	p = find_piconet(bs, lap);
	if (!p) {
		p = malloc(sizeof(*p));
		if (!p)
			err(1, "malloc()");

		memset(p, 0, sizeof(*p));
		p->p_LAP   = lap;
		p->p_next  = bs->bs_piconets.p_next;
		p->p_UAP   = -1;
		p->p_clock = (btclock_t) -1;
		bs->bs_piconets.p_next = p;
		bs->bs_current = p;

		if (notify)
			notify_event(bs, EVENT_LAP);
	}

	bs->bs_current = p;
}

static void load_piconet(BS *bs, uint32_t lap)
{
	do_load_piconet(bs, lap, 1);
}

static void piconet_set_clock(BS *bs, struct piconet *p, btclock_t clock)
{
	p->p_clock	  = clock;
	p->p_clock_offset = p->p_clock - bs->bs_clock;
}

void bs_set_piconet(BS *bs, struct piconet_info *pi)
{
	uint32_t lap = 0;
	struct piconet *p;

	memcpy(&lap, pi->pi_lap, 3);

	do_load_piconet(bs, lap, 0);

	p = bs->bs_current;
	if (pi->pi_uap != -1)
		p->p_UAP = pi->pi_uap;

	if (pi->pi_clock != (btclock_t) -1)
		piconet_set_clock(bs, p, pi->pi_clock);
}

static void *find_packet(BS *bs, void *data, size_t len)
{
	uint8_t *p = data;
	uint8_t preamble; /* start of sync word (includes LSB of LAP) */
	uint16_t trailer; /* end of sync word: barker sequence and trailer
			   *  (includes MSB of LAP)
			   */
	uint32_t LAP;
	int max_distance = 2; /* maximum number of bit errors to tolerate in
			       * preamble + trailer
			       */

	while (len >= AC_LEN) {
		preamble = air_to_host8(p, 5);
		trailer  = air_to_host16(&p[61], 11);

		if ((PREAMBLE_DISTANCE[preamble] 
		    + TRAILER_DISTANCE[trailer]) > max_distance)
			goto __next;

		LAP = air_to_host32(&p[38], 24);

		if (check_ac(p, LAP)) {
			bs->bs_raw  = data;
			bs->bs_rlen = len;
			load_piconet(bs, LAP);
			return p;
		}
__next:
		p++;
		len--;
	}

	return NULL;
}

/* Remove the whitening from an air order array */
static void unwhiten(uint8_t* input, uint8_t* output, int clock, int length,
		     int skip)
{       
        int count, index;
        index = INDICES[clock & 0x3f];
        index += skip;
        index %= 127;

        for(count = 0; count < length; count++)
        {       
                /* unwhiten if d_unwhitened, otherwise just copy input to output */
                output[count] = (1) ? input[count] ^ WHITENING_DATA[index] : input[count];
                index += 1;
                index %= 127;
        }
}

/* Decode 1/3 rate FEC, three like symbols in a row */
static uint8_t *unfec13(uint8_t *stream, uint8_t *output, int length)
{
    int count, a, b, c;

    for(count = 0; count < length; count++)
    {   
        a = 3*count;
        b = a + 1;
        c = a + 2;
        output[count] = ((stream[a] & stream[b]) | (stream[b] & stream[c]) | (stream[c] & stream[a]));
    }
    return stream;
}

/* Decode 2/3 rate FEC, a (15,10) shortened Hamming code */
static uint8_t *unfec23(uint8_t *input, int length)
{       
        /* input points to the input data
         * length is length in bits of the data
         * before it was encoded with fec2/3 */
        uint8_t *output;
        int iptr, optr, blocks;
        uint8_t difference, count, *codeword;
        uint8_t fecgen[] = {1,1,0,1,0,1};

        iptr = -15;
        optr = -10;
        difference = length % 10;
        // padding at end of data
        if(0!=difference)
                length += (10 - difference);

        blocks = length/10;
        output = malloc(length);

        while(blocks) {
                iptr += 15;
                optr += 10;
                blocks--;
                
                // copy data to output
                for(count=0;count<10;count++)
                        output[optr+count] = input[iptr+count];
                
                // call fec23gen on data to generate the codeword
                //codeword = fec23gen(input+iptr);
                codeword = lfsr((uint8_t *) input+iptr, 15, 10, fecgen);
                
                // compare codeword to the 5 received bits
                difference = 0;
                for(count=0;count<5;count++)
                        if(codeword[count]!=input[iptr+10+count])
                                difference++;
                
                /* no errors or single bit errors (errors in the parity bit):
                 * (a strong hint it's a real packet) */
                if((0==difference) || (1==difference)) {
                    free(codeword);
                    continue;
                }
                
                // multiple different bits in the codeword
                for(count=0;count<5;count++) {
                        difference |= codeword[count] ^ input[iptr+10+count];
                        difference <<= 1;
                }
                free(codeword);
                

                switch (difference) {
                /* comments are the bit that's wrong and the value
                 * of difference in binary, from the BT spec */
                        // 1000000000 11010
                        case 26: output[optr] ^= 1; break;
                        // 0100000000 01101
                        case 13: output[optr+1] ^= 1; break;
                        // 0010000000 11100
                        case 28: output[optr+2] ^= 1; break;
                        // 0001000000 01110
                        case 14: output[optr+3] ^= 1; break;
                        // 0000100000 00111
                        case 7: output[optr+4] ^= 1; break;
                        // 0000010000 11001
                        case 25: output[optr+5] ^= 1; break;
                        // 0000001000 10110
                        case 22: output[optr+6] ^= 1; break;
                        // 0000000100 01011
                        case 11: output[optr+7] ^= 1; break;
                        // 0000000010 11111
                        case 31: output[optr+8] ^= 1; break;
                        // 0000000001 10101
                        case 21: output[optr+9] ^= 1; break;
                        /* not one of these errors, probably multiple bit errors
                         * or maybe not a real packet, safe to drop it? */
                        default: free(output); return NULL;
                }
        }
        return output;
}

static int fhs(BS *bs, uint8_t *stream, int clock, uint8_t UAP, int size)
{
        uint8_t *corrected;
        uint8_t payload[144];
        uint8_t fhsuap;
        uint32_t fhslap;

        /* FHS packets are sent with a UAP of 0x00 in the HEC */
        if(UAP != 0)
                return 0;

        if(size < 225)
                return 1; //FIXME should throw exception

        corrected = unfec23(stream, 144);
        if(NULL == corrected)
                return 0;
        unwhiten(corrected, payload, clock, 144, 18);
        free(corrected);

        fhsuap = air_to_host8(&payload[64], 8);

        fhslap = air_to_host32(&payload[34], 24);

        if((fhsuap == UAP) && (fhslap == bs->bs_current->p_LAP))
                return 1000;
        else
                return 0;
}

/* decode payload header, return value indicates success */
static int decode_payload_header(BS *bs, uint8_t *stream, int clock,
				 int header_bytes, int size, int fec)
{       
        uint8_t *corrected;
        /* payload header, one bit per char */
        /* the packet may have a payload header of 0, 1, or 2 bytes, reserving 2 */
        uint8_t d_payload_header[16];

        if(header_bytes == 2)
        {       
                if(size < 30)
                        return 0; //FIXME should throw exception
                if(fec) {
                        corrected = unfec23(stream, 16);
                        if(NULL == corrected)
                                return 0;
                        unwhiten(corrected, d_payload_header, clock, 16, 18);
                        free(corrected);
                } else {
                        unwhiten(stream, d_payload_header, clock, 16, 18);
                }
                /* payload length is payload body length + 2 bytes payload header + 2 bytes CRC */
                bs->bs_payload_length = air_to_host16(&d_payload_header[3], 10) + 4;
        } else {
                if(size < 8)
                        return 0; //FIXME should throw exception
                if(fec) {
                        corrected = unfec23(stream, 8);
                        if(NULL == corrected)
                                return 0;
                        unwhiten(corrected, d_payload_header, clock, 8, 18);
                        free(corrected);
                } else {
                        unwhiten(stream, d_payload_header, clock, 8, 18);
                }
                /* payload length is payload body length + 1 byte payload header + 2 bytes CRC */
                bs->bs_payload_length = air_to_host8(&d_payload_header[3], 5) + 3;
        }
#if 0
        d_payload_llid = air_to_host8(&d_payload_header[0], 2);
        d_payload_flow = air_to_host8(&d_payload_header[2], 1);
#endif
        return 1;
}

/* Pointer to start of packet, length of packet in bits, UAP */
static uint16_t crcgen(uint8_t *payload, int length, int UAP)
{                
        uint8_t byte;         
        uint16_t reg, count;
                           
        reg = (reverse(UAP) << 8) & 0xff00;
        for(count = 0; count < length; count++)
        {       
                byte = payload[count];
                           
                reg = (reg >> 1) | (((reg & 0x0001) ^ (byte & 0x01))<<15);
                        
                /*Bit 5*/  
                reg ^= ((reg & 0x8000)>>5);
                         
                /*Bit 12*/ 
                reg ^= ((reg & 0x8000)>>12);
        }         
        return reg;
}

static void bytes_to_bits(uint8_t *bytes, uint8_t *bits, size_t bitlen)
{
	for (; bitlen >= 8; bitlen -= 8) {
		*bytes++ = air_to_host8(bits, 8);
		bits += 8;
	}

	assert(bitlen == 0);
}

static void set_payload(BS *bs, uint8_t *data)
{
	assert(bs->bs_payload_length <= sizeof(bs->bs_payload));

	bytes_to_bits(bs->bs_payload, data, bs->bs_payload_length * 8);
}

/* DH 1/3/5 packet */
/* similar to DM 1/3/5 but without FEC */
static int DH(BS *bs, uint8_t *stream, int clock, uint8_t UAP, int type,
	      int size)
{       
        int bitlength;
        uint16_t crc, check;
        /* number of bytes in the payload header */
        int header_bytes = 2;
        /* maximum payload length */
        int max_length;

        switch(type)
        {       
                case(4): /* DH1 */
                        header_bytes = 1;
                        max_length = 30;
                        break;
                case(11): /* DH3 */
                        max_length = 187;
                        break;
                case(15): /* DH5 */
                        max_length = 343;
                        break;
                default: /* not a DH1/3/5 */
                        return 0;
        }
        if(!decode_payload_header(bs, stream, clock, header_bytes, size, 0))
                return 0;
        /* check that the length indicated in the payload header is within spec */
        if(bs->bs_payload_length > max_length)
                return 0;
        bitlength = bs->bs_payload_length*8;
        if(bitlength > size)
                return 1; //FIXME should throw exception

        uint8_t payload[bitlength];
        unwhiten(stream, payload, clock, bitlength, 18);
        crc = crcgen(payload, (bs->bs_payload_length-2)*8, UAP);
        check = air_to_host16(&payload[(bs->bs_payload_length-2)*8], 16);

	set_payload(bs, payload);

        if(crc == check) {
#if 0
                d_payload_crc = crc;
                d_payload_header_length = header_bytes;
#endif
                return 10;
        }

        return 0;
}

static int EV(BS *bs, uint8_t *stream, int clock, uint8_t UAP, int type, int size)
{       
        int count;
        uint16_t crc, check;
        uint8_t *corrected;
        /* EV5 has a maximum of 180 bytes + 2 bytes CRC */
        int maxlength = 182;
        uint8_t payload[maxlength*8];

        switch (type)
        {       
                case 12:/* EV4 */
                        if(size < 1470)
                                return 1; //FIXME should throw exception
                        /* 120 bytes + 2 bytes CRC */
                        maxlength = 122;
                        corrected = unfec23(stream, maxlength * 8);
                        if(NULL == corrected)
                                return 0;
                        unwhiten(corrected, payload, clock, maxlength * 8, 18);
                        free(corrected);
                        break;
                
                case 7:/* EV3 */
                        /* 30 bytes + 2 bytes CRC */
                        maxlength = 32;
                case 13:/* EV5 */
                        if(size < (maxlength * 8))
                                return 1; //FIXME should throw exception
                        unwhiten(stream, payload, clock, maxlength * 8, 18);
                        break;
                
                default:
                        return 0;
        }

        /* Check crc for any integer byte length up to maxlength */
        for(count = 1; count < (maxlength-1); count++)
        {
                crc = crcgen(payload, count*8, UAP);
                check = air_to_host16(&payload[count*8], 16);

                /* Check CRC */
                if(crc == check) {
#if 0
                        d_payload_crc = crc;
                        d_payload_header_length = 0;
                        d_payload = payload;
#endif
                        bs->bs_payload_length = count + 2;
			
			set_payload(bs, payload);

                        return 10;
                }
        }
        return 0;
}

/* DM 1/3/5 packet (and DV)*/
static int DM(BS *bs, uint8_t *stream, int clock, uint8_t UAP, int type, int size)
{       
        int bitlength;
        uint16_t crc, check;
        uint8_t *corrected;
        /* number of bytes in the payload header */
        int header_bytes = 2;
        /* maximum payload length */
        int max_length;

        switch(type)
        {       
                case(8): /* DV */
                        /* skip 80 voice bits, then treat the rest like a DM1 */
                        stream += 80;
                        size -= 80;
                        header_bytes = 1;
                        /* I don't think the length of the voice field ("synchronous data
                         * field") is included in the length indicated by the payload
                         * header in the data field ("asynchronous data field"), but I
                         * could be wrong.
                         */
                        max_length = 12;
                        break;
                case(3): /* DM1 */
                        header_bytes = 1;
                        max_length = 20;
                        break;
                case(10): /* DM3 */
                        max_length = 125;
                        break;
                case(14): /* DM5 */
                        max_length = 228;
                        break;
                default: /* not a DM1/3/5 or DV */
                        return 0;
        }
        if(!decode_payload_header(bs, stream, clock, header_bytes, size, 1))
                return 0;
        /* check that the length indicated in the payload header is within spec */
        if(bs->bs_payload_length > max_length)
                return 0;
        bitlength = bs->bs_payload_length*8;
        if(bitlength > size)
                return 1; //FIXME should throw exception

        uint8_t payload[bitlength];
        corrected = unfec23(stream, bitlength);
        if(NULL == corrected)
                return 0;
        unwhiten(corrected, payload, clock, bitlength, 18);
        free(corrected);
        crc = crcgen(payload, (bs->bs_payload_length-2)*8, UAP);
        check = air_to_host16(&payload[(bs->bs_payload_length-2)*8], 16);

	set_payload(bs, payload);

        if(crc == check) {
#if 0
                d_payload_crc = crc;
                d_payload_header_length = header_bytes;
                d_payload = payload;
#endif
                return 10;
        }
        
        return 0;
}

/* check if the packet's CRC is correct for a given clock (CLK1-6) */
static int crc_check(BS* bs, uint8_t *data, int d_length, int clock)
{      
        /*
         * return value of 1 represents inconclusive result (default)
         * return value > 1 represents positive result (e.g. CRC match)
         * return value of 0 represents negative result (e.g. CRC failure without
         * the possibility that we have assumed the wrong logical transport)
         */
        int retval = 1;
        /* skip the access code and packet header */
        uint8_t *stream = data + 126;
	int d_UAP = bs->bs_UAP;
	int d_packet_type = bs->bs_packet_type;

	d_length -= 126; /* XXX check negative, or leave it to lower layers? */
       
        switch (bs->bs_packet_type) {       
                case 2:/* FHS packets are sent with a UAP of 0x00 in the HEC */
                        retval = fhs(bs, stream, clock, d_UAP, d_length);
                        break;
                
                case 8:/* DV */
                case 3:/* DM1 */
                case 10:/* DM3 */
                case 14:/* DM5 */
                        retval = DM(bs, stream, clock, d_UAP, d_packet_type, d_length);
                        break;
                
                case 4:/* DH1 */
                case 11:/* DH3 */
                case 15:/* DH5 */
                        retval = DH(bs, stream, clock, d_UAP, d_packet_type, d_length);
                        break;
                
                case 7:/* EV3 */
                case 12:/* EV4 */
                case 13:/* EV5 */
                        /* Unknown length, need to cycle through it until CRC matches */
                        retval = EV(bs, stream, clock, d_UAP, d_packet_type, d_length);
                        break;
        }
        /* never return a zero result unless this ia a FHS or DM1 */
        /* any other type could have actually been something else */
        if(retval == 0 && (d_packet_type < 2 || d_packet_type > 3))
                return 1;
        return retval;
}

/* extract UAP by reversing the HEC computation */
static int UAP_from_hec(uint8_t *packet)
{                
	char byte;
	int count;
	uint8_t hec;

	hec = *(packet + 2);
	byte = *(packet + 1);

	for(count = 0; count < 10; count++)
	{
		if(2==count)
			byte = *packet;

		/* 0xa6 is xor'd if LSB is 1, else 0x00 (which does nothing) */
		if(hec & 0x01)
			hec ^= 0xa6;

		hec = (hec >> 1) | (((hec & 0x01) ^ (byte & 0x01)) << 7);
		byte >>= 1;
	}
	return hec;
}

/* try a clock value (CLK1-6) to unwhiten packet header,
 * sets resultant d_packet_type and d_UAP, returns UAP.
 */
static uint8_t try_clock(BS *bs, uint8_t *data, int clock)
{       
        /* skip 72 bit access code */
        uint8_t *stream = data + 72;
        /* 18 bit packet header */
        uint8_t header[18];
        uint8_t unwhitened[18];
        uint8_t unwhitened_air[3]; // more than one bit per byte but in air order

        unfec13(stream, header, 18);
        unwhiten(header, unwhitened, clock, 18, 0);

        unwhitened_air[0] = unwhitened[0] << 7 | unwhitened[1] << 6 | unwhitened[2] << 5 | unwhitened[3] << 4 | unwhitened[4] << 3 | unwhitened[5] << 2 | unwhitened[6] << 1 | unwhitened[7];
        unwhitened_air[1] = unwhitened[8] << 1 | unwhitened[9];
        unwhitened_air[2] = unwhitened[10] << 7 | unwhitened[11] << 6 | unwhitened[12] << 5 | unwhitened[13] << 4 | unwhitened[14] << 3 | unwhitened[15] << 2 | unwhitened[16] << 1 | unwhitened[17];

        bs->bs_UAP = UAP_from_hec(unwhitened_air);
        bs->bs_packet_type = air_to_host8((uint8_t*) &unwhitened[3], 4);
        bs->bs_lower_clock = clock & 0xff;

        return bs->bs_UAP;
}

/* use packet headers to determine UAP */
static int UAP_from_header(BS *bs, uint8_t *data, size_t len, int interval,
			   int channel)
{
        uint8_t UAP;    
        int count, retval, first_clock;
        int crc_match = -1;
        int starting = 0;
        int remaining = 0;
	struct piconet *p = bs->bs_current;
                        
        if(!p->p_got_first_packet)
                interval = 0;
        if(p->p_packets_observed < MAX_PATTERN_LENGTH)
        {       
                p->p_pattern_indices[p->p_packets_observed] = interval + p->p_previous_clock_offset;
                p->p_pattern_channels[p->p_packets_observed] = channel;
        } else
        {       
                printf("Oops. More hops than we can remember.\n");
                return 0; //FIXME ought to throw exception
        }
        p->p_packets_observed++;
        p->p_total_packets_observed++;

        /* try every possible first packet clock value */
        for(count = 0; count < 64; count++)
        {       
                /* skip eliminated candidates unless this is our first time through */
                if(p->p_clock6_candidates[count] > -1 || !p->p_got_first_packet)
                {       
                        /* clock value for the current packet assuming count was the clock of the first packet */            
                        int clock = (count + p->p_previous_clock_offset + interval) % 64;
                        starting++;
                        UAP = try_clock(bs, data, clock);
                        retval = -1;
                        
                        /* if this is the first packet: populate the candidate list */
                        /* if not: check CRCs if UAPs match */
                        if(!p->p_got_first_packet || UAP == p->p_clock6_candidates[count])
                                retval = crc_check(bs, data, len, clock);
                        switch(retval)
                        {       
                                case -1: /* UAP mismatch */
                                case 0: /* CRC failure */
                                        p->p_clock6_candidates[count] = -1;
                                        break;
                                
                                case 1: /* inconclusive result */
                                        p->p_clock6_candidates[count] = UAP;
                                        /* remember this count because it may be the correct clock of the first packet */                    
                                        first_clock = count;
                                        remaining++;
                                        break;
                                
                                default: /* CRC success */
                                        /* It is very likely that this is the correct clock/UAP, but I have seen a false positive */         
                                        printf("Correct CRC! UAP = 0x%x Awaiting confirmation. . .\n", UAP);                                 
                                        p->p_clock6_candidates[count] = UAP;
                                        first_clock = count;
                                        crc_match = count;
                                        break;
                        }
                }
        }
        if(crc_match > -1)
        {       
                /* set things up for one additional packet to confirm */
                remaining = 1;
                /* eliminate all other candidates */
                for(count = 0; count < 64; count++)
                        if(count != crc_match)
                                        p->p_clock6_candidates[count] = -1;
        }
        p->p_previous_clock_offset += interval;
        p->p_got_first_packet = 1;
        printf("reduced from %d to %d CLK1-6 candidates\n", starting, remaining);
        if(0 == remaining)
        {       
                printf("no candidates remaining! starting over . . .\n");
                p->p_got_first_packet = 0;
                p->p_previous_clock_offset = 0;
                p->p_packets_observed = 0;
        } else if(1 == starting && 1 == remaining)
        {       
                /* we only trust this result if two packets in a row agree on the winner */
                printf("We have a winner! UAP = 0x%x found after %d total packets.\n", UAP, p->p_total_packets_observed);
                p->p_clock6 = first_clock;
                p->p_UAP = UAP;
                return 1;
        }
        return 0;
}

/* 5 bit permutation */
/* assumes z is constrained to 5 bits, p_high to 5 bits, p_low to 9 bits */
static int perm5(int z, int p_high, int p_low)
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

/* do all the precalculation that can be done before knowing the address */
static void precalc(BS *bs)
{
	struct piconet *p = bs->bs_current;
        int i;
        int z, p_high, p_low;

        /* populate frequency register bank*/
        for (i = 0; i < CHANNELS; i++)
                        p->p_bank[i] = ((i * 2) % CHANNELS);
        /* actual frequency is 2402 + d_bank[i] MHz */

        /* populate perm_table for all possible inputs */
        for (z = 0; z < 0x20; z++)
                for (p_high = 0; p_high < 0x20; p_high++)
                        for (p_low = 0; p_low < 0x200; p_low++)
                                p->p_perm_table[z][p_high][p_low] = perm5(z, p_high, p_low);
}

/* do precalculation that requires the address */
static void address_precalc(BS *bs, int address)
{
	struct piconet *p = bs->bs_current;

        /* precalculate some of single_hop()/gen_hop()'s variables */
        p->p_a1 = (address >> 23) & 0x1f;
        p->p_b = (address >> 19) & 0x0f;
        p->p_c1 = ((address >> 4) & 0x10) +
                ((address >> 3) & 0x08) +
                ((address >> 2) & 0x04) +
                ((address >> 1) & 0x02) +
                (address & 0x01);
        p->p_d1 = (address >> 10) & 0x1ff;
        p->p_e = ((address >> 7) & 0x40) +
                ((address >> 6) & 0x20) +
                ((address >> 5) & 0x10) +
                ((address >> 4) & 0x08) +
                ((address >> 3) & 0x04) +
                ((address >> 2) & 0x02) +
                ((address >> 1) & 0x01);
}

/* drop-in replacement for perm5() using lookup table */
static int fast_perm(BS *bs, int z, int p_high, int p_low)
{       
        return(bs->bs_current->p_perm_table[z][p_high][p_low]);
}


/* generate the complete hopping sequence */
static void gen_hops(BS *bs)
{      
	struct piconet *p = bs->bs_current;
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
                        a = p->p_a1 ^ i;              
                        for (j = 0; j < 0x20; j++) { /* clock bits 16-20 */
                                c = p->p_c1 ^ j;
                                c_flipped = c ^ 0x1f;
                                for (k = 0; k < 0x200; k++) { /* clock bits 7-15 */
                                        d = p->p_d1 ^ k;
                                        for (x = 0; x < 0x20; x++) { /* clock bits 2-6 */
                                                perm_in = ((x + a) % 32) ^ p->p_b;
                                                /* y1 (clock bit 1) = 0, y2 = 0 */
                                                perm_out = fast_perm(bs, perm_in, c, d);
                                                p->p_sequence[index++] = p->p_bank[(perm_out + p->p_e + f) % CHANNELS];                                       
                                                /* y1 (clock bit 1) = 1, y2 = 32 */
                                                perm_out = fast_perm(bs, perm_in, c_flipped, d);
                                                p->p_sequence[index++] = p->p_bank[(perm_out + p->p_e + f + 32) % CHANNELS];                          
                                        }
                                        f += 16;
                                }
                        }
                }
        }
}

/* create list of initial candidate clock values (hops with same channel as first observed hop) */
static int init_candidates(BS *bs, char channel, int known_clock_bits)
{       
        int i;         
        int count = 0; /* total number of candidates */
        char observable_channel; /* accounts for aliasing if necessary */
	struct piconet *p = bs->bs_current;
                       
        /* only try clock values that match our known bits */
        for (i = known_clock_bits; i < SEQUENCE_LENGTH; i += 0x40) {
                observable_channel = p->p_sequence[i];
                if (observable_channel == channel)
                        p->p_clock_candidates[count++] = i;
                //FIXME ought to throw exception if count gets too big
        }           
        return count;
}

/* initialize the hop reversal process */
static int init_hop_reversal(BS *bs)
{
	struct piconet *p = bs->bs_current;
        int max_candidates;

        max_candidates = (SEQUENCE_LENGTH / CHANNELS) / 32;

        /* this can hold twice the approximate number of initial candidates */
        p->p_clock_candidates = (uint32_t*) malloc(sizeof(uint32_t) * max_candidates);

        /* this holds the entire hopping sequence */
        p->p_sequence = (char*) malloc(SEQUENCE_LENGTH);

        precalc(bs);     
        address_precalc(bs, ((p->p_UAP<<24) | p->p_LAP) & 0xfffffff);
        gen_hops(bs);    
        p->p_num_candidates = init_candidates(bs, p->p_pattern_channels[0], p->p_clock6);
        p->p_winnowed = 0;

        return p->p_num_candidates;
}

/* narrow a list of candidate clock values based on a single observed hop */
static int winnow(struct piconet *p, int offset, char channel)
{       
        int i;
        int new_count = 0; /* number of candidates after winnowing */
        char observable_channel; /* accounts for aliasing if necessary */

        /* check every candidate */
        for (i = 0; i < p->p_num_candidates; i++) {
                observable_channel = p->p_sequence[(p->p_clock_candidates[i] + offset) % SEQUENCE_LENGTH];         
                if (observable_channel == channel) {
                        /* this candidate matches the latest hop */
                        /* blow away old list of candidates with new one */
                        /* safe because new_count can never be greater than i */
                        p->p_clock_candidates[new_count++] = p->p_clock_candidates[i];
                }
        }
        p->p_num_candidates = new_count;
        //FIXME maybe do something if new_count == 1
        //FIXME maybe do something if new_count == 0
        return new_count;
}

/* CLK1-27 */
static uint32_t piconet_clock(struct piconet *p)
{
        /* this is completely bogus unless d_num_candidates == 1 */
	assert(p->p_num_candidates == 1);
        return p->p_clock_candidates[0];
}

/* narrow a list of candidate clock values based on all observed hops */
static int piconet_winnow(struct piconet *p)
{       
        int new_count = p->p_num_candidates;

        for (; p->p_winnowed < p->p_packets_observed; p->p_winnowed++)
                new_count = winnow(p, p->p_pattern_indices[p->p_winnowed], p->p_pattern_channels[p->p_winnowed]);

        return new_count;
}

static void reset_piconet(struct piconet *p)
{
	assert(!"code me");
	abort();
}

static void do_uap_clock(BS *bs, uint8_t *data, size_t len)
{
	struct piconet *p = bs->bs_current;
	int interval, diff;
	int have_uap = p->p_UAP != -1;
	int rc;
	int num_candidates;

	if (p->p_clock != (btclock_t) -1)
		return;

	/* number of samples elapsed since previous packet */
	diff = bs->bs_clock - p->p_last_clock;

	/* number of time slots elapsed since previous packet */
	interval = (diff + 312) / 625;

	rc = UAP_from_header(bs, data, len, interval, bs->bs_rx->rx_chan);
	if (!rc) {
		if (have_uap)
			reset_piconet(p);

		return;
	}

	/* We've done our UAP discovery */
	if (!have_uap) {
		init_hop_reversal(bs);
		notify_event(bs, EVENT_UAP);
		assert(p->p_UAP != -1);
	}

	/* move on to clock discovery */
	num_candidates = piconet_winnow(p);
	printf("%d CLK1-27 candidates remaining\n", num_candidates);

	if (num_candidates == 0)
		reset_piconet(p);
	else if (num_candidates == 1) {
		piconet_set_clock(bs, p, piconet_clock(p));
		notify_event(bs, EVENT_CLOCK);

		assert(p->p_clock != (btclock_t) -1);
	}
}

/* decode the packet header */
static void decode_header(BS *bs, uint8_t *data, size_t len)
{       
	/* skip 72 bit access code */
	uint8_t *stream = data + 72;
        /* 18 bit packet header */
	uint8_t header[18];
	uint8_t unwhitened[18];
	uint8_t unwhitened_air[3]; // more than one bit per byte but in air order
        uint8_t UAP, ltadr;
        int count;
	struct piconet *p = bs->bs_current;

        unfec13(stream, header, 18);

        if (p->p_clock != (btclock_t) -1) {
                //FIXME use try_clock() or otherwise merge functions
                unwhiten(header, unwhitened, p->p_clock, 18, 0);
                
                unwhitened_air[0] = unwhitened[0] << 7 | unwhitened[1] << 6 | unwhitened[2] << 5 | unwhitened[3] << 4 | unwhitened[4] << 3 | unwhitened[5] << 2 | unwhitened[6] << 1 | unwhitened[7];
                unwhitened_air[1] = unwhitened[8] << 1 | unwhitened[9];
                unwhitened_air[2] = unwhitened[10] << 7 | unwhitened[11] << 6 | unwhitened[12] << 5 | unwhitened[13] << 4 | unwhitened[14] << 3 | unwhitened[15] << 2 | unwhitened[16] << 1 | unwhitened[17];             
                
                UAP = UAP_from_hec(unwhitened_air);
                if (UAP != p->p_UAP)
                        printf("bad HEC! ");

		count = p->p_clock & 0xff;
		goto __out;
        }

        /* we don't have the clock, so we try every possible CLK1-6 value until we find the most likely LT_ADDR */
        for(count = 0; count < 64; count++)
        {       
                //FIXME use try_clock() or otherwise merge functions
                unwhiten(header, unwhitened, count, 18, 0);

                unwhitened_air[0] = unwhitened[0] << 7 | unwhitened[1] << 6 | unwhitened[2] << 5 | unwhitened[3] << 4 | unwhitened[4] << 3 | unwhitened[5] << 2 | unwhitened[6] << 1 | unwhitened[7];
                unwhitened_air[1] = unwhitened[8] << 1 | unwhitened[9];
                unwhitened_air[2] = unwhitened[10] << 7 | unwhitened[11] << 6 | unwhitened[12] << 5 | unwhitened[13] << 4 | unwhitened[14] << 3 | unwhitened[15] << 2 | unwhitened[16] << 1 | unwhitened[17];

                UAP = UAP_from_hec(unwhitened_air);

                //FIXME throw exception if !d_have_UAP
                if(UAP != p->p_UAP)
                        continue;

                ltadr = air_to_host8(unwhitened, 3);

                //FIXME assuming that the lt_addr can only be 1
                if(1 != ltadr)
                        continue;

		goto __out;
        }
        //FIXME if we get to here without setting d_packet_type, we are in trouble
	assert(!"fuck");
	abort();

__out:
	bs->bs_packet_type = air_to_host8(&unwhitened[3], 4);
	bs->bs_lower_clock = count & 0xff;

	/* XXX copy header */
	bs->bs_hdr.bh_addr = air_to_host8(unwhitened, 3);
	bs->bs_hdr.bh_type = bs->bs_packet_type;
	bs->bs_hdr.bh_flow = unwhitened[7];
	bs->bs_hdr.bh_arq  = unwhitened[8];
	bs->bs_hdr.bh_seq  = unwhitened[9];
	bs->bs_hdr.bh_hec  = air_to_host8(&unwhitened[10], 8);

	return;
}

/* HV packet type payload parser */
static int HV(BS *bs, uint8_t *stream, int clock, uint8_t UAP, int type, int size)
{                     
        uint8_t *corrected;
        if(size < 240) {
                bs->bs_payload_length = 0;
                return 1; //FIXME should throw exception
        }
               
        switch (type)
        {             
                case 5:/* HV1 */
                        corrected = malloc(80);
                        unfec13(stream, corrected, 240);
                        if(NULL == corrected)
                                return 0;
                        bs->bs_payload_length = 10;
                        break;
                      
                case 6:/* HV2 */
                        corrected = unfec23(stream, 240);
                        if(NULL == corrected)
                                return 0;
                        bs->bs_payload_length = 20;
                        break;
                case 7:/* HV3 */
                        bs->bs_payload_length = 30;
                        corrected = stream;
                        break;
        }
        uint8_t payload[bs->bs_payload_length*8];
        unwhiten(corrected, payload, clock, bs->bs_payload_length*8, 18);

	set_payload(bs, payload);

        return 0;
}

static void decode_payload(BS *bs, uint8_t *data, size_t len)
{                      
        int size, clock;
	int d_UAP = bs->bs_current->p_UAP;
	int d_packet_type = bs->bs_packet_type;

	assert(d_UAP != -1);                      
 
        /* number of symbols remaining after access code and packet header */
        size = len - 126;
        clock = bs->bs_lower_clock;

        uint8_t *stream = data + 126; // AC + header

	bs->bs_payload_length = 0;

        switch(bs->bs_packet_type)
        {              
                case 0: /* NULL */
                        /* no payload to decode */
                        break;
                case 1: /* POLL */
                        /* no payload to decode */
                        break;
                case 2: /* FHS */
			/* XXX these are handled badly.  UAP has to be 0?  Also,
			 * what's their length?
			 */
                        fhs(bs, stream, clock, d_UAP, size);
                        break;
                case 3: /* DM1 */
                        DM(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 4: /* DH1 */
                        /* assuming DH1 but could be 2-DH1 */
                        DH(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 5: /* HV1 */
                        HV(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 6: /* HV2 */
                        HV(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 7: /* EV3 */
                        /* assuming EV3 but could be HV3 or 3-EV3 */
                        if(!EV(bs, stream, clock, d_UAP, d_packet_type, size)) {
                                HV(bs, stream, clock, d_UAP, d_packet_type, size);
                        }
                        break;
                case 8: /* DV */
                        /* assuming DV but could be 3-DH1 */
                        DM(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 9: /* AUX1 */
                        /* don't know how to decode */
                        break;
                case 10: /* DM3 */
                        /* assuming DM3 but could be 2-DH3 */
                        DM(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 11: /* DH3 */
                        /* assuming DH3 but could be 3-DH3 */
                        DH(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 12: /* EV4 */
                        /* assuming EV4 but could be 2-EV5 */
                        EV(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 13: /* EV5 */
                        /* assuming EV5 but could be 3-EV5 */
                        EV(bs, stream, clock, d_UAP, d_packet_type, size);
                case 14: /* DM5 */
                        /* assuming DM5 but could be 2-DH5 */
                        DM(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
                case 15: /* DH5 */
                        /* assuming DH5 but could be 3-DH5 */
                        DH(bs, stream, clock, d_UAP, d_packet_type, size);
                        break;
        }
}

static uint8_t *process_packet(BS *bs, uint8_t *data, size_t len)
{
	int plen = AC_LEN + 54;
	struct piconet *p = bs->bs_current;

	if (len < plen) {
		printf("Truncated packet\n");

		return data + len;
	}

	do_uap_clock(bs, data, len);

	/* XXX assumption: if we don't know the UAP, we can't decode the packet
	 */
	if (p->p_UAP == -1) {
		/* XXX */
		printf("XXX queue packet - decode later\n");
		return data + plen;
	}

	/* If we have the UAP, we can decode the packet.  We don't need the full
	 * clock - that's only needed for hopping, though we're not responsible
	 * for that.  If we have the clock though, it makes our lives easier.
	 */

	/* XXX is this correct? */
	if (p->p_clock != (btclock_t) -1) {
		p->p_clock = (bs->bs_clock + p->p_clock_offset)
			     % SEQUENCE_LENGTH;
	}

	/* XXX avoid decoding headers & payloads twice */

	decode_header(bs, data, len);
	decode_payload(bs, data, len);

	notify_event(bs, EVENT_PACKET);

	return data + plen + bs->bs_payload_length;
}

BS *bs_new(evcb_t cb, void *priv)
{
	BS *bs = malloc(sizeof(*bs));

	if (!bs)
		return NULL;

	memset(bs, 0, sizeof(*bs));

	bs->bs_ev_cb   = cb;
	bs->bs_ev_priv = priv;

	return bs;
}

void bs_delete(BS *bs)
{
	struct piconet *p;

	assert(bs);

	p = bs->bs_piconets.p_next;
	while (p) {
		struct piconet *tmp = p->p_next;

		free(p);
		p = tmp;
	}

	free(bs);
}

static void advance_clock(BS *bs, size_t did)
{
	bs->bs_clock += did;
	assert(bs->bs_clock >= 0);
}

static size_t do_process(BS *bs, uint8_t *data, size_t len)
{
	size_t did, total;
	uint8_t *p;
	struct piconet *pi;

	/* find a packet */
	p = find_packet(bs, data, len);
	if (!p) {
		advance_clock(bs, len);
		return len;
	}

	/* fixup clock */
	total = did = p - data;
	advance_clock(bs, did);

	assert(did >= 0 && did <= len);

	pi = bs->bs_current;

	if (!pi->p_last_clock)
		pi->p_last_clock = bs->bs_clock;

	/* process data */
	data   = p;
	p      = process_packet(bs, data, len - did);
	did    = p - data;
	total += did;

	/* fixup clock */
	pi->p_last_clock = bs->bs_clock;
	advance_clock(bs, did);

	return total;
}

int bs_process(BS *bs, RXINFO *rx, void *data, size_t len)
{
	uint8_t *p = data;
	size_t did;

	bs->bs_rx = rx;

	/* XXX can be called multiple times, when sniffing all chans */
	assert(rx->rx_clock >= bs->bs_clock); /* XXX handle wrap */
	bs->bs_clock = rx->rx_clock;

	/* XXX buffer data so we capture shit across call boundaries */

	while (len) {
		did = do_process(bs, p, len);

		assert(did <= len && did >= 0);

		len -= did;
		p   += did;

		/* XXX interrupt processing as necessary */
	}

	return p - (uint8_t*) data;
}
