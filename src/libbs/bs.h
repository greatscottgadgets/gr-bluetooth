#ifndef __LIBBS_BS_H__
#define __LIBBS_BS_H__

#ifdef  __cplusplus
extern "C" {
#endif

#define BT_BADCRC	0x00000001

/* XXX when to notify about hopping sequence? */
enum {
	EVENT_LAP	= 1,
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

struct btevent {
	uint8_t		be_type;
	uint8_t		be_uap;
	uint8_t		be_lap[3];
	uint32_t	be_rclock; /* piconet clock */
	uint32_t	be_lclock; /* local clock */
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
	uint8_t	pi_lap[3];
	int	pi_uap;
	int	pi_clock;
};

typedef struct rxinfo_t {
	int		rx_chan;
	int		rx_clock;
	uint32_t	rx_flags;
} RXINFO;

typedef struct bs_t BS;

typedef int (*evcb_t)(struct btevent *be);

BS	*bs_new(evcb_t cb, void *priv);
void	bs_delete(BS *bs);
int	bs_process(BS *bs, RXINFO *rx, void *data, size_t len);
void	bs_set_piconet(BS *bs, struct piconet_info *pi);

#ifdef  __cplusplus
}
#endif

#endif /* __LIBBS_BS_H__ */
