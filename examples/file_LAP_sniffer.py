#!/usr/bin/python

from gnuradio import gr, eng_notation
from gnuradio import gr, blks
from gnuradio import usrp
from gnuradio import bluetooth
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import struct


class my_graph(gr.flow_graph):

    def __init__(self):
        gr.flow_graph.__init__(self)

        usage="%prog: [options] input_filename"
        parser = OptionParser(option_class=eng_option, usage=usage)
	parser.add_option("-N", "--nsamples", type="eng_float", default=None,
                          help="number of samples to collect [default=+inf]")
        (options, args) = parser.parse_args ()
        if len(args) != 1:
            parser.print_help()
            raise SystemExit, 1
	filename = args[0]

	self.src = gr.file_source (gr.sizeof_gr_complex, filename, False)

	# GMSK demodulate baseband to bits
	self.demod = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	
	#Set up the packet sniffer
	self.dst = bluetooth.LAP(1)
	
	if options.nsamples is None:
            self.connect(self.src, self.demod, self.dst)
        else:
	    self.head = gr.head(gr.sizeof_gr_complex, int(options.nsamples))
	    self.connect(self.src, self.head, self.demod, self.dst)

        
if __name__ == '__main__':
    try:
        my_graph().run()
    except KeyboardInterrupt:
        pass
