/*
 * main.c
 *
 *  Created on: Jun 21, 2012
 *      Author: hhh
 */
#include <stdio.h>
#include <iostream>
#include <vector>
#include "global.h"
#include "tcp-server.h"
#include "string.h"

#include "burst_process.h"
#include "burstdata.h"
#include "gs_process/gsmstack.h"
#include "burst_fifo.h"
#include "decoders.h"
#include <pthread.h>
#include <algorithm>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>

using namespace std;
namespace po = boost::program_options;


pthread_t threads[NUM_THREADS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int n_threads;

SERVER_CONF server_conf;
CCCH_CONF ccch_conf;
int d_fn;
int d_bfn;
int d_mfn;
int d_tc;
int del_ia=0;
int cell_chan_dec =0;

unsigned long ccch_decode=0;
unsigned long ccch_error=0;
unsigned long xcch_decode=0;
unsigned long xcch_error=0;

unsigned long getburst=0;
unsigned long getnull=0;
int arfcn_fn [20];

char my_time[50];

bool get_system_info;

vector<int> d_arfcn_list;
int fcch_arfcn;
STATUS status;

vector<unsigned char*> g_burst;  // grouped burst;
//unsigned char g_burst[4];

GS_CTX ctx;
GS_CTX ctx_d;
Burst_fifo *fifo;


int main(int argc, char *argv[]) {

	ARFCNSET::iterator iter_arfcnset;
	g_itotal_arfcn=0;
	po::options_description cmdconfig("Program options");
	//po::options_description desc("Allowed options");
	cmdconfig.add_options()
		 ("help,h", "Produce help message")
         ("arfcn,a", po::value<vector<int> >(&d_arfcn_list)->multitoken(), "Set arfcn(s), the first to be the one with FCCH")
         ("outall,x", "Output all gsm frames to XML")
         ("printoctets,o", "Print octets to screen")
         ("burstinput,i", "Print bursts upload from receivers")
         ("autoarfcn_list,l", "Enable auto ARFCN list from client sides")
		 ("debug,D", "Temp for debug use, change over per programmer")
	;

	//("ia", "Print Immediate Assignment info")
	po::variables_map vm;

	// the following line is from boost eg:
	po::store(po::parse_command_line(argc, argv, cmdconfig), vm);
	po::notify(vm);

	//po::store(po::command_line_parser(argc, argv).
	//		options(config).positional(inputfile).run(), vm);

	if (vm.count("help")) {
	    cout << cmdconfig << "\n";
	    exit(0);
	}



	if (vm.count("arfcn")) {

		//
		// The following option session must be put before
		//	arfcn process session
		//
		if (vm.count("autoarfcn_list")) {
			printf("ARFCN auto mode \n");
			server_conf.autoarfcn=true;
		} else {
			printf("ARFCN manual mode \n");
			server_conf.autoarfcn=false;
		}
		if (vm.count("printoctets")) {
			server_conf.print_octets=true;
		} else {
			server_conf.print_octets=false;
		}
		if(vm.count("outall")) {
			server_conf.output_allframes=true;
		} else {
			server_conf.output_allframes=false;
		}
		if(vm.count("burstinput")) {
			server_conf.printburstinput=true;
		} else {
			server_conf.printburstinput=false;
		}

	if(vm.count("debug")) {
			server_conf.debug=true;
		} else
			server_conf.debug=false;

		vector<int> arfcns(vm["arfcn"].as< vector<int> > ());
		vector<string>::iterator vI;

		fcch_arfcn = d_arfcn_list[0];
		printf("FCCH ARFCN: %d", fcch_arfcn);

		if (!server_conf.autoarfcn)
			cout << "Number of input arfcn: " << arfcns.size() << endl;

		if (!server_conf.autoarfcn) {
			cout << "List of ARFCN(s): ";
			BOOST_FOREACH(int arfcn, d_arfcn_list) {
				std::cout << arfcn << " ";
				g_itotal_arfcn = arfcns.size();
			}
			fflush(stdout);
			sort(d_arfcn_list.begin(), d_arfcn_list.end());

		} // end if
		cout<<endl;
	} else {
		//cout << cmdconfig << "\n";
	    return 1;
	}



//	if (vm.count("compression")) {
//		cout << "Compression level was set to "
//				<< vm["compression"].as<int>() << ".\n";
//	} else {
//		cout << "Compression level was not set.\n";
//	}

    /* should be replaced by a function set config */

	/*
	 * by -HW, 2012Oct, 9th;
	 *
	 * added boost options above, the following parts will be removed soon
	 * keep here for ref or debug usage.
	 *
	 *
    if (argc > 1) {
        fcch_arfcn = atoi(argv[1]);
        for(int i=1; i<argc; ++i) {
            d_arfcn_list.push_back(atoi(argv[i]));
        }
    } else {
        printf("No arfcn inputed \n");
        exit(0);
    }
    sort(d_arfcn_list.begin(), d_arfcn_list.end());
	 */

    for(int i=0;i<8;i++) {
        ccch_conf.is_ccch_ts[i]=false;
    }

    n_threads = 0;
    status = idle;
    get_system_info = false;
    d_fn = 0;
    d_bfn = 0;
    int ret;

    //initialize xml file output
    time_t myTime=time(NULL);
    strftime(my_time,21,"%Y%m%d-%H%M%S",localtime(&myTime));
    strcat(my_time,".xml");
    FILE *pFILE = fopen(my_time,"w");
    fputs("<?xml version=\"1.0\"?>\n<dump> \n",pFILE);
    fclose(pFILE);

    initial(); //initialize server,include init_mutex_burst,and system init
    fifo=new Burst_fifo();// initialize fifo
    int a = GS_new(&ctx);
     a = GS_new(&ctx_d);
    /* these values will be set by System info 3 */
    ccch_conf.physical_channel_num = -1;
    ccch_conf.is_combined_with_SDCCHs = false;


    struct TCP_SERVER_CONF receiver[7];

   // int port_t=8000;
    for(int i=0;i<7;++i) {
    	receiver[i].port=8000+i;
    	receiver[i].is_fcch=0;
    }
    receiver[0].is_fcch=1;


    //start threads
	ret = pthread_create(&threads[++n_threads], NULL, burst_process_ia_thread, NULL);
	ret = pthread_create(&threads[++n_threads], NULL, burst_process_thread, NULL);
	ret = pthread_create(&threads[++n_threads], NULL, burstdata_maintain_thread, NULL);
    for(int i=0;i<7;++i) {
    	ret = pthread_create(&threads[++n_threads], NULL, tcpserver_thread, \
    			(struct TCP_SERVER_CONF*)&receiver[i]);
    }

    while(1)
    sleep(100);
    pthread_exit(NULL);
    return 0;
}

