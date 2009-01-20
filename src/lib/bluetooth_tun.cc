/*
 * Taken from the gssm project (http://www.thre.at/gsm)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <exception>
#include <stdexcept>

#include <gr_io_signature.h>
#include <gr_buffer.h>
#include <gr_math.h>

#include "bluetooth_tun.h"
// #include "gsm_constants.h"
// #include "burst_types.h"
// #include "bursts.h"
// #include "sch.h"
// #include "cch.h"
// #include "gssm_state.h"
 #include "tun.h"
// #include "rr_decode.h"
// #include "display.h"
// #include "buffer.h"


bluetooth_tun_sptr bluetooth_make_tun(double sps) {
	
	return bluetooth_tun_sptr(new bluetooth_tun(sps));
}


static const char *chan_name = "gr-bluetooth";	// Bluetooth TUN interface name


bluetooth_tun::bluetooth_tun(double sps) :
   gr_sync_block("bluetooth_tun",
      gr_make_io_signature(1, 1, sizeof(gr_complex)),
      gr_make_io_signature(0, 0, 0)) {
	
	if((d_tunfd = mktun(chan_name, d_ether_addr)) == -1) {
		fprintf(stderr, "warning: was not able to open TUN device, "
		   "disabling Wireshark interface\n");
		// throw std::runtime_error("cannot open TUN device");
	}

	// allocate memory for physical channel to logical channel mapping
	if(!(d_phy_buf = (unsigned char *)malloc(8 * 51 * eBLOCK_SIZE))) {
		perror("malloc");
		close(d_tunfd);
		throw std::runtime_error("cannot allocate buffer memory");
	}

	// allocate memory to specify which physical data is present
	if(!(d_phy_ind = (int *)malloc(8 * 51 * sizeof(int)))) {
		perror("malloc");
		close(d_tunfd);
		throw std::runtime_error("cannot allocate index memory");
	}
	memset(d_phy_ind, 0, 8 * 51 * sizeof(int));

	// buffers to hold quadrature demod and clock recovery output
	d_buf_qd = gr_make_buffer(BUFFER_QD_SIZE, sizeof(float));
	d_qd_reader = gr_buffer_add_reader(d_buf_qd, 0);
	d_buf_mm = gr_make_buffer(BUFFER_MM_SIZE, sizeof(float));
	d_mm_reader = gr_buffer_add_reader(d_buf_mm, 0);

	// indicate to clock recovery that we are not sync'ed
	d_bitno = -1;

	// set samples per second and symbol
	d_sps = sps;
	d_samples_per_symbol = d_sps / GSM_RATE;

	// initial program state
	d_state = state_fc;

	// initial base station synchronization
	d_tn = -1;
	d_fn = -1;
	d_bsic = -1;

	// stats
	d_search_fc_count = d_found_fc_count = d_valid_s = d_invalid_s =
	   d_invalid_s_1 = d_valid_bcch = d_invalid_bcch = d_valid_ia =
	   d_invalid_ia = d_valid_sdcch4 = d_invalid_sdcch4 =
	   d_valid_sacchc4 = d_invalid_sacchc4 = d_valid_sdcch8 =
	   d_invalid_sdcch8 = d_valid_sacchc8 = d_invalid_sacchc8 = 0;

	// interpolator
	d_interp = new gri_mmse_fir_interpolator();

	// M&M clock recovery
	reset_clock();

	// set quad demod constants
	d_qd_last = 0;
	d_qd_gain = M_PI_2l / d_samples_per_symbol; // rate at quad demod

	// we always want multiples of the burst length
	set_output_multiple(
	   (int)ceil((BURST_LENGTH + 1) * d_samples_per_symbol)
	   + d_interp->ntaps() + 10);
}


bluetooth_tun::~bluetooth_tun() {

	// close TUN interface
	close(d_tunfd);

	// free phy memory
	free(d_phy_buf);
	free(d_phy_ind);
}


int bluetooth_tun::check_logical_channel(int b1, int b2, int b3, int b4) {

	int r = 0;
	unsigned int data_len;
	unsigned char *data;

	if(d_phy_ind[b1] && d_phy_ind[b2] && d_phy_ind[b3] &&
	   d_phy_ind[b4]) {
		// channel data present
		data = decode_cch(
		   d_phy_buf + b1 * eBLOCK_SIZE,
		   d_phy_buf + b2 * eBLOCK_SIZE,
		   d_phy_buf + b3 * eBLOCK_SIZE,
		   d_phy_buf + b4 * eBLOCK_SIZE,
		   &data_len);
		if(data) {
			//TODO This writes data out to the interface
			write_interface(d_tunfd, data + 1, data_len - 1,
			   d_ether_addr);

			// since we decoded this data, clear present flag
			d_phy_ind[b1] = d_phy_ind[b2] = d_phy_ind[b3] =
			   d_phy_ind[b4] = 0;

			free(data);

			return 0;
		}
		return -1;
	}

	return -2; // channel data not present
}


void bluetooth_tun::check_logical_channels() {

	int o, o_s, ts, rl, fn_s;

	// do we have complete data for various logical channels?

	// BCCH Norm
	rl = 51;
	fn_s = 2;
	for(ts = 0; ts < 8; ts += 2) {
		o = ts * rl + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: BCCH Norm (%d, %d)\n", ts,
			   fn_s);
	}

	// BCCH Ext
	rl = 51;
	fn_s = 6;
	for(ts = 0; ts < 8; ts += 2) {
		o = ts * rl + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: BCCH Ext (%d, %d)\n", ts,
			   fn_s);
	}

	// PCH, AGCH, SDCCH, SACCHC4
	rl = 51;
	for(ts = 0; ts < 8; ts += 2) {
		o_s = ts * rl;

		fn_s = 6;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);

		fn_s = 12;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);

		fn_s = 16;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);

		fn_s = 22;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);

		fn_s = 26;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);

		fn_s = 32;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);

		fn_s = 36;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);

		fn_s = 42;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);

		fn_s = 46;
		o = o_s + fn_s;
		if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
			fprintf(stderr, "error: PCH, AGCH (%d, %d)\n", ts,
			   fn_s);
	}

	// SDCCH8
	rl = 51;
	for(ts = 0; ts < 8; ts++) {
		o_s = ts * rl;
		for(fn_s = 0; fn_s < 32; fn_s += 4) {
			o = o_s + fn_s;
			if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
				fprintf(stderr, "error: SDCCH8 (%d, %d)\n",
				   ts, fn_s);
		}
	}

	// SACCHC8
	// XXX ignoring subchannel numbers
	rl = 51;
	for(ts = 0; ts < 8; ts++) {
		o_s = ts * rl;
		for(fn_s = 32; fn_s < 48; fn_s += 4) {
			o = o_s + fn_s;
			if(check_logical_channel(o, o + 1, o + 2, o + 3) == -1)
				fprintf(stderr, "error: SACCH8 (%d, %d)\n",
				   ts, fn_s);
		}
	}

}


void bluetooth_tun::next_timeslot() {

	d_tn++;
	if(d_tn >= 8) {
		d_tn %= 8;
		d_fn = (d_fn + 1) % MAX_FN;
		d_fnm51 = d_fn % 51;
		d_fnm102 = d_fn % 102;
	}
}


static void differential_decode(const unsigned char *in, unsigned char *out) {

	int i;
	unsigned char is = 1;

	for(i = 0; i < BURST_LENGTH; i++)
		is = out[i] = !in[i] ^ is;
}


static void differential_decode(const float *in, unsigned char *out) {

	int i;
	unsigned char is = 1;

	for(i = 0; i < BURST_LENGTH; i++)
		is = out[i] = (in[i] < 0) ^ is;
}


static const unsigned int MAX_INVALID_S = 10;
//static const unsigned int MAX_INVALID_S = 1;

int bluetooth_tun::check_num_invalid_s() {

	if(d_invalid_s - d_invalid_s_1 >= MAX_INVALID_S) {
		d_invalid_s_1 = d_invalid_s;
		memset(d_phy_ind, 0, 8 * 51 * sizeof(int));

		return 1;
	}
	return 0;
}


int bluetooth_tun::handle_sch(const unsigned char *buf, int *fn, int *bsic) {

	int ret;

	if(ret = !decode_sch(buf, fn, bsic)) {
		d_valid_s++;
		d_invalid_s_1 = d_invalid_s;
	} else {
		d_invalid_s++;
	}
	return ret;
}


/*
 * search_s
 *
 * 	Searches for the synchronization packet.  We assume that we have
 * 	just seen the frequency correction packet.  Hence:
 *
 * 		1.  We first enter with tn = 1.
 * 		2.  The sync burst is the next burst in this channel.
 */
