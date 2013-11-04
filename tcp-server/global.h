/*
 * constant.h
 *
 *  Created on: Jun 19, 2012
 *      Author: hhh
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

//#include <vector>
#include <map>
#include <set>

using namespace std;

#define LISTEN_NUM 16

#define SERVER_PORT 8000
//#define NET_IP "202.193.9.12"
#define NET_IP "127.0.0.1"
#define MAX_LINK 10
#define BUF_SIZE 1024
#define STATU_CONNECT 1
#define STATU_DISCONN 0

#define NUM_THREADS 5

#define BURST_SIZE 116
#define TIMESLOT 8
/* CCCH-CONF==
 *0 1 basic physical channel used for CCCH,not combined with SDCCH
 *1 1 basic physical channel used for CCCH,combined with SDCCH
 *2 2 basic physical channel used for CCCH,not combined with SDCCH
 *4 3 basic physical channel used for CCCH,not combined with SDCCH
 *6 4 basic physical channel used for CCCH,not combined with SDCCH
 */
#define physical_channel_1_not_combined 1
#define physical_channel_1_combined 1
#define physical_channel_2_not_combined 2
#define physical_channel_3_not_combined 3
#define physical_channel_4_not_combined 4
#define packet_len 23
typedef unsigned char BURSTDATA[BURST_SIZE];
typedef BURSTDATA TDMA_FRAME[8];

struct CCCH_CONF{
	short physical_channel_num; //number of CCCH, TS0, TS2, TS4, TS6
	bool is_combined_with_SDCCHs;
	bool is_ccch_ts[8];
};

//
// assume to add all global variables here
// and move the old var into here
//
struct SERVER_CONF {
	bool autoarfcn;
	bool output_allframes;
	bool print_octets;
	bool printburstinput;
	bool debug;
};


struct TCP_SERVER_CONF {
	// the below are read from ini file using iniparser
	// they are pointer to an allocated memory
	char* host;
	int port;

	bool is_fcch;

};


//typedef enum {empty, fcch_burst, sch_burst, normal_burst, rach_burst, dummy, dummy_or_normal} burst_type;
typedef enum {
	FCCH, SCH, BCCH, CCCH, SACCH, SDCCH, unknown
} LOGICAL_CHANNEL_TYPE;

typedef enum {
	idle, waiting_bcch, process
} STATUS;

const unsigned BCCH_FRAMES[] = {2, 3, 4, 5};
const unsigned CCH_FRAMES[] = {2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 18, 19, 22, 23, 24, 25, 26, 27, 28, 29, 32, 33, 34, 35, 36, 37, 38, 39, 42, 43, 44, 45, 46, 47, 48, 49};
const unsigned SDCCH_FRAMES[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};
const unsigned SDCCH_FRAMES_UP[] = {0,1,2,3,4,5,6,7,8,9,10,11,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50};
//CCH_POS[] is matched with CCH_FRAMES
const unsigned CCH_POS[]   =  {0, 1, 2, 3, 0, 1, 2, 3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3};
const unsigned SDCCH_POS[]  =  {0, 1, 2, 3, 0, 1, 2, 3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3,  0,  1,  2,  3 , 0,  1,  2,  3, 0,  1,  2,  3, 0,  1,  2,  3, 0,  1,  2,  3};

const unsigned SDCCH_SACCH_8_FRAMES[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47};
const unsigned SDCCH_SACCH_8_FIRST[] =  {1, 0, 0, 0, 1, 0, 0, 0, 1, 0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0};

const unsigned SDCCH_8_SUBCHANNEL_SFRAMES[] = {0, 4, 8, 12, 16, 20, 24, 28 };//, 51, 55, 59, 63, 67, 71, 75, 79};
const unsigned SACCH_8_SUBCHANNEL_SFRAMES[] = {32, 36, 40, 44, 83, 87, 91, 95};
//const unsigned SDCCH_8_SUBCHANNEL_SFRAMES_UP[] = {15,19,23,27,31,35,39,43,47};
/*                                       0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50 */
const unsigned SDCCH_SACCH_4_MAP[51] =  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  0};






#endif /* GLOBAL_H_ */
