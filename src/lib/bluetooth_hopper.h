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
#ifndef INCLUDED_BLUETOOTH_HOPPER_H
#define INCLUDED_BLUETOOTH_HOPPER_H

#include "bluetooth_UAP.h"
#include "bluetooth_piconet.h"

static const int sequence_length = 134217728;
static const int channels = 79;

class bluetooth_hopper;
typedef boost::shared_ptr<bluetooth_hopper> bluetooth_hopper_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_hopper.
 */
bluetooth_hopper_sptr bluetooth_make_hopper (int LAP, int channel);

/*!
 * \brief Sniff Bluetooth packets, determine UAP, reverse hopping sequence
 * \ingroup block
 */
class bluetooth_hopper : public bluetooth_UAP
{
private:
	// The friend declaration allows bluetooth_make_hopper to
	// access the private constructor.

	friend bluetooth_hopper_sptr bluetooth_make_hopper (int LAP, int channel);

	bluetooth_hopper (int LAP, int channel);	// private constructor

	/* channel number (0-78) we are observing */
	int d_channel;

	/* the piconet we are monitoring */
	bluetooth_piconet_sptr d_piconet;

public:
	~bluetooth_hopper ();	// public destructor

	int work (int noutput_items, gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_HOPPER_H */