int bluetooth_tun::search_state_s(const float *in, int nitems) {

	int i, imax, wl;
	unsigned char buf[BURST_LENGTH];
	float fbuf[BURST_LENGTH];

	// assume that we are at the start of a burst

	imax = nitems - (int)ceil((BURST_LENGTH + 1) * d_samples_per_symbol) -
	   d_interp->ntaps() - (int)d_mu;

	for(i = 0; i < imax;) {
		wl = BURST_LENGTH;
		i += mm_demod(in + i, nitems - i, fbuf, wl);
		if(!d_tn) {
			differential_decode(fbuf, buf);
			if(handle_sch(buf, &d_fn, &d_bsic)) {

				d_fnm51 = d_fn % 51;
				d_fnm102 = d_fn % 102;

				next_timeslot();

				d_state = state_data;

				return i;
			} else if(check_num_invalid_s()) {
				reset_state();
				return 0;
			}
		}
		next_timeslot();
	}
	return i;
}


static int is_sch(int tn, int fnm51) {

	return ((!tn) && ((fnm51 == 1) || (fnm51 == 11) || (fnm51 == 21) ||
	   (fnm51 == 31) || (fnm51 == 41)));
}


void bluetooth_tun::search_sch(const unsigned char *buf) {

	int fn, bsic;

	if(is_sch(d_tn, d_fnm51)) {
		if(handle_sch(buf, &fn, &bsic)) {
			if(d_fn != fn) {
				fprintf(stderr, "error: lost sync "
				   "(%d != %d [%d])\n", fn, d_fn, fn - d_fn);
				d_fn = fn;
				d_fnm51 = fn % 51;
				d_fnm102 = fn % 102;
			}
			if(d_bsic != bsic) {
				fprintf(stderr, "warning: bsic changed "
				   "(%d -> %d)\n", d_bsic, bsic);
				d_bsic = bsic;
			}

		} else {
			if(check_num_invalid_s()) {
				reset_state();
			}
		}
	}
}


