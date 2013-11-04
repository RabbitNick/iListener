/*
 * decoder.cc
 *
 *  Created on: Jun 30, 2012
 *      Author: hhh
 */
#include <string.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "gs_process/gsmstack.h"
#include "gs_process/cch.h"
#include "decoders.h"
#include "burstdata.h"
#include "burst_fifo.h"
#include "burst_process.h"
#include "dealia.h"
#include "math.h"

extern Burst_fifo *fifo;
extern GS_CTX ctx;
int RNTABLE[114]={48,98,63,1,36,95,78,102,94,73,\
		           0,64,25,81,76,59,124,23,104,100,\
		           101,47,118,85,18,56,96,86,54,2,\
		           80,34,127,13,6,89,57,103,12,74,\
		           55,111,75,38,109,71,112,29,11,88,\
		           87,19,3,68,110,26,33,31,8,45,\
		           82,58,40,107,32,5,106,92,62,67,\
		           77,108,122,37,60,66,121,42,51,126,\
		           117,114,4,90,43,52,53,113,120,72,\
		           16,49,7,79,119,61,22,84,9,97,\
		           91,15,21,24,46,39,93,105,65,70,\
		           125,	99,	17,	123};
//In IA we can get mobile allocation of each communicate
//We can get cell allocation from system info 1
int ma[16]={0};//hard code for mobile allocation
short kill_ia=0;

/**
 *  Return int len of the hex decoded. 0 for error
 */
int assemble_scch(CHANNEL_DESC_MAP::iterator iter_chan_desc, unsigned char* hex) {

	unsigned char burst[BURST_SIZE];
	int _ia_fn; // IA's framenumber
	bool _hopping;
	short _sub_channel, _timeslot, sdcch_start_mfn,sacch_start_mfn; //common
	int next_new_fn, nsdcch_start_fn,nsacch_start_fn;
	short _darfcn; // not hopping
	int fifo_lastest_sync_fn;
	_darfcn= (*iter_chan_desc).first.arfcn;
	_ia_fn = (*iter_chan_desc).first.fn;
	_hopping = (*iter_chan_desc).first.hopping;
	_sub_channel = (*iter_chan_desc).first.subchannel;
	_timeslot = (*iter_chan_desc).first.timeslot;
	if ((*iter_chan_desc).first.channel_type == SDCCH8_SACCH8_or_CBCH) {
		sdcch_start_mfn = SDCCH_8_SUBCHANNEL_SFRAMES[_sub_channel];
		sacch_start_mfn = SACCH_8_SUBCHANNEL_SFRAMES[_sub_channel];
	} else {

		if (server_conf.debug)
			printf("Type not supported, skip! \n");
		return 0;
	}
	next_new_fn = _ia_fn + 102 - _ia_fn%102;  // the next multiframe 0's FN
	nsdcch_start_fn=next_new_fn+sdcch_start_mfn;  // sdcch fn, the next multiframe
	nsacch_start_fn=next_new_fn+sacch_start_mfn;
	nsdcch_start_fn+=(*iter_chan_desc).second.passed_times*102; // get to the multiframe followed by cycles
	nsacch_start_fn+=(*iter_chan_desc).second.passed_times*102;
	pthread_mutex_lock(&mutex_arfcnnum);
	fifo_lastest_sync_fn=latest_sync_fn();
	if (server_conf.debug)
		printf("fifo_lastest_sync_fn %d \n",fifo_lastest_sync_fn);
	pthread_mutex_unlock(&mutex_arfcnnum);
	if(fifo_lastest_sync_fn<(nsacch_start_fn+102))
	{
		return 0;
	}
	if(nsdcch_start_fn>0x296ffe)
	{
		nsdcch_start_fn-=0x296ffe;//is it the last fn?
		nsacch_start_fn-=0x296ffe;//yes! reset fn .
	}

	// Whether the buffer is long enough
	if (_hopping)
	{
		if (server_conf.debug)
			printf("Hopping \n");

		int mai=0;
		int n_len=(*iter_chan_desc).first.mobile_len;
		for(int i=0;i<n_len;i++)
		{
			int v=(*((*iter_chan_desc).first.mobile_allocation+i));
			for(int j=0;j<8;j++)   //Count the 1 in N
			{
				if(v&(1<<j))
				{
					ma[mai]=d_arfcn_list[8*i+j];
					mai++;
				}
			}
			if (server_conf.debug)
				printf("n_len: %d \n",mai);
		}
		decoders_hdownlink( nsdcch_start_fn,iter_chan_desc,(LOGICAL_CHANNEL_TYPE)SDCCH,mai);
		if(kill_ia!=1)
			decoders_hdownlink( nsdcch_start_fn+51,iter_chan_desc,(LOGICAL_CHANNEL_TYPE)SDCCH,mai);
		if(kill_ia!=1)
			decoders_hdownlink( nsacch_start_fn,iter_chan_desc,(LOGICAL_CHANNEL_TYPE)SACCH,mai);
		(*iter_chan_desc).second.passed_times++;
	}
	else
	{
		if (server_conf.debug)
		{
			printf("Start to decode SDCCH: \n");
			printf("ARFCN: %d; FN: %d  MFN: %d, subchannel: %d TS: %d \n", \
				_darfcn, nsdcch_start_fn, nsdcch_start_fn%51, _sub_channel, _timeslot);
		}
		decoders_downlink(nsdcch_start_fn,iter_chan_desc,(LOGICAL_CHANNEL_TYPE)SDCCH);
		if(kill_ia!=1)
			decoders_downlink( nsdcch_start_fn+51,iter_chan_desc,(LOGICAL_CHANNEL_TYPE)SDCCH);
		if(kill_ia!=1)
			decoders_downlink( nsacch_start_fn,iter_chan_desc,(LOGICAL_CHANNEL_TYPE)SACCH);
		(*iter_chan_desc).second.passed_times++;

	}
	if(kill_ia==1)
	{
		kill_ia=0;
		return 100;
	}

		return 0;
}

