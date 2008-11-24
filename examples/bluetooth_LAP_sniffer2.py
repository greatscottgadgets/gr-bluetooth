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

        usage="%prog: [options]"
        parser = OptionParser(option_class=eng_option, usage=usage)
        parser.add_option("-f", "--freq", type="eng_float", default=None,
                          help="set frequency to FREQ", metavar="FREQ")
	parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=(0, 0),
                          help="select USRP Rx side A or B (default=A)")
	parser.add_option("-N", "--nsamples", type="eng_float", default=None,
                          help="number of samples to collect [default=+inf]")
	parser.add_option("-g", "--gain", type="eng_float", default=None,
                          help="set gain in dB (default is midpoint)")
        (options, args) = parser.parse_args ()
        if len(args) != 0:
            parser.print_help()
            raise SystemExit, 1


	#self.src = gr.file_source (gr.sizeof_gr_complex, filename, False)
	self.u = usrp.source_c(decim_rate=16)
	self.subdev = usrp.selected_subdev(self.u, options.rx_subdev_spec)
        r = self.u.tune(0, self.subdev, options.freq)
        if options.gain is None:
            # if no gain was specified, use the mid-point in dB
            g = self.subdev.gain_range()
            options.gain = float(g[0]+g[1])/2

        self.subdev.set_gain(options.gain)

	# compute FIR filter taps for channel selection
	channel_coeffs = \
	    gr.firdes.low_pass (1.0,          # gain
                          4000000,   # sampling rate
                          1,        # low pass cutoff freq
                          1e6,      # width of trans. band
                          gr.firdes.WIN_HAMMING)

	# input: short; output: complex
	self.chan_filter1 = \
	    gr.freq_xlating_fir_filter_ccc (1,
                                      channel_coeffs,
                                      2000000,        # 1st station freq
                                      4000000)

	# GMSK demodulate baseband to bits
	self.demod = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	self.demod2 = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	
	#Set up the packet sniffer
	self.dst = bluetooth.LAP(0)
	self.dst2 = bluetooth.LAP(1)

	if options.nsamples is None:
            self.connect(self.u, self.chan_filter1, self.demod, self.dst)
	    self.connect(self.u, self.demod2, self.dst2)
        else:
	    self.head = gr.head(gr.sizeof_gr_complex, int(options.nsamples))
	    self.head2 = gr.head(gr.sizeof_gr_complex, int(options.nsamples))
	    self.connect(self.u, self.head, self.chan_filter1, self.demod, self.dst)
	    self.connect(self.u, self.head2, self.demod2, self.dst2)


        
if __name__ == '__main__':
    try:
        my_graph().run()
    except KeyboardInterrupt:
        pass
