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
#ifndef INCLUDED_GR_BLUETOOTH_MULTI_LAP_H
#define INCLUDED_GR_BLUETOOTH_MULTI_LAP_H

#include "gr_bluetooth_multi_block.h"

class gr_bluetooth_multi_LAP;
typedef boost::shared_ptr<gr_bluetooth_multi_LAP> gr_bluetooth_multi_LAP_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of gr_bluetooth_multi_LAP.
 */
gr_bluetooth_multi_LAP_sptr gr_bluetooth_make_multi_LAP(double sample_rate, double center_freq, double squelch_threshold);

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class gr_bluetooth_multi_LAP : public gr_bluetooth_multi_block
{
private:
	// The friend declaration allows gr_bluetooth_make_multi_LAP to
	// access the private constructor.
	friend gr_bluetooth_multi_LAP_sptr gr_bluetooth_make_multi_LAP(double sample_rate, double center_freq, double squelch_threshold);

	/* constructor */
	gr_bluetooth_multi_LAP(double sample_rate, double center_freq, double squelch_threshold);

public:
	/* destructor */
	~gr_bluetooth_multi_LAP();

	/* handle input */
	int work(int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_GR_BLUETOOTH_MULTI_LAP_H */
