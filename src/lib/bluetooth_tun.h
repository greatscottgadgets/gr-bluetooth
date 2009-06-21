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
#ifndef INCLUDED_BLUETOOTH_TUN_H
#define INCLUDED_BLUETOOTH_TUN_H

#include "bluetooth_block.h"
#include <stdlib.h>
#include <linux/if_ether.h>

class bluetooth_tun;
typedef boost::shared_ptr<bluetooth_tun> bluetooth_tun_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_tun.
 */
bluetooth_tun_sptr bluetooth_make_tun (int x, int channel);

/*!
 * \brief Sniff Bluetooth packets.
 * \ingroup block
 */
class bluetooth_tun : public bluetooth_block
{
private:
  // The friend declaration allows bluetooth_make_tun to
  // access the private constructor.

  friend bluetooth_tun_sptr bluetooth_make_tun (int x, int channel);

  bluetooth_tun (int x, int channel);  	// private constructor

  int d_x;

  // Wireshark interface
  int			d_tunfd;	// TUN fd
  char			chan_name[20];  // TUN interface name
  unsigned char		d_ether_addr[ETH_ALEN];
static const unsigned short HCI_H1 = 0xFFFD;
static const unsigned short HCI_H4 = 0xFFFE;

public:
  ~bluetooth_tun ();	// public destructor

  // Where all the action really happens

  int work (int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif /* INCLUDED_BLUETOOTH_TUN_H */
