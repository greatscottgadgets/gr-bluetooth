/* -*- c++ -*- */

%feature("autodoc", "1");		// generate python docstrings

%include "exception.i"
%import "gnuradio.i"			// the common stuff

%{
#pragma GCC diagnostic ignored "-Wuninitialized"
#include "gnuradio_swig_bug_workaround.h"	// mandatory bug fix
#include "bluetooth_sniffer.h"
#include "bluetooth_LAP.h"
#include "bluetooth_multi_LAP.h"
#include "bluetooth_multi_UAP.h"
#include "bluetooth_multi_hopper.h"
#include "bluetooth_multi_sniffer.h"
#include "bluetooth_UAP.h"
#include "bluetooth_hopper.h"
#include "bluetooth_block.h"
#include "bluetooth_tun.h"
#include "bluetooth_bluecore_source.h"
#include <stdexcept>
%}

// ----------------------------------------------------------------

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,sniffer);

bluetooth_sniffer_sptr bluetooth_make_sniffer (int lap, int uap);

class bluetooth_sniffer : public gr_sync_block
{
private:
  bluetooth_sniffer (int lap, int uap);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,LAP);

bluetooth_LAP_sptr bluetooth_make_LAP (int x);

class bluetooth_LAP : public gr_sync_block
{
private:
  bluetooth_LAP (int x);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,multi_LAP);

bluetooth_multi_LAP_sptr bluetooth_make_multi_LAP (double sample_rate, double center_freq, double squelch_threshold);

class bluetooth_multi_LAP : public gr_sync_block
{
private:
  bluetooth_multi_LAP (double sample_rate, double center_freq, double squelch_threshold);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,multi_UAP);

bluetooth_multi_UAP_sptr bluetooth_make_multi_UAP (double sample_rate, double center_freq, double squelch_threshold, int LAP);

class bluetooth_multi_UAP : public gr_sync_block
{
private:
  bluetooth_multi_UAP (double sample_rate, double center_freq, double squelch_threshold, int LAP);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,multi_hopper);

bluetooth_multi_hopper_sptr bluetooth_make_multi_hopper (double sample_rate, double center_freq, double squelch_threshold, int LAP, bool aliased, bool tun);

class bluetooth_multi_hopper : public gr_sync_block
{
private:
  bluetooth_multi_hopper (double sample_rate, double center_freq, double squelch_threshold, int LAP, bool aliased, bool tun);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,multi_sniffer);

bluetooth_multi_sniffer_sptr bluetooth_make_multi_sniffer (double sample_rate, double center_freq, double squelch_threshold, bool tun);

class bluetooth_multi_sniffer : public gr_sync_block
{
private:
  bluetooth_multi_sniffer (double sample_rate, double center_freq, double squelch_threshold, bool tun);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,UAP);

bluetooth_UAP_sptr bluetooth_make_UAP (int LAP);

class bluetooth_UAP : public gr_sync_block
{
private:
  bluetooth_UAP (int LAP);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,hopper);

bluetooth_hopper_sptr bluetooth_make_hopper (int LAP, int channel);

class bluetooth_hopper : public gr_sync_block
{
private:
  bluetooth_hopper (int LAP, int channel);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,tun);

bluetooth_tun_sptr bluetooth_make_tun (int x, int channel);

class bluetooth_tun : public gr_sync_block
{
private:
  bluetooth_tun (int x, int channel);
};

/*
 * Dongle source module to receive data from a bluecore.
 */
/*
GR_SWIG_BLOCK_MAGIC(bluetooth,bluecore_source);

bluetooth_bluecore_source_sptr bluetooth_make_bluecore_source (int device);

class bluetooth_bluecore_source : public gr_sync_block
{
private:
  bluetooth_bluecore_source (int device);
};
*/
