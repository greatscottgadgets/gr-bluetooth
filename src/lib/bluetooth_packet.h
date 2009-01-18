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
#ifndef INCLUDED_BLUETOOTH_PACKET_H
#define INCLUDED_BLUETOOTH_PACKET_H

#include <stdint.h>
#include <boost/enable_shared_from_this.hpp>

class bluetooth_packet;
typedef boost::shared_ptr<bluetooth_packet> bluetooth_packet_sptr;

/*!
 * \brief Return a shared_ptr to a new instance of bluetooth_packet.
 */
bluetooth_packet_sptr bluetooth_make_packet(char *stream);

class bluetooth_packet
{
private:
	/* allow bluetooth_make_packet to access the private constructor. */
	friend bluetooth_packet_sptr bluetooth_make_packet(char *stream);

	/* constructor */
	bluetooth_packet(char *stream);

public:

	/* destructor */
	~bluetooth_packet();
};

#endif /* INCLUDED_BLUETOOTH_PACKET_H */
