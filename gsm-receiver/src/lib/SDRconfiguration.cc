/*
 * SDRconfiguration.cc
 *
 *  Created on: 2012��10��21��
 *      Author: hhh
 */

#include "SDRconfiguration.h"





void print_sdrconf(struct SDRconfiguration sdr_config) {

	std::cout << "decim: \t\t\t" << sdr_config.decim << std::endl;
	std::cout << "freq: \t\t\t" << sdr_config.freq << std::endl;
	std::cout << "osr: \t\t\t" << sdr_config.osr << std::endl;
	std::cout << "ch0_infile: \t\t" << sdr_config.ch0_infile << std::endl;
	std::cout << "ch1_infile: \t\t" << sdr_config.ch1_infile << std::endl;
	std::cout << "ch0_arfcn: \t\t" << sdr_config.ch0_arfcn << std::endl;
	std::cout << "ch1_arfcn: \t\t" << sdr_config.ch1_arfcn << std::endl;
	std::cout << "rxa_gain: \t\t" << sdr_config.rxa_gain << std::endl;
	std::cout << "rxb_gain: \t\t" << sdr_config.rxb_gain << std::endl;
	std::cout << "is_double_downlink: \t" << sdr_config.is_double_downlink << std::endl;
	std::cout << "which_board: \t\t" << sdr_config.which_board << std::endl;

	std::cout << "realtime: \t\t" << sdr_config.realtime << std::endl;

	std::cout << "ch0_upload: \t\t" << sdr_config.ch0_upload << std::endl;
	std::cout << "ch1_upload: \t\t" << sdr_config.ch1_upload << std::endl;

//	std::cout << "revmode: \t\t" << sdr_config.revmode << std::endl;
	std::cout << "sch_timeout_boundary: \t\t" << sdr_config.sch_timeout_boundary << std::endl;
	std::cout << "fcch_timeout_boundary: \t\t" << sdr_config.fcch_timeout_boundary << std::endl;

//	std::cout << "scan_bts_chanstart: \t\t" << sdr_config.scan_bts_chanstart << std::endl;
//	std::cout << "scan_bts_chanstop: \t\t" << sdr_config.scan_bts_chanstop << std::endl;

/*
	std::cout << "wideband_mode: \t\t" << sdr_config.wideband_mode << std::endl;
	std::cout << "wb_cfreq_arfcn: \t\t" << sdr_config.wb_cfreq_arfcn << std::endl;
	std::cout << "wb_fcch_chan: \t\t" << sdr_config.wb_fcch_chan << std::endl;
	*/


//	std::cout << "fcch_timeout_boundary: \t\t" << sdr_config.wb_bandwith << std::endl;

	std::cout << "fcch_ch: \t\t" << sdr_config.fcch_ch << std::endl;

	std::cout << "width8: \t\t" << sdr_config.width8 << std::endl;



	std::cout << "net_ip: \t\t" << sdr_config.net_ip << std::endl;

	std::cout << "net_port: \t\t" << sdr_config.net_port << std::endl;
}


int ini_handler(void* user, const char* section, const char* name, const char* value)
{
    struct SDRconfiguration* pconfig = (struct SDRconfiguration*)user;

    #define MATCH(s, n) strcasecmp(section, s) == 0 && strcasecmp(name, n) == 0
	if (MATCH("general", "decim")) {
		pconfig->decim = atoi(strdup(value));
	} else if (MATCH("general", "freq")) {
		pconfig->freq = atoi(value);
	} else if (MATCH("general", "osr")) {
		pconfig->osr = atoi(value);
	} else if (MATCH("general", "ch0_infile")) {
		pconfig->ch0_infile = strdup(value);
	} else if (MATCH("general", "ch1_infile")) {
		pconfig->ch1_infile = strdup(value);
	} else if (MATCH("general", "ch0_arfcn")) {
		pconfig->ch0_arfcn = atoi(value);
	} else if (MATCH("general", "ch1_arfcn")) {
		pconfig->ch1_arfcn = atoi(value);
	} else if (MATCH("general", "rxa_gain")) {
		pconfig->rxa_gain = atoi(value);
	} else if (MATCH("general", "rxb_gain")) {
		pconfig->rxb_gain = atoi(value);
	} else if (MATCH("general", "fcch_ch")) {
		pconfig->fcch_ch = atoi(value);



	} else if (MATCH("general", "is_double_downlink")) {
		pconfig->is_double_downlink = (bool) atoi(value);
	} else if (MATCH("general", "realtime")) {
		pconfig->realtime = (bool) atoi(value);
	} else if (MATCH("general", "which_board")) {
		pconfig->which_board = atoi(strdup(value));
	}// else if (MATCH("general", "scanBTSmode")) {
	//	pconfig->scanBTSmode = atoi(strdup(value));
	//}
	  else if (MATCH("general", "fcch_timeout_boundary")) {
		pconfig->fcch_timeout_boundary = atoi(strdup(value));
	} else if (MATCH("general", "sch_timeout_boundary")) {
		pconfig->sch_timeout_boundary = atoi(strdup(value));
	} //else if (MATCH("general", "scan_bts_chanstart")) {
	//	pconfig->scan_bts_chanstart = atoi(strdup(value));
	//} else if (MATCH("general", "scan_bts_chanstop")) {
	//	pconfig->scan_bts_chanstop = atoi(strdup(value));
	//}
	/*
	  else if (MATCH("general", "wideband_mode")) {
			pconfig->wideband_mode = atoi(strdup(value));
	} else if (MATCH("general", "wb_fcch_chan")) {
		pconfig->wb_fcch_chan = atoi(strdup(value));
	} else if (MATCH("general", "wb_bandwith")) {
		pconfig->wb_bandwith = atoi(strdup(value));
	} else if (MATCH("general", "wb_cfreq_arfcn")) {
		pconfig->wb_cfreq_arfcn = atoi(strdup(value));
	} else if (MATCH("general", "wb_chan1")) {
		pconfig->wb_chan1 = atoi(strdup(value));
	} */else if(MATCH("general", "width8")) {
		pconfig->width8 = atoi(strdup(value));
	}
	  else if (MATCH("libnetwork", "ch0_upload")) {
		pconfig->ch0_upload = (bool) atoi(strdup(value));
	} else if (MATCH("libnetwork", "ch1_upload")) {
		pconfig->ch1_upload = (bool) atoi(strdup(value));
	} else if (MATCH("libnetwork", "server")) {
		pconfig->net_ip = strdup(value);
	} else if (MATCH("libnetwork", "port")) {
		pconfig->net_port = atoi(strdup(value));
	}

    return 1;

}