int get_harfcn(int start_fn,short _hsn,short _maio,int mai)
{

	int _t1=start_fn/1326;
	int _t2=start_fn%26;
	int _t3=start_fn%51;
	int _M=0,_M1=0,_T=0,_S=0;
	int a1=0;
	int _mai=0;
	if (server_conf.debug)
	{
		printf("t1: %d t2: %d t3: %d\n",_t1,_t2,_t3);
		printf("HSN=%d maio=%d \n",_hsn,_maio);
	}
	_M=_t2+RNTABLE[(_hsn^(_t1%64))+_t3];
	if (server_conf.debug)
	{
		printf("_M=%d\n",_M);
	}
	a1=pow(2,(int)((log(mai)/log(2.0))+1));
	if (server_conf.debug)
	{
		printf("a=%d\n",a1);
	}
	_M1=_M%a1;
	_T=_t3%a1;

	if(_M1<(mai))
	{
		_S=_M1;
	}
	else
		_S=(_M1+_T)%(mai);


	_mai=(_maio+_S)%(mai);
	return ma[_mai];

}

void decoders_downlink(int start_fn,CHANNEL_DESC_MAP::iterator iter_chan_desc,LOGICAL_CHANNEL_TYPE type)
{
	int s=0; // burst position counter
	int b;
	int len=0;
	int itime=0;
	unsigned char* gsm_frame;
	unsigned char burst[BURST_SIZE];
	struct gs_ts_ctx* ts_ctx;
	ts_ctx = &ctx.ts_ctx[7];
	memset(ts_ctx, 0, sizeof(*ts_ctx));
	for(int k=start_fn; k<start_fn+4;++k)
	{
		if (server_conf.debug)
			printf("hihi: Arfcn: %d, FN: %d /MFN: %d, TS: %d \n",\
					(*iter_chan_desc).first.arfcn, k, k%51, (*iter_chan_desc).first.timeslot);
		//goto fifo to take the sdcchs
		memset(burst, 0, sizeof(*burst));
		int ret = fifo->retrieve((*iter_chan_desc).first.arfcn, k,(*iter_chan_desc).first.timeslot, burst);
		if (ret != -1)
		{
			b = GS_group_process(ts_ctx, s++, burst);
		}
		else
		{
			if (server_conf.debug)
				printf("Cannot fetch burst. \n");
			return ;
		}
		if (server_conf.debug)
		{
			printf("FIFO result: %d \n", ret);
			printf("Burst count#1: %d \n", ts_ctx->burst_count);
		}
		if (server_conf.debug)
			printf("GS_group_process return: %d \n", b); //when finish here, b shoout == 4
	} //for
	gsm_frame = decode_cch(&ctx, ts_ctx->burst,(unsigned int*) &len);
	if (gsm_frame == NULL)
	{
		memset(burst, 0, sizeof(*burst));
		if(type!=SACCH)
			(*iter_chan_desc).second.missed_frames++;
		if((*iter_chan_desc).second.missed_frames>=4)
		{
			kill_ia=1;
			if (server_conf.debug)
					printf("kill_ia1\n");
		}
		if (server_conf.debug)
			printf("XSDCCH Decode error\n");
	}
	else
	{
		//successed
		(*iter_chan_desc).second.decoded_frames++;

		if(is_communication_end(gsm_frame))
		{
			kill_ia=1;
			if (server_conf.debug)
				printf("kill_ia\n");
		}
		if (server_conf.print_octets) {
			print_frame(gsm_frame);
		}

		itime=time(NULL);
		if(type==SDCCH)
			output_xml(gsm_frame, (*iter_chan_desc).first.arfcn,(LOGICAL_CHANNEL_TYPE)SDCCH, \
				start_fn, start_fn%51,(*iter_chan_desc).first.timeslot);
		else
			output_xml(gsm_frame+2, (*iter_chan_desc).first.arfcn, (LOGICAL_CHANNEL_TYPE)SACCH, \
				start_fn, start_fn%51,(*iter_chan_desc).first.timeslot);
	}
	return ;
}


