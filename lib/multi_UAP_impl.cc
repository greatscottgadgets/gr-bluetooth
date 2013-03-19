/* -*- c++ -*- */
/* 
 * Copyright 2013 Christopher D. Kilgour
 * Copyright 2008, 2009 Dominic Spill, Michael Ossmann                                                                                            
 * Copyright 2007 Dominic Spill                                                                                                                   
 * Copyright 2005, 2006 Free Software Foundation, Inc.
 *
 * This file is part of gr-bluetooth
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_io_signature.h>
#include "multi_UAP_impl.h"
#include <stdio.h>

namespace gr {
  namespace bluetooth {

    multi_UAP::sptr
    multi_UAP::make(double sample_rate, double center_freq, double squelch_threshold, int LAP)
    {
      return gnuradio::get_initial_sptr (new multi_UAP_impl(sample_rate, center_freq, squelch_threshold, LAP));
    }

    /*
     * The private constructor
     */
    multi_UAP_impl::multi_UAP_impl(double sample_rate, double center_freq, double squelch_threshold, int LAP)
      : multi_block(sample_rate, center_freq, squelch_threshold),
        gr_sync_block ("bluetooth multi UAP block",
                       gr_make_io_signature (1, 1, sizeof (gr_complex)),
                       gr_make_io_signature (0, 0, 0))
    {
        d_LAP = LAP;
	set_symbol_history(SYMBOLS_FOR_BASIC_RATE_HISTORY);
	d_piconet = basic_rate_piconet::make(d_LAP);
    }

    /*
     * Our virtual destructor.
     */
    multi_UAP_impl::~multi_UAP_impl()
    {
    }

    int
    multi_UAP_impl::work(int noutput_items,
                         gr_vector_const_void_star &input_items,
                         gr_vector_void_star &output_items)
    {
      int retval;
      uint32_t clkn; /* native (local) clock in 625 us */
      double freq;
      char symbols[history()]; //poor estimate but safe

      clkn = (int) (d_cumulative_count / d_samples_per_slot) & 0x7ffffff;

      for (freq = d_low_freq; freq <= d_high_freq; freq += 1e6)
	{
          if (check_basic_rate_squelch(input_items)) {
            int num_symbols = channel_symbols(freq, input_items, symbols, history());
            if (num_symbols >= SYMBOLS_PER_BASIC_RATE_SHORTENED_ACCESS_CODE) {
              /* don't look beyond one slot for ACs */
              int latest_ac = ((num_symbols - SYMBOLS_PER_BASIC_RATE_SHORTENED_ACCESS_CODE) < SYMBOLS_PER_BASIC_RATE_SLOT) ? 
                (num_symbols - SYMBOLS_PER_BASIC_RATE_SHORTENED_ACCESS_CODE) : SYMBOLS_PER_BASIC_RATE_SLOT;
              retval = classic_packet::sniff_ac(symbols, latest_ac);
              if (retval > -1) {
                classic_packet::sptr packet = classic_packet::make( &symbols[retval], num_symbols - retval,
                                                                    clkn, freq );
                if (packet->get_LAP() == d_LAP && packet->header_present()) {
                  if (d_piconet->UAP_from_header(packet))
                    exit(0);
                  break;
                }
              }
            }
          }
	}
      d_cumulative_count += (int) d_samples_per_slot;

      /* 
       * The runtime system wants to know how many output items we produced, assuming that this is equal
       * to the number of input items consumed.  We tell it that we produced/consumed one time slot of
       * input items so that our next run starts one slot later.
       */
      return (int) d_samples_per_slot;
    }

  } /* namespace bluetooth */
} /* namespace gr */

