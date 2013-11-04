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


from gnuradio.eng_option import eng_option
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import wx
import gsm #, howto

from gnuradio import gr, gru, blks2
from gnuradio import usrp
from gnuradio.gr import firdes
from gnuradio import eng_notation
from gnuradio.wxgui import stdgui2, fftsink2, waterfallsink2, scopesink2, form, slider
from optparse import OptionParser
from math import pi

import os
import os.path


downfile = '/srv/samba/share/cfiles/yd889/A0-300-20120316-103611_down.cfile'
upfile = '/srv/samba/share/cfiles/yd889/A0-300-20120316-103611_down.cfile'
#upfile = '/srv/samba/share/cfiles/yd889/A0-300-20120316-103611_up.cfile'

####################
class tuner(gr.feval_dd):
    def __init__(self, top_block):
        gr.feval_dd.__init__(self)
        self.top_block = top_block
    def eval(self, freq_offet):
        self.top_block.set_center_frequency(freq_offet)
        return freq_offet

class synchronizer(gr.feval_dd):
    def __init__(self, top_block):
        gr.feval_dd.__init__(self)
        self.top_block = top_block

    def eval(self, timing_offset):
        self.top_block.set_timing(timing_offset)
        return freq_offet



class burst_callback(gr.feval_ll):
	def __init__(self, fg):
		gr.feval_ll.__init__(self)
		self.fg = fg
		self.offset_mean_num = 10		#number of FCCH offsets to average
		self.offset_vals = []
		
####################
	def eval(self, x):
		#print "burst_callback: eval(",x,")\n";
		try:
			
			if gsm.BURST_CB_SYNC_OFFSET == x:
				print "burst_callback: SYNC_OFFSET\n";
				if self.fg.options.tuning.count("o"):
					last_offset = self.fg.burst.last_freq_offset()
					self.fg.offset -= last_offset
					#print "burst_callback: SYNC_OFFSET:", last_offset, " ARFCN: ", self.fg.channel, "\n";
					#self.fg.set_channel(self.fg.channel)
					self.fg.filter0.set_center_freq(-last_offset) 
					self.fg.filter1.set_center_freq(-last_offset) 

			elif gsm.BURST_CB_ADJ_OFFSET == x:
				last_offset = self.fg.burst.last_freq_offset()
				self.offset_vals.append(last_offset)
				count =  len(self.offset_vals)
				print "burst_callback: ADJ_OFFSET:", last_offset, ", count=",count,"\n";
				self.fg.filter0.set_center_freq(-last_offset) 
				self.fg.filter1.set_center_freq(-last_offset) 
				
				#if count <= self.offset_mean_num:
					#sum = 0.0
					#while len(self.offset_vals):
					#	sum += self.offset_vals.pop(0)

					#self.fg.mean_offset = sum / self.offset_mean_num
				
					#print "burst_callback: mean offset:", self.fg.mean_offset, "\n";
					
					#retune if greater than 100 Hz
					#if abs(self.fg.mean_offset) > 100.0:	
					#	print "burst_callback: mean offset adjust:", self.fg.mean_offset, "\n";
						#if self.fg.options.tuning.count("o"):	
							#print "burst_callback: tuning.\n";
					#		self.fg.offset -= self.fg.mean_offset
							#self.fg.set_channel(self.fg.channel)
					#	self.fg.filter0.set_center_freq(self.fg.offset)
					#	self.fg.filter1.set_center_freq(self.fg.offset)  
							
			elif gsm.BURST_CB_TUNE == x:
				print "burst_callback: BURST_CB_TUNE: ARFCN: ", self.fg.burst.next_arfcn, "\n";
				if self.fg.options.tuning.count("h"):
					#print "burst_callback: tuning.\n";
					self.fg.set_channel(self.fg.burst.next_arfcn)

			return 0

		except Exception, e:
			print >> sys.stderr, "burst_callback: Exception: ", e


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
		print "abc"
		##################################################
		# Variables
		##################################################
		#self.samp_rate = samp_rate = 32000
		self.osr = 4
		self.key = ''
		self.configuration = ''

		self.clock_rate = 52e6
		self.input_rate = self.clock_rate / 72		#TODO: what about usrp value?
		self.gsm_symb_rate = 1625000.0 / 6.0
		self.sps = self.input_rate / self.gsm_symb_rate




		# configure channel filter
		filter_cutoff	= 135e3		#135,417Hz is GSM bandwidth 
		filter_t_width	= 10e3
		offset = 0.0

		##################################################
		# Blocks
		##################################################
		self.gr_null_sink_0 = gr.null_sink(gr.sizeof_gr_complex*1)

		print "Input files: ", downfile, " ", upfile
		self.gr_file_source_0 = gr.file_source(gr.sizeof_gr_complex*1, downfile, False)
		self.gr_file_source_1 = gr.file_source(gr.sizeof_gr_complex*1, upfile, False)


		filter_taps = gr.firdes.low_pass(1.0, self.input_rate, filter_cutoff, filter_t_width, gr.firdes.WIN_HAMMING)

		print len(filter_taps)

		self.filter0 = gr.freq_xlating_fir_filter_ccf(1, filter_taps, offset, self.input_rate)
		self.filter1 = gr.freq_xlating_fir_filter_ccf(1, filter_taps, offset, self.input_rate)

		self.interpolator_1 = gr.fractional_interpolator_cc(0, self.sps) 

        	self.tuner_callback = tuner(self)
	        self.synchronizer_callback = synchronizer(self)

		#self.buffer =  howto.buffer_cc()

		self.burst_cb = burst_callback(self)

		print ">>>>>Input rate: ", self.input_rate
		#self.burst = gsm.burst_cf(self.burst_cb,self.input_rate)
        	self.receiver = gsm.receiver_cf(self.tuner_callback, self.synchronizer_callback, self.osr, self.key.replace(' ', '').lower(), self.configuration.upper())
		##################################################
		# Connections
		##################################################
		#self.connect((self.gr_file_source_0, 0), (self.filter0, 0), (self.burst, 0))

		self.connect((self.gr_file_source_1, 0), (self.filter1, 0), (self.interpolator_1, 0), (self.receiver, 0))
#		self.connect((self.gr_file_source_1, 0),  (self.buffer, 0))
#		self.connect((self.gr_file_source_0, 0),  (self.buffer, 1))

	####################
	def set_freq(self, freq):
		#TODO: for wideband processing, determine if the desired freq is within our current sample range.
		#		If so, use the frequency translator to tune.  Tune the USRP otherwise.
		#		Maybe have a flag to force tuning the USRP?
		self.using_usrp = 0
		if not self.using_usrp:
			#if reading from file just adjust for offset in the freq translator
			if self.print_status:
				print >> sys.stderr, "Setting filter center freq to offset: ", self.offset, "\n"
			print self.offset

			self.filter0.set_center_freq(self.offset)
			self.filter1.set_center_freq(self.offset)
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



	def set_center_frequency(self, center_freq):
		self.filtr.set_center_freq(center_freq)

	def set_timing(self, timing_offset):
		pass	

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
	#downfile = args[0]
	#upfile = args[1]
	tb = top_block()
	#tb.Run(True)
	tb.run()
