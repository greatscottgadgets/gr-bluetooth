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
        parser.add_option("-f", "--freq", type="eng_float", default=None,
                          help="set frequency to FREQ", metavar="FREQ")
	parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=(0, 0),
                          help="select USRP Rx side A or B (default=A)")
	parser.add_option("-N", "--nsamples", type="eng_float", default=None,
                          help="number of samples to collect [default=+inf]")
	parser.add_option("-g", "--gain", type="eng_float", default=None,
                          help="set gain in dB (default is midpoint)")
        parser.add_option("-l", "--lap", type="string", default=None,
                          help="LAP of the master device")
	parser.add_option("-u", "--uap", type="string", default=None,
                          help="UAP of the master device")
        (options, args) = parser.parse_args ()
        if len(args) != 1:
            parser.print_help()
            raise SystemExit, 1
	filename = args[0]

	self.src = gr.file_source (gr.sizeof_gr_complex, filename)


	# GMSK demodulate baseband to bits
	self.demod = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	
	#Set up the packet sniffer
	LAP = int(options.lap, 16)
	UAP = int(options.uap, 16)
	self.dst = bluetooth.sniffer(LAP, UAP)
	
	self.connect(self.src, self.demod, self.dst)

        
if __name__ == '__main__':
    try:
        my_graph().run()
    except KeyboardInterrupt:
        pass
