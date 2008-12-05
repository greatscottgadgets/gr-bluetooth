#!/usr/bin/python

"""
Bluetooth monitoring utility.
Receives samples from USRP, file (as created by usrp_rx_cfile.py), or standard input.
If LAP is unspecified, LAP detection mode is enabled.
If LAP is specified without UAP, UAP detection mode is enabled.
If both LAP and UAP are specified, . . .
"""

from gnuradio import gr, eng_notation, blks2
from gnuradio import usrp
from gnuradio import bluetooth
from gnuradio.eng_option import eng_option
from optparse import OptionParser
import sys

class my_top_block(gr.top_block):

	def __init__(self):
		gr.top_block.__init__(self)

		usage="%prog: [options]"
		parser = OptionParser(option_class=eng_option, usage=usage)
		parser.add_option("-N", "--nsamples", type="eng_float", default=None,
						help="number of samples to collect [default=+inf]")
		parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=(0, 0),
						help="select USRP Rx side A or B (default=A)")
		parser.add_option("-c", "--ddc", type="string", default="0",
						help="comma separated list of ddc frequencies (default=0)")
		parser.add_option("-d", "--decim", type="int", default=32,
						help="set fgpa decimation rate to DECIM (default=32)") 
		parser.add_option("-f", "--freq", type="eng_float", default=0,
						help="set USRP frequency to FREQ", metavar="FREQ")
		parser.add_option("-g", "--gain", type="eng_float", default=None,
						help="set USRP gain in dB (default is midpoint)")
		parser.add_option("-i", "--input-file", type="string", default=None,
						help="use named input file instead of USRP")
		parser.add_option("-l", "--lap", type="string", default=None,
						help="LAP of the master device")
		parser.add_option("-s", "--sample-rate", type="eng_float", default=0,
						help="sample rate of input (default: use -d)")
		parser.add_option("-p", "--packets", type="int", default=100,
						help="Number of packets to sniff (default=100)")
		parser.add_option("-u", "--uap", type="string", default=None,
						help="UAP of the master device")
		parser.add_option("-2","--usrp2", action="store_true", default=False,
						help="use USRP2 (or file originating from USRP2) instead of USRP")

		(options, args) = parser.parse_args ()
		if len(args) != 0:
			parser.print_help()
			raise SystemExit, 1

		# use options.sample_rate unless not provided by user
		if options.sample_rate == 0:
			if options.usrp2:
				# original source is USRP2
				adc_rate = 100e6
			else:
				# assume original source is USRP
				adc_rate = 64e6
			options.sample_rate = adc_rate / options.decim

		# select input source
		if options.input_file is None:
			# input from USRP or USRP2
			if options.usrp2:
				# FIXME, but not right away
				raise NotImplementedError
			else:
				src = usrp.source_c(decim_rate=options.decim)
				subdev = usrp.selected_subdev(src, options.rx_subdev_spec)
				r = src.tune(0, subdev, options.freq)
				if not r:
					sys.stderr.write('Failed to set USRP frequency\n')
					raise SystemExit, 1
				if options.gain is None:
					# if no gain was specified, use the mid-point in dB
					g = subdev.gain_range()
					options.gain = float(g[0]+g[1])/2
				subdev.set_gain(options.gain)
		elif options.input_file == '-':
			# input from stdin
			src = gr.file_descriptor_source(gr.sizeof_gr_complex, 0)
		else:
			# input from file
			src = gr.file_source(gr.sizeof_gr_complex, options.input_file)
	
		# coefficients for filter to select single channel
		channel_filter = gr.firdes.low_pass(1.0, options.sample_rate, 500e3, 500e3, gr.firdes.WIN_HANN)

		# look for packets on each channel specified by options.ddc
		# this works well for a small number of channels, but a more efficient
		# method should be possible for a large number of contiguous channels.
		for ddc_option in options.ddc.split(","):
			ddc_freq = int(eng_notation.str_to_num(ddc_option))
			# digital downconverter
			# does three things:
			# 1. converts frequency so channel of interest is centered at 0 Hz
			# 2. filters out everything outside the channel
			# 3. downsamples to 2 Msps (2 samples per symbol)
			ddc = gr.freq_xlating_fir_filter_ccf(int(options.sample_rate/2e6), channel_filter, ddc_freq, options.sample_rate)

			# GMSK demodulate baseband to bits
			demod = blks2.gmsk_demod(mu=0.32, samples_per_symbol=2)
			#demod = blks2.gmsk_demod(mu=0.32, samples_per_symbol=2, log=True, verbose=True)

			# bluetooth decoding
			if options.lap is None:
				# print out LAP for every frame detected
				dst = bluetooth.LAP(ddc_freq)
			else:
				# determine UAP from frames matching the user-specified LAP
				# FIXME do something different if both LAP and UAP are specified
				# FIXME analyze multiple channels together, not separately
				dst = bluetooth.UAP(int(options.lap, 16), options.packets)
		
			# connect the blocks
			if options.nsamples is None:
					self.connect(src, ddc, demod, dst)
			else:
				head = gr.head(gr.sizeof_gr_complex, int(options.nsamples))
				self.connect(src, head, ddc, demod, dst)

if __name__ == '__main__':
	try:
		my_top_block().run()
	except KeyboardInterrupt:
		pass
