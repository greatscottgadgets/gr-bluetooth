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
	parser.add_option("-g", "--gain", type="eng_float", default=None,
                          help="set gain in dB (default is midpoint)")
        (options, args) = parser.parse_args ()
        if len(args) != 0:
            parser.print_help()
            raise SystemExit, 1


	#self.src = gr.file_source (gr.sizeof_gr_complex, filename, False)
	self.u = usrp.source_c(decim_rate=16)
	self.subdev = usrp.selected_subdev(self.u, options.rx_subdev_spec)
	self.subdev.select_rx_antenna("TX/RX")
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
	self.chan_filter0 = gr.freq_xlating_fir_filter_ccc (1, channel_coeffs, 2000000, 4000000)
	self.chan_filter1 = gr.freq_xlating_fir_filter_ccc (1, channel_coeffs, 1000000, 4000000)
	self.chan_filter2 = gr.freq_xlating_fir_filter_ccc (1, channel_coeffs, 0, 4000000)
	self.chan_filter3 = gr.freq_xlating_fir_filter_ccc (1, channel_coeffs, -1000000, 4000000)
	self.chan_filter4 = gr.freq_xlating_fir_filter_ccc (1, channel_coeffs, -2000000, 4000000)
	self.chan_filter5 = gr.freq_xlating_fir_filter_ccc (1, channel_coeffs, -3000000, 4000000)
	

	# GMSK demodulate baseband to bits
	self.demod0 = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	self.demod1 = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	self.demod2 = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	self.demod3 = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	self.demod4 = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	self.demod5 = blks.gmsk_demod(self, mu=0.32, samples_per_symbol=4)
	
	#Set up the packet sniffer
	self.dst0 = bluetooth.LAP(2419)
	self.dst1 = bluetooth.LAP(2420)
	self.dst2 = bluetooth.LAP(2421)
	self.dst3 = bluetooth.LAP(2422)
	self.dst4 = bluetooth.LAP(2423)
	self.dst5 = bluetooth.LAP(2424)

	self.connect(self.u, self.chan_filter0, self.demod0, self.dst0)
	self.connect(self.u, self.chan_filter1, self.demod1, self.dst1)
	self.connect(self.u, self.chan_filter2, self.demod2, self.dst2)
	self.connect(self.u, self.chan_filter3, self.demod3, self.dst3)
	self.connect(self.u, self.chan_filter4, self.demod4, self.dst4)
	self.connect(self.u, self.chan_filter5, self.demod5, self.dst5)



        
if __name__ == '__main__':
    try:
        my_graph().run()
    except KeyboardInterrupt:
        pass
