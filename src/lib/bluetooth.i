/* -*- c++ -*- */

%feature("autodoc", "1");		// generate python docstrings

%include "exception.i"
%import "gnuradio.i"			// the common stuff

%{
#include "gnuradio_swig_bug_workaround.h"	// mandatory bug fix
#include "bluetooth_sniffer.h"
#include "bluetooth_LAP.h"
#include "bluetooth_UAP.h"
#include "bluetooth_block.h"
#include "bluetooth_dump.h"
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
GR_SWIG_BLOCK_MAGIC(bluetooth,UAP);

bluetooth_UAP_sptr bluetooth_make_UAP (int LAP, int pkts);

class bluetooth_UAP : public gr_sync_block
{
private:
  bluetooth_UAP (int LAP, int pkts);
};

/*
 * First arg is the package prefix.
 * Second arg is the name of the class minus the prefix.
 */
GR_SWIG_BLOCK_MAGIC(bluetooth,dump);

bluetooth_dump_sptr bluetooth_make_dump ();

class bluetooth_dump : public gr_sync_block
{
private:
  bluetooth_dump ();
};

