#
# Copyright 2004 Free Software Foundation, Inc.
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

include $(top_srcdir)/Makefile.common

GR_GSMTOP_LA=$(top_srcdir)/src/lib/libgnuradio-gsmtop.la

SDRCONFIGURATION_LA=$(top_srcdir)/src/lib/libSDRconfiguration.la

SDRINI_LA=$(top_srcdir)/src/lib/libSDRini.la

GR_USRP_LA=/usr/local/lib/libgnuradio-usrp.la

#BASE64_LA = $(top_srcdir)/src/lib/libbase64/libbase64.la

#AUTOMAKE_OPTIONS=foreign


run_LDADD = \
	$(GNURADIO_CORE_LA) \
	$(USRP_LA) \
	$(GR_USRP_LA)\
	$(BOOST_LDFLAGS) \
	$(BOOST_PROGRAM_OPTIONS_LIB)\
	$(LIBOSMOCORE_LIBS)  \
	$(GR_GSMTOP_LA)    \
	$(SDRINI_LA) \
	$(SDRCONFIGURATION_LA) 
	
noinst_PROGRAMS = \
	run

noinst_HEADERS = 	\
	run.h
run_SOURCES = \
	run.cc 

#EXTRA_DIST = gsm_findfcch.py gsm_findfcch_usrp.py