int bluetooth_tun::search_state_data(const float *in, int nitems) {

	int i, imax, wl;
	unsigned int offset, offset_i;
	unsigned char buf[BURST_LENGTH];
	float fbuf[BURST_LENGTH];

	imax = nitems - (int)ceil((BURST_LENGTH + 1) * d_samples_per_symbol) -
	   d_interp->ntaps() - (int)d_mu;

	for(i = 0; i < imax;) {
		wl = BURST_LENGTH;
		i += mm_demod(in + i, nitems - i, fbuf, wl);

		differential_decode(fbuf, buf);

		/*
		for(int ii = 0; ii < BURST_LENGTH; ii++)
			printf("%f\n", fbuf[ii]);
		 */

		/*
		if(is_sch(d_tn, d_fnm51))
			printf("S");
		else
			printf(" ");
		printf("%2d:%d: ", d_fnm51, d_tn);
		for(int ii = 0; ii < BURST_LENGTH; ii++)
			printf("%d", buf[ii]);
		printf("\n");
		 */

		/*
		if((buf[0] == 1) || (buf[1] == 1) || (buf[2] == 1))
			printf("%2d:%d: %d%d%d:%d%d%d %f %f %f : %f %f %f\n",
			   d_fnm51, d_tn, buf[0], buf[1], buf[2], buf[145],
			   buf[146], buf[147], fbuf[0], fbuf[1], fbuf[2],
			   fbuf[145], fbuf[146], fbuf[147]);
		 */

		search_sch(buf);
		if(d_state != state_data)
			return 0;

		if(!is_dummy_burst(buf)) {
			offset_i = (d_tn * 51 + (d_fn % 51));
			offset = offset_i * eBLOCK_SIZE;
			memcpy(d_phy_buf + offset, buf + N_EDATA_OS_1,
			   N_EDATA_LEN_1);
			memcpy(d_phy_buf + offset + N_EDATA_LEN_1,
			   buf + N_EDATA_OS_2, N_EDATA_LEN_2);
			d_phy_ind[offset_i] = 1;
		}

		// check_logical_channels();

		if((!d_tn) && (!d_fnm51)) {
			check_logical_channels();
			memset(d_phy_ind, 0, 8 * 51 * sizeof(int));
		}

		next_timeslot();
	}
	return i;
}


