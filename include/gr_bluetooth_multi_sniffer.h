/* -*- c++ -*- */
/*
 * Copyright 2008, 2009 Dominic Spill, Michael Ossmann
 * Copyright 2007 Dominic Spill
 * Copyright 2005, 2006 Free Software Foundation, Inc.
 * 
 * This file is part of gr-bluetooth
 * 
 * gr-bluetooth is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * gr-bluetooth is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with gr-bluetooth; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_GR_BLUETOOTH_MULTI_SNIFFER_H
#define INCLUDED_GR_BLUETOOTH_MULTI_SNIFFER_H

#include "gr_bluetooth_multi_block.h"
#include "gr_bluetooth_piconet.h"
#include "tun.h"
#include <map>

class gr_bluetooth_multi_sniffer;
typedef boost::shared_ptr<gr_bluetooth_multi_sniffer> gr_bluetooth_multi_sniffer_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of gr_bluetooth_multi_sniffer.
 */
gr_bluetooth_multi_sniffer_sptr gr_bluetooth_make_multi_sniffer(double sample_rate,
		double center_freq, double squelch_threshold, bool tun);

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class gr_bluetooth_multi_sniffer : public gr_bluetooth_multi_block
{
private:
	// The friend declaration allows gr_bluetooth_make_multi_sniffer to
	// access the private constructor.
	friend gr_bluetooth_multi_sniffer_sptr gr_bluetooth_make_multi_sniffer(
			double sample_rate, double center_freq,
			double squelch_threshold, bool tun);

	/* constructor */
	gr_bluetooth_multi_sniffer(double sample_rate, double center_freq,
			double squelch_threshold, bool tun);

	/* General Inquiry and Limited Inquiry Access Codes */
	static const uint32_t GIAC = 0x9E8B33;
	static const uint32_t LIAC = 0x9E8B00;

	/* Using tun for output */
	bool d_tun;

	/* Tun stuff */
	int d_tunfd;
	char d_chan_name[20];
	unsigned char d_ether_addr[ETH_ALEN];
	static const unsigned short ETHER_TYPE = 0xFFF0;

	/* the piconets we are monitoring */
	map<int, gr_bluetooth_piconet_sptr> d_piconets;

	/* handle AC */
	void ac(char *symbols, int len, int channel);

	/* handle ID packet (no header) */
	void id(uint32_t lap);

	/* decode packets with headers */
	void decode(gr_bluetooth_packet_sptr pkt, gr_bluetooth_piconet_sptr pn,
			bool first_run);

	/* work on UAP/CLK1-6 discovery */
	void discover(gr_bluetooth_packet_sptr pkt, gr_bluetooth_piconet_sptr pn);

	/* decode stored packets */
	void recall(gr_bluetooth_piconet_sptr pn);

	/* pull information out of FHS packet */
	void fhs(gr_bluetooth_packet_sptr pkt);

public:
	/* destructor */
	~gr_bluetooth_multi_sniffer();

	/* handle input */
	int work(int noutput_items, gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items);
};

#endif /* INCLUDED_GR_BLUETOOTH_MULTI_SNIFFER_H */
