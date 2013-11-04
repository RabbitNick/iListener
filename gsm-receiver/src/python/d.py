#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Top Block
# Generated: Fri Sep 30 20:34:46 2011
##################################################

import sys
#nasty hack for testing
for extdir in ['../lib','../lib/.libs']:
	if extdir not in sys.path:
		sys.path.append(extdir)


from gnuradio import gr, gru, blks2
import gsm
from gnuradio.eng_option import eng_option
from optparse import OptionParser
from os import sys

import os
import os.path

ch_0 = 32
ch_1 = 32
ch = 'du'


file_0 = '/home/zhengxiangcai/TD-60-32-20120722-192645_down.done'
file_1 = '/home/zhengxiangcai/TD-60-32-20120722-192645_up.done'



#file_0 = '/home/my_project/usrp/airprobe/my_gsm_receiver1/cfile/TD-60-32-20120607-172128_down.cfile'
#file_1 = '/home/my_project/usrp/airprobe/my_gsm_receiver1/cfile/TD-60-32-20120607-172128_down.cfile'


#file_0 = '/srv/samba/s/TD-60-32-20120607-172128_down.cfile'
#file_1 = '/home/hope/TD-60-32-20120607-172128_up.cfile'

####################
class tuner(gr.feval_dd):
    def __init__(self, top_block):
        gr.feval_dd.__init__(self)
        self.top_block = top_block
    def eval(self, freq_offet):
	print 'Set Frequency', freq_offet
        self.top_block.set_center_frequency(freq_offet)
        return freq_offet

class synchronizer(gr.feval_dd):
    def __init__(self, top_block):
        gr.feval_dd.__init__(self)
        self.top_block = top_block

    def eval(self, timing_offset):
        self.top_block.set_timing(timing_offset)
        return freq_offet

###################################################
def get_freq_from_arfcn(chan,region):

	#P/E/R-GSM 900
	if chan >= 0 and chan <= 124:
		freq = 890 + 0.2*chan + 45

	#GSM 850
	elif chan >= 128 and chan <= 251:
		freq = 824.2 + 0.2*(chan - 128) + 45
		
	#GSM 450
	elif chan >= 259 and chan <= 293:
		freq = 450.6 + 0.2*(chan - 259) + 10
		
	#GSM 480
	elif chan >= 306 and chan <= 340:
		freq = 479 + 0.2*(chan - 306) + 10
		
	#DCS 1800
	elif region is "e" and chan >= 512 and chan <= 885:
		freq = 1710.2 + 0.2*(chan - 512) + 95
		
	#DCS 1900
	elif region is "u" and chan >= 512 and chan <= 810:
		freq = 1850.2 + 0.2*(chan - 512) + 80

	#E/R-GSM 900
	elif chan >= 955 and chan <= 1023:
		freq = 890 + 0.2*(chan - 1024) + 45

	else:
		freq = 0

	return freq * 1e6

def get_arfcn_from_freq(freq,region):
	freq = freq / 1e6
	# GSM 450
	if freq <= 450.6 + 0.2*(293 - 259) + 10:
		arfcn = ((freq - (450.6 + 10)) / 0.2) + 259
	# GSM 480
	elif freq <= 479 + 0.2*(340 - 306) + 10:
		arfcn = ((freq - (479 + 10)) / 0.2) + 306
	# GSM 850
	elif freq <= 824.2 + 0.2*(251 - 128) + 45:
		arfcn = ((freq - (824.2 + 45)) / 0.2) + 128
	#E/R-GSM 900
	elif freq <= 890 + 0.2*(1023 - 1024) + 45:
		arfcn = ((freq - (890 + 45)) / -0.2) + 955
	# GSM 900
	elif freq <= 890 + 0.2*124 + 45:
		arfcn = (freq - (890 + 45)) / 0.2
	else:
		if region is "u":
			if freq > 1850.2 + 0.2*(810 - 512) + 80:
				arfcn = 0;
			else:
				arfcn = (freq - (1850.2 + 80) / 0.2) + 512
		elif region is "e":
			if freq > 1710.2 + 0.2*(885 - 512) + 95:
				arfcn = 0;
			else:
				arfcn = (freq - (1710.2 + 95) / 0.2) + 512
		else:
			arfcn = 0

	return arfcn

####################