void decoders_hdownlink(int start_fn,CHANNEL_DESC_MAP::iterator iter_chan_desc,LOGICAL_CHANNEL_TYPE type,int mai)
{
	int s=0; // burst position counter
	int b;
	int len=0;
	int itime=0;
	unsigned char* gsm_frame;
	unsigned char burst[BURST_SIZE];
	struct gs_ts_ctx* ts_ctx;
	int ret =0;
	int _darfcn;
	ts_ctx = &ctx.ts_ctx[7];
	int tmp_arfcn[4]={0,0,0,0};
	memset(ts_ctx, 0, sizeof(*ts_ctx));
	for(int k=start_fn; k<start_fn+4;++k)
	{
		//goto fifo to take the sdcchs
		if((*iter_chan_desc).first.hsn!=0)
			_darfcn=get_harfcn( k,(*iter_chan_desc).first.hsn, (*iter_chan_desc).first.maio,mai);
		else
			_darfcn=ma[(k+(*iter_chan_desc).first.maio) % mai];
		tmp_arfcn[k-start_fn]=_darfcn;
		if (server_conf.debug)
			printf("hihi: Arfcn: %d, FN: %d /MFN: %d, TS: %d \n", _darfcn, k, k%51, (*iter_chan_desc).first.timeslot);
		int ret = fifo->retrieve(_darfcn, k, (*iter_chan_desc).first.timeslot, burst);
		if (server_conf.debug)
		{
			printf("FIFO result: %d \n", ret);
			printf("Burst count#1: %d \n", ts_ctx->burst_count);
		}
		if (ret != -1)
		{
			b = GS_group_process(ts_ctx, s++, burst);
		}
		else
		{
			if (server_conf.debug)
				printf("Cannot fetch burst. \n");
			return ;
		}
		if (server_conf.debug)
			printf("GS_group_process return: %d \n", b); //when finish here, b shoout == 4
	} //for

	gsm_frame = decode_cch(&ctx, ts_ctx->burst,(unsigned int*) &len);
	if (gsm_frame == NULL)
	{
		memset(burst, 0, sizeof(*burst));
		if(type!=SACCH)
			(*iter_chan_desc).second.missed_frames++;
		if((*iter_chan_desc).second.missed_frames>=4)
		{
			kill_ia=1;
		}
		if (server_conf.debug)
			printf("XSDCCH Decode error\n");
	}
	else
	{
		//successed
		(*iter_chan_desc).second.decoded_frames++;
		if (server_conf.print_octets) {
			print_frame(gsm_frame);
		}
		if(is_communication_end(gsm_frame))
		{
			kill_ia=1;
			if (server_conf.debug)
				printf("kill_ia\n");
		}
		itime=time(NULL);
		if(type==SDCCH)
			output_xml(gsm_frame, _darfcn,(LOGICAL_CHANNEL_TYPE)SDCCH, start_fn, start_fn%51,(int)(*iter_chan_desc).first.timeslot);
		else
			output_xml(gsm_frame+2, _darfcn,(LOGICAL_CHANNEL_TYPE)SACCH, start_fn, start_fn%51,(int)(*iter_chan_desc).first.timeslot);
	}
	return ;
}

bool is_communication_end(unsigned char* gsm_frame)
{
	if((gsm_frame[0]==0x01)&&(gsm_frame[1]==0x73)&&(gsm_frame[2]==0x01))//is it func=ua?
		return 1;
	else
		return 0;
}


