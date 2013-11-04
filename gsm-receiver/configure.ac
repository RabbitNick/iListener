dnl 
dnl  Copyright 2004,2005,2006,2007,2008 Free Software Foundation, Inc.
dnl  
dnl  This file is part of GNU Radio
dnl  
dnl  GNU Radio is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 3, or (at your option)
dnl  any later version.
dnl  
dnl  GNU Radio is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl  
dnl  You should have received a copy of the GNU General Public License
dnl  along with GNU Radio; see the file COPYING.  If not, write to
dnl  the Free Software Foundation, Inc., 51 Franklin Street,
dnl  Boston, MA 02110-1301, USA.
dnl 

AC_INIT(gsm-receiver,0.0.3)
AC_PREREQ(2.57)
AC_CONFIG_AUX_DIR([.])

m4_include([config/gr_standalone.m4])
GR_STANDALONE

dnl Check for libosmocore (gsmtap support)
#PKG_CHECK_MODULES(LIBOSMOCORE, libosmocore >= 0.1.13)

AC_PROG_CXX
AC_PROG_CC

AC_CONFIG_FILES([\
	  Makefile \
	  config/Makefile \
	  src/Makefile \
	  src/lib/Makefile \
	  src/lib/libnetwork/Makefile \
	  src/lib/decoder/Makefile \
	  src/lib/libbase64cc/Makefile \
	  src/lib/decoder/openbtsstuff/Makefile \
	  src/python/Makefile \
	  gsm-receiver.pc   \
	])
dnl #	  doc/Makefile \
dnl #	  src/python/run_tests \

dnl run_tests is created from run_tests.in.  Make it executable.
#AC_CONFIG_COMMANDS([run_tests], [chmod +x src/python/run_tests])


dnl Check for header files you need
dnl AC_CHECK_HEADERS(fcntl.h limits.h strings.h sys/ioctl.h sys/time.h unistd.h)
dnl AC_CHECK_HEADERS(sys/mman.h)

dnl Checks for library functions.
dnl AC_CHECK_FUNCS([])

dnl We pick up the boost cppflags, cxxflags and thread lib via GNURADIO_CORE
dnl
dnl If you need additional boost libraries, you'll need to
dnl uncomment AX_BOOST_BASE, plus some of the following:
dnl
dnl calls AC_SUBST(BOOST_CPPFLAGS), AC_SUBST(BOOST_LDFLAGS) and defines HAVE_BOOST
dnl Current Boost version requirement is >=1.35 for all platforms except Darwin,
dnl which requires >=1.37 for code in usrp host library.
case "$host_os" in
     darwin*)
        AX_BOOST_BASE([1.37])
        ;;
      *)
        AX_BOOST_BASE([1.35])
        ;;
esac
AX_BOOST_THREAD
CXXFLAGS="$CXXFLAGS $BOOST_CXXFLAGS"       dnl often picks up a -pthread or something similar
CFLAGS="$CFLAGS $BOOST_CXXFLAGS"           dnl often picks up a -pthread or something similar

dnl
dnl All the rest of these call AC_SUBST(BOOST_<foo>_LIB) and define HAVE_BOOST_<foo>
dnl
AX_BOOST_DATE_TIME
dnl AX_BOOST_FILESYSTEM
AX_BOOST_IOSTREAMS
AX_BOOST_PROGRAM_OPTIONS
dnl AX_BOOST_REGEX
dnl AX_BOOST_SERIALIZATION
dnl AX_BOOST_SIGNALS
AX_BOOST_SYSTEM
dnl AX_BOOST_TEST_EXEC_MONITOR
dnl AX_BOOST_UNIT_TEST_FRAMEWORK
dnl AX_BOOST_WSERIALIZATION



AC_OUTPUT