static int slice(float f) {

	return (f >= 0);
}


int bluetooth_tun::mm_demod(const float *in, int nitems, float *out, int &nitems_out) {
	int i, o;
	float mm_val, v, f;

	f = floor(d_mu);
	i = (int)f;
	d_mu -= f;

	for(o = 0; (i < nitems - d_interp->ntaps()) && (o < nitems_out); o++) {
		/*
		 * Produce output sample interpolated by d_mu where d_mu
		 * represents the normalized distance between the first and
		 * second sample of the given sequence.
		 *
		 * For example, d_mu = 0.5 interpolates a sample halfway
		 * in-between the current sample and the next.
		 */
		v = d_interp->interpolate(&in[i], d_mu);

		// adjust how fast and in which direction we modify omega 
		mm_val = slice(d_last_sample) * v - slice(v) * d_last_sample;

		// write output
		// out[o] = (d_last_sample = v) >= 0;	// hard decision
		out[o] = d_last_sample = v;		// soft decision

		// adjust the sample time for the next symbol
		d_omega = d_omega + d_gain_omega * mm_val;

		// don't allow it to exceed extrema
		if(d_omega > d_max_omega)
			d_omega = d_max_omega;
		else if(d_omega < d_min_omega)
			d_omega = d_min_omega;

		// advance to next symbol
		d_mu += d_omega + d_gain_mu * mm_val;

		// if we're sync'ed
		if(d_bitno > -1) {
			d_bitno++;

			// skip the quarter-bit at the end of a burst
			if(d_bitno >= BURST_LENGTH) {
				d_mu += d_omega / 4.0l;
				d_bitno = 0;
			}
		}

		// d_mu is now the number of samples to the next symbol
		f = floor(d_mu);
		i += (int)f;	// integer advance
		d_mu -= f;	// fractional advance
	}

	// we may have advanced past the buffer
	i -= (int)f;
	d_mu += f;

	nitems_out = o;

	return i; // returns number consumed
}


int bluetooth_tun::quad_demod(const gr_complex *in, int nitems, float *out, int &nitems_out) {

	int r, M;
	gr_complex product;

	M = std::min(nitems, nitems_out);
	for(r = 0; r < M; r++) {
		product = in[r] * conj(d_qd_last); 
		d_qd_last = in[r];
		out[r] = d_qd_gain * gr_fast_atan2f(imag(product), real(product));
	}

	nitems_out = r;
	return r; // returns number consumed
}


int bluetooth_tun::process_input(const gr_complex *in, const int nitems) {

	float *w;
	const float *r;
	int wl, rl, rrl, ret;

	w = (float *)d_buf_qd->write_pointer();
	wl = d_buf_qd->space_available();
	ret = quad_demod(in, nitems, w, wl);
	d_buf_qd->update_write_pointer(wl);

	r = (const float *)d_qd_reader->read_pointer();
	rl = d_qd_reader->items_available();
	w = (float *)d_buf_mm->write_pointer();
	wl = d_buf_mm->space_available();
	rrl = mm_demod(r, rl, w, wl);
	d_qd_reader->update_read_pointer(rrl);
	d_buf_mm->update_write_pointer(wl);

	return ret;
}


void bluetooth_tun::flush_buffers() {

	d_qd_reader->update_read_pointer(d_qd_reader->items_available());
	d_mm_reader->update_read_pointer(d_mm_reader->items_available());
}


void bluetooth_tun::save_clock() {

	d_mu_bak = d_mu;
	d_omega_bak = d_omega;
	d_last_sample_bak = d_last_sample;
}


