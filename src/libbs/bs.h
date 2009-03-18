#ifndef __LIBBS_BS_H__
#define __LIBBS_BS_H__

#ifdef  __cplusplus
extern "C" {
#endif

#define BT_BADCRC	0x00000001

/* XXX when to notify about hopping sequence? */
enum {
	EVENT_LAP	= 1,	/* first observation of a new LAP */
	EVENT_UAP,
	EVENT_CLOCK,
	EVENT_PACKET, /* XXX send past packets too */
};

/* XXX endiannes */
struct bthdr {
	uint32_t	bh_hec:8,
			bh_seq:1,
			bh_arq:1,
			bh_flow:1,
			bh_type:4,
			bh_addr:3;
} __attribute__ ((__packed__));

typedef uint64_t btclock_t;

struct btevent {
	uint8_t		be_type;
	uint8_t		be_uap;
	uint8_t		be_lap[3];
	btclock_t	be_rclock; /* piconet CLK1-27, counts time slots (1600/s) */
	btclock_t	be_lclock; /* local clock, counts microseconds */
	uint32_t	be_flags;
	struct bthdr	be_hdr;
	uint32_t	be_len;
	uint8_t		*be_data;  /* payload */
	uint8_t		*be_raw;   /* raw bitstream */
	uint32_t	be_rlen;
	void		*be_priv;
};

/* -1 if unknown */
struct piconet_info {
	uint8_t		pi_lap[3];
	int		pi_uap;
	btclock_t	pi_clock; /* piconet CLK1-27, counts time slots (1600/s) */
};

typedef struct rxinfo_t {
	int		rx_chan;
	btclock_t	rx_clock; /* local clock, counts microseconds */
	uint32_t	rx_flags;
} RXINFO;

typedef struct bs_t BS;

typedef int (*evcb_t)(struct btevent *be);	/* event callback */

BS	*bs_new(evcb_t cb, void *priv);
void	bs_delete(BS *bs);
int	bs_process(BS *bs, RXINFO *rx, void *data, size_t len);
void	bs_set_piconet(BS *bs, struct piconet_info *pi);

#ifdef  __cplusplus
}
#endif

#endif /* __LIBBS_BS_H__ */
