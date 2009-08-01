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
#ifndef INCLUDED_BLUETOOTH_MULTI_SNIFFER_H
#define INCLUDED_BLUETOOTH_MULTI_SNIFFER_H

#include "bluetooth_multi_block.h"
#include "bluetooth_piconet.h"
#include "tun.h"
#include <map>

class bluetooth_multi_sniffer;
typedef boost::shared_ptr<bluetooth_multi_sniffer> bluetooth_multi_sniffer_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_multi_sniffer.
 */
bluetooth_multi_sniffer_sptr bluetooth_make_multi_sniffer(double sample_rate,
		double center_freq, double squelch_threshold, bool tun);

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class bluetooth_multi_sniffer : public bluetooth_multi_block
{
private:
	// The friend declaration allows bluetooth_make_multi_sniffer to
	// access the private constructor.
	friend bluetooth_multi_sniffer_sptr bluetooth_make_multi_sniffer(
			double sample_rate, double center_freq,
			double squelch_threshold, bool tun);

	/* constructor */
	bluetooth_multi_sniffer(double sample_rate, double center_freq,
			double squelch_threshold, bool tun);

	/* Using tun for output */
	bool d_tun;

	/* Tun stuff */
	int d_tunfd;
	char d_chan_name[20];
	unsigned char d_ether_addr[ETH_ALEN];
	static const unsigned short ETHER_TYPE = 0xFFF0;

	/* the piconets we are monitoring */
	map<int, bluetooth_piconet_sptr> d_piconets;

	/* handle AC */
	void ac(char *symbols, int len, int channel);

	/* handle ID packet (no header) */
	void id(uint32_t lap);

	/* decode packets with headers */
	void decode(bluetooth_packet_sptr pkt, bluetooth_piconet_sptr pn,
			bool first_run);

	/* work on UAP/CLK1-6 discovery */
	void discover(bluetooth_packet_sptr pkt, bluetooth_piconet_sptr pn);

	/* decode stored packets */
	void recall(bluetooth_piconet_sptr pn);

	/* pull information out of FHS packet */
	void fhs(bluetooth_packet_sptr pkt);

public:
	/* destructor */
	~bluetooth_multi_sniffer();

	/* handle input */
	int work(int noutput_items, gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_MULTI_SNIFFER_H */
