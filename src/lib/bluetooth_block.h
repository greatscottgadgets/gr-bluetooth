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
#ifndef INCLUDED_BLUETOOTH_BLOCK_H
#define INCLUDED_BLUETOOTH_BLOCK_H

#include <gr_sync_block.h>
#include <stdint.h>
#include <cstdio>

static const int IN = 1;
static const int OUT = 0;

/*!
 * \brief Bluetooth parent class.
 * \ingroup block
 */
class bluetooth_block : public gr_sync_block
{
protected:

	bluetooth_block ();

	// could be 32 bits, but then it would roll over after about 70 minutes
	uint64_t d_cumulative_count;
	uint32_t d_LAP;
	uint8_t d_UAP;
};

#endif /* INCLUDED_BLUETOOTH_BLOCK_H */
