/* -*- c++ -*- */

#define GR_BLUETOOTH_API

%include "exception.i"
%include "gnuradio.i"			// the common stuff

%{
#pragma GCC diagnostic ignored "-Wuninitialized"
#include "gr_bluetooth_multi_LAP.h"
#include "gr_bluetooth_multi_UAP.h"
#include "gr_bluetooth_multi_hopper.h"
#include "gr_bluetooth_multi_sniffer.h"
#include <stdexcept>
%}

// ----------------------------------------------------------------

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
%include "gr_bluetooth_multi_LAP.h"
GR_SWIG_BLOCK_MAGIC(gr_bluetooth,multi_LAP);

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
%include "gr_bluetooth_multi_UAP.h"
GR_SWIG_BLOCK_MAGIC(gr_bluetooth,multi_UAP);

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
%include "gr_bluetooth_multi_hopper.h"
GR_SWIG_BLOCK_MAGIC(gr_bluetooth,multi_hopper);

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
%include "gr_bluetooth_multi_sniffer.h"
GR_SWIG_BLOCK_MAGIC(gr_bluetooth,multi_sniffer);
