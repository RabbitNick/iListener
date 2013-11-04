#!/bin/sh

#CFILE_PATH=/home/my_project/usrp/airprobe/my_gsm_receiver1/cfile/TD-60-32-20120607-165931_down.cfile
CFILE_PATH=/home/my_project/usrp/airprobe/my_gsm_receiver1/cfile/TD-60-32-20120607-172128_down.cfile
#CFILE_PATH=/home/my_project/usrp/airprobe/my_gsm_receiver1/cfile/TD-60-96-20120607-190410_d.cfile

./gsm_receiver_double_chan_top -U $CFILE_PATH -D $CFILE_PATH -N 32 -M 32
