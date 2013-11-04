/*
 * decoder.h
 *
 *  Created on: Jun 30, 2012
 *      Author: hhh
 */

#ifndef DECODER_H_
#define DECODER_H_

#include "burstdata.h"

//typedef struct _hopping_element
//{
//
//	int start_fn;
//	int maio;
//	int hsn;
//
//}hopping_element ;

extern unsigned long ccch_decode;
extern unsigned long ccch_error;
extern unsigned long xcch_decode;
extern unsigned long xcch_error;

extern unsigned long getburst;
extern unsigned long getnull;
extern int arfcn_fn [20];
extern char my_time[50];

int decode_ia(unsigned char *gsm_frame);
int assemble_scch(CHANNEL_DESC_MAP::iterator, unsigned char* hex);
int get_harfcn(int start_fn,short _hsn,short _maio,int mai);

bool is_communication_end(unsigned char* gsm_frame);
void decoders_downlink(int start_fn,CHANNEL_DESC_MAP::iterator iter_chan_desc,LOGICAL_CHANNEL_TYPE type);
void decoders_hdownlink(int start_fn,CHANNEL_DESC_MAP::iterator iter_chan_desc,LOGICAL_CHANNEL_TYPE type,int mai);
#endif /* DECODER_H_ */
