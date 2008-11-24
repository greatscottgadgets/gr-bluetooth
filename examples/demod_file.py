#!/usr/bin/env python

from gnuradio import gr, blks
from optparse import OptionParser
from gnuradio.eng_option import eng_option
import struct


class my_graph(gr.flow_graph):

    def __init__(self):
        gr.flow_graph.__init__(self)

        usage="%prog: [options] input_filename output_filename"
        parser = OptionParser(option_class=eng_option, usage=usage)
	parser.add_option("-S", "--sps", type="eng_float", default=4,
                          help="Samples per symbol, default=4")
        (options, args) = parser.parse_args ()
        if len(args) != 2:
            parser.print_help()
            raise SystemExit, 1
        filename = args[0]
	outputfilename = args[1]



	self.src = gr.file_source(gr.sizeof_gr_complex, filename)
	demod = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	self.dst = gr.file_sink(gr.sizeof_char, outputfilename)

	self.connect(self.src, demod, self.dst)

        
if __name__ == '__main__':
    try:
        my_graph().run()
    except KeyboardInterrupt:
        pass