void bluetooth_tun::restore_clock() {

	d_mu = d_mu_bak;
	d_omega = d_omega_bak;
	d_last_sample = d_last_sample_bak;
}


void bluetooth_tun::reset_state() {

	d_state = state_fc;
	d_bitno = -1;
	flush_buffers();
	d_tn = -1;
	d_fn = -1;
	d_fnm51 = -1;
	d_fnm102 = -1;
	d_bsic = -1;

	reset_clock();
}


void bluetooth_tun::reset_clock() {

	// M&M clock recovery
	d_mu = 0.0;
	d_gain_mu = 0.01;
	d_omega = d_samples_per_symbol;
	//d_omega_relative_limit = 0.01;
	d_omega_relative_limit = 0.3;
	d_max_omega = d_omega * (1.0 + d_omega_relative_limit);
	d_min_omega = d_omega * (1.0 - d_omega_relative_limit);
	d_gain_omega = 0.25 * d_gain_mu * d_gain_mu;
}


/*
 * search_fc
 *
 * 	Searches for the frequency correction burst.
 */
int bluetooth_tun::search_state_fc(const float *in, int nitems) {

	int i, j, slen = sizeof(fc_fb_de) / sizeof(*fc_fb_de),
	   imax, c, nbits;
	float w[BURST_LENGTH];
	double f;
	const unsigned char *s = fc_fb_de;

	f = floor(d_mu);
	i = (int)f;
	d_mu -= f;

	imax = nitems - (int)ceil((BURST_LENGTH + 1) * d_samples_per_symbol) -
	   d_interp->ntaps() - i;

	while(i < imax) {

		// update stats
		d_search_fc_count++;

		// save current clock parameters
		save_clock();

		// calculate burst
		nbits = BURST_LENGTH;
		c = mm_demod(in + i, nitems - i, w, nbits);

		// compare fixed and tail bits
		for(j = 0; j < std::min(slen, nbits); j++)
			if(slice(w[j]) != s[j])
				break;

		// was the fc burst detected?
		if(j == slen) {

			// update stats
			d_found_fc_count++;

			d_mu += d_omega / 4.0l;	// advance quarter-bit
			d_bitno = 0;		// burst sync'ed now
			d_tn = 0;		// fc always in timeslot 0
			next_timeslot();
			d_state = state_s;

			return i + c;
		}

		// restore clock to start of attempt
		restore_clock();

		// skip 1 bit and start again
		nbits = 1;
		i += mm_demod(in + i, nitems - i, w, nbits);
	}

	return i;
}


void bluetooth_tun::stats() {

	printf("search fc:\t%d\n", d_search_fc_count);
	printf("found fc:\t%d\n", d_found_fc_count);
	printf("valid s:\t%d\n", d_valid_s);
	printf("invalid s:\t%d\n", d_invalid_s);
}


int bluetooth_tun::process_qd(const gr_complex *in, int nitems) {

	float *w;
	int wl, ret;

	w = (float *)d_buf_qd->write_pointer();
	wl = d_buf_qd->space_available();
	ret = quad_demod(in, nitems, w, wl);
	d_buf_qd->update_write_pointer(wl);

	return ret;
}


int bluetooth_tun::work(int nitems, gr_vector_const_void_star &input_items,
  gr_vector_void_star &) {

	const gr_complex *in = (gr_complex *)input_items[0];
	const float *r;
	int rl, ret, i, imax, n;

	ret = process_qd(in, nitems);

	r = (const float *)d_qd_reader->read_pointer();
	rl = d_qd_reader->items_available();

	imax = rl -
	   (int)(ceil((BURST_LENGTH + 1) * d_samples_per_symbol + d_mu) +
	   d_interp->ntaps());

	for(i = 0; i < imax;) {
		switch(d_state) {
			case state_fc:
				n = search_state_fc(r + i, rl - i);
				break;
			case state_s:
				n = search_state_s(r + i, rl - i);
				break;
			case state_data:
				n = search_state_data(r + i, rl - i);
				break;
			default:
				fprintf(stderr, "error: bad state\n");
				reset_state();
				return 0;
		}
		i += n;
	}
	d_qd_reader->update_read_pointer(i);

	return ret;
}
