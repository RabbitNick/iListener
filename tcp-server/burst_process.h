/*
 * burst_process.h
 *
 *  Created on: Jun 20, 2012
 *      Author: hhh
 */

#ifndef BURST_PROCESS_H_
#define BURST_PROCESS_H_

#include "global.h"
#include <vector>
#include <pthread.h>

extern SERVER_CONF server_conf;

extern int g_itotal_arfcn;
extern int cell_chan_dec;
extern vector<int> d_arfcn_list;
extern pthread_mutex_t mutex_physical_channel_num ;
extern pthread_mutex_t mutex_arfcnnum ;
extern char my_time[50];

void *burst_process_thread(void *);
void *burst_process_ia_thread(void *);
void *burstdata_maintain_thread(void *);
void *deal_pdu_to_sms(void *);

int get_system_info_frame(unsigned char* burstbits, int fn);
int is_ccch_fframes(int mfn);
void do_process(unsigned char* burstbits, int arfcn,int timeslot, int fn);
void process_bcch();
short get_ccch_conf(unsigned char *gsm_frame);
void print_burst(unsigned char* b);
void output_xml(unsigned char* gsm_frame, int arfcn,LOGICAL_CHANNEL_TYPE logicalchannel,int fn,int mfn, int timeslot);

LOGICAL_CHANNEL_TYPE get_logical_channeltype(int arfcn, int timeslot, int fn,  int &burst_position);
int decode_ia(unsigned char *gsm_frame,int *charia_data);
bool is_first_burst(LOGICAL_CHANNEL_TYPE t, int fn);
int is_sdcch_fframes(int mfn);
int is_sdcch_up_fframes(int mfn);
void store_ia_data(unsigned char *gsm_frame);
void print_burst(unsigned char* b);
void print_frame(unsigned char* gsm_frame);
void store_fn(int arfcn,int fn);
int latest_sync_fn(void);
int get_arfcn_fn(int arfcn);
void decode_bit_map_0(unsigned char *gsm_frame);
void decode_bit_map_1024(unsigned char *gsm_frame);
void decode_bit_map_512(unsigned char *gsm_frame);
void decode_bit_map_256(unsigned char *gsm_frame);
void decode_bit_map_128(unsigned char *gsm_frame);
void decode_bit_map_Variable(unsigned char *gsm_frame);

struct Channel_description {
	int arfcn;
	int mfn;
	int timeslot;
	int sdcch;
};




#endif /* BURST_PROCESS_H_ */
