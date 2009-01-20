// Taken from the gssm project (http://www.thre.at/gsm)

#pragma once

#include <linux/if_ether.h>
#include <gr_sync_block.h>
#include <gri_mmse_fir_interpolator.h>
// #include "gssm_state.h"

class bluetooth_tun;
typedef boost::shared_ptr<bluetooth_tun> bluetooth_tun_sptr;
bluetooth_tun_sptr bluetooth_make_tun(double);

class bluetooth_tun : public gr_sync_block {

public:
	~bluetooth_tun(void);

	int work(int nitems, gr_vector_const_void_star &input_items,
	   gr_vector_void_star &output_items);

private:
	// sample speeds
	double			d_sps;		// samples per second
	double			d_samples_per_symbol;

	// M&M clock recovery
	gri_mmse_fir_interpolator *d_interp;
	double			d_mu;
	double			d_gain_mu;
	double			d_omega;
	double			d_gain_omega;
	double			d_omega_relative_limit;
	double			d_max_omega;
	double			d_min_omega;
	float			d_last_sample;

	double			d_mu_bak;
	double			d_omega_bak;
	double			d_last_sample_bak;

	int			d_bitno;

	// buffers
	gr_buffer_sptr		d_buf_qd;
	gr_buffer_reader_sptr	d_qd_reader;
	gr_buffer_sptr		d_buf_mm;
	gr_buffer_reader_sptr	d_mm_reader;

	// quad demod
	gr_complex		d_qd_last;
	double			d_qd_gain;

	// GSM BTS timing
	int			d_tn;		// time slot
	int			d_fn;		// frame number
	int			d_fnm51;	// frame number mod 51
	int			d_fnm102;	// frame number mod 102
	int			d_bsic;		// current bsic

	// program state
	gssm_state_t		d_state;

	// buffer to hold physical data
	unsigned char *		d_phy_buf;
	int *			d_phy_ind;

	// Wireshark interface
	int			d_tunfd;	// TUN fd
	unsigned char		d_ether_addr[ETH_ALEN];

	/*******************************************************************/
	
	friend bluetooth_tun_sptr bluetooth_make_tun(double);
	bluetooth_tun(double);

	int search_state_fc(const float *, int);
	int search_state_s(const float *, int);
	int search_state_data(const float *, int);

	void search_sch(const unsigned char *);
	int handle_sch(const unsigned char *, int *, int *);

	void next_timeslot(void);

	int check_logical_channel(int, int, int, int);
	void check_logical_channels(void);

	int mm_demod(const float *, int, float *, int &);
	int quad_demod(const gr_complex *, int, float *, int &);
	int process_input(const gr_complex *, int);
	int process_qd(const gr_complex *, int);

	void save_clock();
	void restore_clock();
	void reset_clock();
	void flush_buffers();
	void reset_state();


	/*******************************************************************/
	// Debug

	int check_num_invalid_s();

public:
	int			d_search_fc_count,
				d_found_fc_count,
				d_valid_s,
				d_invalid_s,
				d_invalid_s_1,
				d_valid_bcch,
				d_invalid_bcch,
				d_valid_ia,
				d_invalid_ia,
				d_valid_sdcch4,
				d_invalid_sdcch4,
				d_valid_sacchc4,
				d_invalid_sacchc4,
				d_valid_sdcch8,
				d_invalid_sdcch8,
				d_valid_sacchc8,
				d_invalid_sacchc8;
	void stats();
};
