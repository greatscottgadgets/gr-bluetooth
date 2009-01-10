/* -*- c++ -*- */
/*
 * Copyright 2004 Free Software Foundation, Inc.
 * 
 * This file is part of GNU Radio
 * 
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#ifndef INCLUDED_BLUETOOTH_MULTI_BLOCK_H
#define INCLUDED_BLUETOOTH_MULTI_BLOCK_H

#include "bluetooth_block.h"
#include "bluetooth_LAP.h"
#include <gr_pwr_squelch_cc.h>
#include <gr_firdes.h>
#include <gr_freq_xlating_fir_filter_ccf.h>
#include <stdio.h>
#include <stdlib.h>

class bluetooth_multi_block;
typedef boost::shared_ptr<bluetooth_multi_block> bluetooth_multi_block_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_multi_block.
 */
bluetooth_multi_block_sptr bluetooth_make_multi_block ();

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class bluetooth_multi_block : public bluetooth_block
{
private:
	// The friend declaration allows bluetooth_make_multi_block to
	// access the private constructor.

	friend bluetooth_multi_block_sptr bluetooth_make_multi_block ();

protected:
	bluetooth_multi_block ();  	// private constructor

public:
	~bluetooth_multi_block ();	// public destructor

	// Where all the action really happens

	int work (int noutput_items,
		gr_vector_const_void_star &input_items,
		gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_MULTI_BLOCK_H */