#class top_block(grc_wxgui.top_block_gui):
class top_block(gr.top_block):
	def __init__(self):
		#grc_wxgui.top_block_gui.__init__(self, title="Top Block")
		gr.top_block.__init__(self)
		
		##################################################
		# Variables
		##################################################
		#self.samp_rate = samp_rate = 32000

		self.osr = 4
		self.key = 'AD 6A 3E C2 B4 42 E4 00'
		self.configuration = ch
		self.ch0 = ch_0
		self.ch1 = ch_1

		self.clock_rate = 52e6
		self.input_rate = self.clock_rate / 72		#TODO: what about usrp value?
		self.gsm_symb_rate = 1625000.0 / 6.0
        	self.sps = self.input_rate / self.gsm_symb_rate / self.osr

		# configure channel filter
		filter_cutoff	= 145e3		#135,417Hz is GSM bandwidth 
		filter_t_width	= 10e3
		offset = 0.0

		##################################################
		# Blocks
		##################################################
		self.gr_null_sink_0 = gr.null_sink(gr.sizeof_gr_complex*1)

		print "Input files: ", file_0, " ", file_1
		self.gr_file_source_0 = gr.file_source(gr.sizeof_gr_complex*1, file_0, False)
					
		self.gr_file_source_1 = gr.file_source(gr.sizeof_gr_complex*1, file_1, False)

		filter_taps = gr.firdes.low_pass(1.0, self.input_rate, filter_cutoff, filter_t_width, gr.firdes.WIN_HAMMING)

		self.filter_0 = gr.freq_xlating_fir_filter_ccf(1, filter_taps, offset, self.input_rate)
		self.filter_1 = gr.freq_xlating_fir_filter_ccf(1, filter_taps, offset, self.input_rate)

		self.interpolator_0 = gr.fractional_interpolator_cc(0, self.sps) 
		self.interpolator_1 = gr.fractional_interpolator_cc(0, self.sps) 

		self.tuner_callback = tuner(self)
		self.synchronizer_callback = synchronizer(self)

		#print ">>>>>Input rate: ", self.input_rate		

		
		

		self.receiver = gsm.receiver_cf(self.tuner_callback, self.synchronizer_callback, self.osr, self.key.replace(' ', '').lower(), self.configuration.upper(), self.ch0, self.ch1, "127.0.0.1", 8080)

#		self.receiver = gsm.receiver_cf(self.tuner_callback, self.synchronizer_callback, self.osr, self.key.replace(' ', '').lower(), self.configuration.upper(), self.ch0, self.ch1)


		##################################################
		# Connections
		##################################################
		self.connect((self.gr_file_source_1, 0), (self.filter_1, 0), (self.interpolator_1, 0), (self.receiver, 1))

#		self.connect((self.gr_file_source_1, 0), (self.filter_1, 0), (self.interpolator_1, 0), (self.receiver, 1))

		self.connect((self.gr_file_source_0, 0), (self.filter_0, 0), (self.interpolator_0, 0), (self.receiver, 0))


	#        self.connect(self.source, self.filtr,  self.interpolator, self.receiver, self.converter, self.sink)
		#self.connect((self.gr_file_source_1, 0), (self.filter1, 0), (self.interpolator_1, 0), (self.burst, 1))



	def set_center_frequency(self, center_freq):
		self.filter_0.set_center_freq(center_freq)
		self.filter_1.set_center_freq(center_freq)
		pass

	def set_timing(self, timing_offset):
		pass

	####################
	def set_freq_frequency(self, freq):
		#TODO: for wideband processing, determine if the desired freq is within our current sample range.
		#		If so, use the frequency translator to tune.  Tune the USRP otherwise.
		#		Maybe have a flag to force tuning the USRP?
		self.using_usrp = 0
		if not self.using_usrp:
			#if reading from file just adjust for offset in the freq translator
			if self.print_status:
				print >> sys.stderr, "Setting filter center freq to offset: ", self.offset, "\n"
			print self.offset

			self.filter_0.set_center_freq(self.offset)
			self.filter_1.set_center_freq(self.offset)
			#print "set_freq......."
			return True
	
		freq = freq - self.offset

		r = self.ursp.tune(0, self.subdev, freq)

		if r:
			self.status_msg = '%f' % (freq/1e6)
			return True
		else:
			self.status_msg = "Failed to set frequency (%f)" % (freq/1e6)
			return False

	####################
	def set_gain(self, gain):

		if not self.using_usrp:
			return False

		self.subdev.set_gain(gain)

	####################
	def set_channel(self, chan):

		self.chan = chan
		
		freq = get_freq_from_arfcn(chan,self.region)

		if freq:
			self.set_freq(freq)
		else:
			self.status_msg = "Invalid Channel"
'''
	def get_samp_rate(self):
		return self.samp_rate

	def set_samp_rate(self, samp_rate):
		self.samp_rate = samp_rate

	def setup_usrp(self):
		self.decim = 48
		self.sample_rate = self.input_rate
		self.antenna_num = 2
		self.freq_down = 942.8e6 # 39 arfcn
		self.freq_up = self.freq_down - 45e6
		self.fpga_filename="std_4rx_0tx.rbf"
							'''

		
	

if __name__ == '__main__':
	parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
	(options, args) = parser.parse_args()
	file_0 = args[0]
	file_1 = args[1]
	tb = top_block()
	#tb.Run(True)
	tb.run()
