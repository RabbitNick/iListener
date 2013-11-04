/*
 * run.cc
 *
 *  Created on: 2012��10��20��
 *      Author: hhh
 */

#include "run.h"

using namespace std;
namespace po = boost::program_options;

struct SDRconfiguration sdr_config;

bool process_program_options(po::variables_map _vm);

void init_sdr_config(struct SDRconfiguration *sdr_config)
{

	sdr_config->decim = 96;
    sdr_config->freq = 52000000;
    sdr_config->osr = 4;
    std::string ch0_infile = "/home";
    std::string ch1_infile = "/home";
    sdr_config->ch0_arfcn = 32;
    sdr_config->ch1_arfcn = 32;
    sdr_config->rxa_gain = 28;
    sdr_config->rxb_gain = 28;
    sdr_config->is_double_downlink = 1;
    sdr_config->realtime = 1;
    sdr_config->which_board = 0;
    sdr_config->fcch_ch = 0;

    sdr_config->ch0_upload = 1;
    sdr_config->ch1_upload = 1;
    std::string net_ip = "127.0.0.1";
    sdr_config->net_port = 8000;

    sdr_config->sch_timeout_boundary = 10;
    sdr_config->fcch_timeout_boundary = 50;

    sdr_config->scan_bts_chanstart = 1;
    sdr_config->scan_bts_chanstop = 124;                                            

    sdr_config->width8 = 0;


}


int main(int argc, char *argv[]) {

	init_sdr_config(&sdr_config);
	int config_error = ini_parse("run.ini", ini_handler, &sdr_config);
	if (config_error < 0) {
		printf("Can't load 'run.ini'\n");
	}


	po::variables_map vm;

	po::options_description cmdconfig("Program options: gsm_receiver_top [options] filename");
	cmdconfig.add_options()
			("help,h", "produce help message")
			("decim,d", po::value<int>(&sdr_config.decim), "set fgpa decimation rate to DECIM")
			("freq,f", po::value<int>(&sdr_config.freq), "set frequency to FREQ")
			("osr,o", po::value<int>(&sdr_config.osr), "Oversampling ratio")
			("fcch_ch,C", po::value<int>(&sdr_config.fcch_ch), "ch0 or ch1")

			("dl-cfile,D",po::value<std::string>(&sdr_config.ch0_infile), "cfile for downlink or ch0")
			("ul-cfile,U",po::value<std::string>(&sdr_config.ch1_infile), "cfile for uplink or ch1")

			("a0,a", po::value<int>(&sdr_config.ch0_arfcn), "ch0 ARFCN")
			("a1,b", po::value<int>(&sdr_config.ch1_arfcn), "ch1 ARFCN")
			("g0", po::value<int>(&sdr_config.rxa_gain), "RXA Gain")
			("g1", po::value<int>(&sdr_config.rxb_gain), "RXB Gain")

			("single,j", "Using one downlink, one uplink, cannot be used with -J option together")
			("double,J", "Using double downlink, if both -J and -j are not found, ini setting is used.")

			("realtime,R",po::value<bool>(&sdr_config.realtime), "Using realtime decoding, 0 or 1")
			("board,B",  po::value<int>(&sdr_config.which_board), "Using which_board (0-7)")

			("ch0-upload,r", po::value<bool>(&sdr_config.ch0_upload), "1 for enabling ch0 upload, 0 for disabling.")
			("ch1-upload,s", po::value<bool>(&sdr_config.ch1_upload), "1 for enabling ch1 upload, 0 for disabling.")


			("sch_timeout_boundary,S", po::value<int>(&sdr_config.sch_timeout_boundary),"set the sch timeout boundary (using integer).")
			("fcch_timeout_boundary,F", po::value<int>(&sdr_config.fcch_timeout_boundary),"set the fcch timeout boundary (using integer).")
			("scan_bts_chanstart,t", po::value<int>(&sdr_config.scan_bts_chanstart),"set scan-BTS starting channel.")
			("scan_bts_chanstop,T", po::value<int>(&sdr_config.scan_bts_chanstop),"set scan-BTS stopping channel.")

			("width8,w", po::value<int>(&sdr_config.width8),"set USRP's data format width(8/16 bit).")


			("IP_addr,i", po::value<std::string>(&sdr_config.net_ip),"set REV IP address.")
			("IP_port,p", po::value<int>(&sdr_config.net_port),"set REV IP PORT.")


			("print-ini,P", "Print ini file contents. Don't use with other options.")



			;


	po::store(po::parse_command_line(argc, argv, cmdconfig), vm);
	po::notify(vm);


	if (vm.count("help")) {

	    std::cout << cmdconfig << "\n";
	    exit(0);
	}
	if (vm.count("print-ini")) { //
		print_sdrconf(sdr_config);
		exit(0);
	}

	if (!process_program_options(vm)) {//process the remaining options
		std::cout << cmdconfig << "\n";
		exit(0);

	}
	gsm_receiver_top_block_sptr top_block = make_gsm_receiver_top_block(sdr_config);
	top_block->run();

	return 0;
}



/**
 * Return true if program_options all right
 * return false for invalid program options
 */
bool process_program_options(po::variables_map _vm) {

	if (_vm.count("single") && _vm.count("double")) {
		return false;
	}

	if (_vm.count("single")) {
		sdr_config.is_double_downlink=false;
	} else if (_vm.count("double")) {
		sdr_config.is_double_downlink=true;
	}


	if (_vm.count("scan_bts_chanstart") && _vm.count("scan_bts_chanstop")) {
		sdr_config.scanBTSmode=1;
	} else sdr_config.scanBTSmode=0;

	if(sdr_config.scanBTSmode == 1)
	{
		sdr_config.ch0_arfcn = 1;
	}

	return true;
}
