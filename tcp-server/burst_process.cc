/*
 * burst_process.cc
 *
 *  Created on: Jun 20, 2012
 *      Author: hhh
 */
#include <stdio.h>
#include <iostream>
#include <vector>
#include <pthread.h>
#include <string.h>
#include "global.h"
#include "burst_process.h"
#include "burstdata.h"
#include "gs_process/gsmstack.h"
#include "gs_process/cch.h"
#include "burst_fifo.h"
#include "dealia.h"
#include "decoders.h"
#include <time.h>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>

//#include "libsms/dealoctets.h"

extern GS_CTX ctx;
extern GS_CTX ctx_d;

extern CCCH_CONF ccch_conf;
extern int fcch_arfcn;
extern int d_tc;
extern int d_mfn;
extern int d_fn;
extern Burst_fifo *fifo;

//int ia_thread_start=0;
int ia_data[23];//for ia store;ia_data[0]->channel-type,
int ia_data_ex[23];//for ia_ex store
int channel_type[4]={1,2,4,8};//1 TCH/F ,2 TCH/H ,4 SDCCH/4 ,8 SDCCH/8
//int print_count=0;
/*
 * burst_process_ia_thread
 * add by ddcat
 * this thread use for store distinguish ia data and store it.
 */


void *burst_process_ia_thread(void *) //without buffer
{

	LOGICAL_CHANNEL_TYPE lch_type;
	BURSTKEY burstkey;
//	int ret;
	unsigned int len;
	unsigned char *gsm_frame;
	struct gs_ts_ctx* ts_ctx;


	int fpos = -1;
	unsigned char *IA_frame;
	int ia_fn= 0;
	int ia_mfn=0;
	unsigned char burstbits[116];
	int physical_channel_num =-1;
	int newest_fn=0;
	int get_ia=0;
	while(1)
	{
		while(physical_channel_num==-1)
		{
//			printf("physical_channel_num_while \n");
			physical_channel_num=ccch_conf.physical_channel_num;//get physical_channel_num
			usleep(1);
		}
		while((ia_fn==0)&&(get_ia==0))
		{
			printf("get_ia??? \n");
			ia_fn=get_arfcn_fn(fcch_arfcn);
			while(ia_fn==0)
			{
				ia_fn=get_arfcn_fn(fcch_arfcn);
				usleep(1);
			}
			get_ia=1;
			ia_fn++;
			usleep(1);
		}
		for(int timeslot=0;timeslot<=((physical_channel_num-1)*2);timeslot+=2)
		{
			printf("timeslot %d\n",timeslot);
			ts_ctx = &ctx.ts_ctx[timeslot];// extract ccch and store IA
			get_burst:
			ctx.fn = ia_fn;
			ia_mfn=ia_fn % 51;

			if ((fpos = is_ccch_fframes(ia_mfn)) != -1)//first frame of ccch, process IA
			{
				printf("fpos  %d \n",fpos);
				int ret = fifo->retrieve(fcch_arfcn, ia_fn, timeslot, burstbits);
				printf("ret %d ia_fn %d\n",ret,ia_fn);
				if (ret == -1)
				{
					//wait
					usleep(1) ;
					newest_fn=get_arfcn_fn(fcch_arfcn);
					printf("newest_fn %d \n",newest_fn);
					if(ia_fn>newest_fn)
					{
						if(ia_fn-newest_fn>0x296ffe/2)//说明循环了
						{//这种情况下ia_fn是给它赋值0呢还是如何处理比较好？
							if(ia_fn>=0x296ffe)
								ia_fn=0;
							else
								ia_fn++;
						}
						else
						{
					//		ia_fn=newest_fn;
							//这个情况是ia_fn太超前了，给ia_fn赋值newest_fn还是等待？
						}
					}
					else
					{
						if(ia_fn>=0x296ffe)
							ia_fn=0;
						else
							ia_fn++;
						//这个情况说明ia_fn比最新的fn小，应该是丢数据了？这种情况下给ia_fn+1?
					}
					if (server_conf.debug)
						printf("cannot find fnr=%d ts=%d\n", ctx.fn, timeslot);
					goto get_burst;
				}
				else //success get a burst
				{

				//	fifo->erase_burst(fcch_arfcn, ia_fn, timeslot);
					printf("fn %d ia_mfn  %d fpos %d\n",ia_fn,ia_mfn,fpos);
					if (fpos == 0)
						memset(ts_ctx, 0, sizeof(*ts_ctx));
					GS_group_process(ts_ctx, fpos, burstbits);

					printf("fpos %d burst_count %d \n",fpos,ts_ctx->burst_count);
					fflush(stdout);
					if ((fpos == 3) && (ts_ctx->burst_count == 4))
					{
						ts_ctx->burst_count=0;
						//ts_ctx->burst_count=0;
//						printf("decode!!!\n");
					//		fflush(stdout);
						gsm_frame = decode_cch(&ctx, ts_ctx->burst,(unsigned int*) &len);
						memset(ts_ctx, 0, sizeof(*ts_ctx));
						if (gsm_frame == NULL)
						{
							memset(ts_ctx, 0, sizeof(*ts_ctx));
							if (server_conf.debug)
								printf("cannot decode fnr=0x%08x ts=%d\n", ctx.fn, timeslot);
						}
						else
						{
							if((*(gsm_frame+2)==0x3f)||(*(gsm_frame+2)==0x39))//IA found
							{
								store_ia_data(gsm_frame);
								//if (server_conf.debug)
									printf("==========================IA add~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
									printf("==========================IA add~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
									printf("==========================IA add~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
									printf("==========================IA add~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
									printf("==========================IA add~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
									printf("==========================IA add~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
									printf("==========================IA add~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
									printf("==========================IA add~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~`\n");
									fflush(stdout);
							}
							if (server_conf.output_allframes) {
								output_xml(gsm_frame, fcch_arfcn, (LOGICAL_CHANNEL_TYPE)CCCH, ia_fn-3, ia_fn%51,timeslot);
							}//output_xml(gsm_frame, fcch_arfcn, CCCH,ia_fn-3, ia_fn%51,timeslot);

							if (server_conf.print_octets) {
								print_frame(gsm_frame);
							}
						}
					} //if (fpos == 3 && ts_ctx->burst_count == 4)
				}
			}
//			else
//			{
//				fifo->erase_burst(fcch_arfcn, ia_fn, timeslot);
//				memset(ts_ctx, 0, sizeof(*ts_ctx));
//			}
		}// end of for loop
		if(ia_fn>=0x296ffe)
			ia_fn=0;
		else
			ia_fn++;
	usleep(1) ;
	}
	printf("ia_thread_end!\n");
	pthread_exit (NULL);
}

void *burst_process_thread(void *) {
    unsigned int len;
    int ret = 0;
    unsigned char *gsm_frame;
    CHANNEL_DESC* channel_desc_t;
    CHANNEL_DESC_MAP::iterator iter_chan_desc, iter_chan_desc1;
    CHANNEL_DESC_MAP temp_channel_desc_map;
    CHANNEL_DESC *pstchannel_desc;

    while (true) {
//    	printf("burst_process_thread \n");
        if(g_itotal_arfcn)
        {
            pthread_mutex_lock(&mutex_channeldescmap);
            temp_channel_desc_map.clear();
            temp_channel_desc_map = g_channeldescmap;
            pthread_mutex_unlock(&mutex_channeldescmap);
            if(temp_channel_desc_map.size()>0)
            {
            int itemp = 0 ;
            int isize = temp_channel_desc_map.size();
            for (iter_chan_desc = temp_channel_desc_map.begin();
                    iter_chan_desc != temp_channel_desc_map.end();
                    iter_chan_desc++)
            {
            	//if (server_conf.debug)
            		printf("ia in");
            	ret = assemble_scch(iter_chan_desc, gsm_frame);
               // if (server_conf.debug)
                	printf("  [ret = %d]  ",ret) ;
                if (ret == 100) {
                    //The channel communication is completed!
                    pthread_mutex_lock(&mutex_channeldescmap);
                    pstchannel_desc =
                            (CHANNEL_DESC *) &((*iter_chan_desc).first);
                    iter_chan_desc1 = g_channeldescmap.find(*pstchannel_desc);
                    if (iter_chan_desc1 != g_channeldescmap.end()) {
                        g_channeldescmap.erase(iter_chan_desc1);
                    }
                    pthread_mutex_unlock(&mutex_channeldescmap);
                }
                else {
                    //The channel communication is to continue!
                    pthread_mutex_lock(&mutex_channeldescmap);
                    pstchannel_desc =(CHANNEL_DESC *) &((*iter_chan_desc).first);
                    iter_chan_desc1 = g_channeldescmap.find(*pstchannel_desc);
                    if (iter_chan_desc1 != g_channeldescmap.end())
                    {
                        (*iter_chan_desc1).second.decoded_frames =
                                (*iter_chan_desc).second.decoded_frames;
                        (*iter_chan_desc1).second.passed_times =
                                (*iter_chan_desc).second.passed_times;
                        (*iter_chan_desc1).second.missed_frames =
                                (*iter_chan_desc).second.missed_frames;
                    }
                    pthread_mutex_unlock(&mutex_channeldescmap);
                }
              //  if (server_conf.debug)
                	printf("no = %d  size = %d\n",itemp , isize) ;
                itemp ++ ;
            } //for
            temp_channel_desc_map.clear();
           }
        }
        usleep(1);

    } //while

    pthread_exit (NULL);
}

/*
 * burstdata_maintain_thread
 * add by hello
 * this thread used to delete the burst data in MAP and channel dcs  which are timeout(10S).
 */


void *burstdata_maintain_thread(void *)
{
	int i ;
	BURSTMAP::iterator iter_burst ;
	CHANNEL_DESC_MAP::iterator iter_chan_desc;
	
	time_t now;
	int inowtime ; 

	while(1)
	{
		time(&now);
		inowtime = (int)now ; 
		inowtime -= 20 ;
		//delete burst data
		for(i=0;i<8;i++)
		{
			pthread_mutex_lock(&mutex_burstmap[i]) ;

			for(iter_burst = g_burstmap[i].begin() ; iter_burst!= g_burstmap[i].end() ; )
			{
				if((*iter_burst).second.icurtime < inowtime)
				{
					g_burstmap[i].erase(iter_burst++) ;
				}
				else
				{
					++iter_burst ; 
				}
			}
			
			pthread_mutex_unlock(&mutex_burstmap[i]) ;
		}

		
		//delete mutex_channel des data
		pthread_mutex_lock(&mutex_channeldescmap);
		for(iter_chan_desc = g_channeldescmap.begin(); iter_chan_desc!= g_channeldescmap.end(); )
		{
			if((*iter_chan_desc).first.icurtime<inowtime)
			{
				g_channeldescmap.erase(iter_chan_desc++) ;
			}
			else
			{
				++iter_chan_desc;
			}
		}
		pthread_mutex_unlock(&mutex_channeldescmap);

		sleep(10) ;
	}

	pthread_exit(NULL);
}
/*
 * deal_pdu_to_sms
 * add by ddcat
 * this thread used for deal decoded gsm-packet data to sms
 */

/*
void *deal_pdu_to_sms(void *)
{

	while (true)
	{
		dealpdutosms() ;
		usleep(1) ;
	}

	pthread_exit(NULL);
}
*/


/*
 * Input C0 timeslot 0 bursts
 *
 *
 */
int get_system_info_frame(unsigned char* burstbits, int mfn) {

	struct gs_ts_ctx* ts_ctx = &ctx.ts_ctx[0];

	if (d_mfn == 2) {
		GS_group_process(ts_ctx, 0, burstbits);
	} else {

		for (int i=3; i<=5; ++i) {
			if (d_mfn == i) {
				GS_group_process(ts_ctx, i-2, burstbits);
			}
		}
	}
	return ts_ctx->burst_count;
}

short get_ccch_conf(unsigned char *gsm_frame)
{
	short ccch_conf_tmp;

	ccch_conf_tmp=(*(gsm_frame+10))&0x07;//get ccch_conf!
	ccch_conf.is_combined_with_SDCCHs=0;
	if(ccch_conf_tmp==0)
		return physical_channel_1_not_combined;
	else if(ccch_conf_tmp==1)
	{
		ccch_conf.is_combined_with_SDCCHs=1;
		return physical_channel_1_combined;
	}
	else if(ccch_conf_tmp==2)
		return physical_channel_2_not_combined;
	else if(ccch_conf_tmp==4)
		return physical_channel_3_not_combined;
	else if(ccch_conf_tmp==6)
		return physical_channel_4_not_combined;
	else
		return 0;
}

int decode_ia(unsigned char *gsm_frame)  //return sizeof queue
{

	if(*(gsm_frame+2)==0x3f)
	{
		if((*(gsm_frame+3)&0xf0)!=0x00)
		{
			//packet channel description
			return 0;
		}
		else
		{
			if (server_conf.debug)
				printf("store!!IA!!\n");

			for(int j=0;j<4;j++)
			{
				if(((*(gsm_frame+4))&0xf8)>>3>=channel_type[3-j])
				{
					ia_data[0]=channel_type[3-j];//channel_type
					ia_data[1]=(((*(gsm_frame+4))&0xf8)>>3)-ia_data[0];//sub_channel
					break;

				}
			}
			ia_data[2]=d_fn-3;//fn
			ia_data[3]=(*(gsm_frame+4))&0x07;//timeslot
			ia_data[4]=((*(gsm_frame+5))&0x10)>>4;//hopping?
			ia_data[8]=*(gsm_frame+10);//time_adv
			if(ia_data[4]==1)
			{
				ia_data[6]=((*(gsm_frame+6))&0xc0)>>6;
				ia_data[6]|=(((*(gsm_frame+5))&0x03)<<2);//MAIO
				ia_data[7]=(*(gsm_frame+6))&0x3f;//HSN
				ia_data[9]=*(gsm_frame+11);//length of Mobile Allocation
				for(int ma=0;ma<ia_data[9];ma++)
					ia_data[10+ma]=*(gsm_frame+12+ma);//Mobile Allocation
			}
			else
			{
				ia_data[5]=*(gsm_frame+6);
				ia_data[5]|=(((*(gsm_frame+5))&0x03)<<8);//arfcn
			}
			return 1;
		}
	}
	else if(*(gsm_frame+2)==0x39)
	{
		//IA Extended
		if(*(gsm_frame+3)==0x01)
		{
			//channel_des1
			for(int j=0;j<4;j++)
			{
				//printf("store!!IA!!1\n");
				if(((*(gsm_frame+4))&0xf8)>>3>=channel_type[3-j])
				{
					ia_data[0]=channel_type[3-j];//channel_type
					ia_data[1]=(((*(gsm_frame+4))&0xf8)>>3)-ia_data[0];//sub_channel
					break;
				}
			}
			ia_data[2]=d_fn-3;//fn
			ia_data[3]=(*(gsm_frame+4))&0x07;//timeslot
			ia_data[4]=((*(gsm_frame+5))&0x10)>>4;//hopping?
			ia_data[8]=*(gsm_frame+10);//time_adv
			if(ia_data[4]==1)
			{
				ia_data[6]=((*(gsm_frame+6))&0xc0)>>6;
				ia_data[6]|=(((*(gsm_frame+5))&0x03)<<2);//MAIO
				ia_data[7]=(*(gsm_frame+6))&0x3f;//HSN
				ia_data[9]=*(gsm_frame+18);//length of Mobile Allocation
				for(int ma=0;ma<ia_data[9];ma++)
					ia_data[10+ma]=*(gsm_frame+19+ma);//Mobile Allocation
			}
			else
			{
				ia_data[5]=*(gsm_frame+6);
				ia_data[5]|=(((*(gsm_frame+5))&0x03)<<8);//arfcn
			}
			//channel_des2
			for(int j=0;j<4;j++)
			{
				//printf("store!!IA!!1\n");
				if(((*(gsm_frame+11))&0xf8)>>3>=channel_type[3-j])
				{
					ia_data_ex[0]=channel_type[3-j];//channel_type
					ia_data_ex[1]=(((*(gsm_frame+11))&0xf8)>>3)-ia_data_ex[0];//sub_channel
					break;
				}
			}
			ia_data_ex[2]=d_fn-3;//fn
			ia_data_ex[3]=(*(gsm_frame+11))&0x07;//timeslot
			ia_data_ex[4]=((*(gsm_frame+12))&0x10)>>4;//hopping?
			ia_data_ex[8]=*(gsm_frame+17);//time_adv
			if(ia_data_ex[4]==1)
			{
				ia_data_ex[6]=((*(gsm_frame+13))&0xc0)>>6;
				ia_data_ex[6]|=(((*(gsm_frame+12))&0x03)<<2);//MAIO
				ia_data_ex[7]=(*(gsm_frame+13))&0x3f;//HSN
				ia_data_ex[9]=*(gsm_frame+18);//length of Mobile Allocation
				for(int ma=0;ma<ia_data_ex[9];ma++)
					ia_data_ex[10+ma]=*(gsm_frame+19+ma);//Mobile Allocation
			}
			else
			{
				ia_data_ex[5]=*(gsm_frame+13);
				ia_data_ex[5]|=(((*(gsm_frame+12))&0x03)<<8);//arfcn
			}
			return 2;

		}
		else
		{
			return 0;
		}
		return 2;
	}
	return 0;
}

void store_ia_data(unsigned char *gsm_frame)
{
	int ia_type=0;
	ia_type=decode_ia(gsm_frame);
	int re;
	time_t now;

	CHANNEL_DESC channel_desc,channel_desc_ex;
	_CHANNEL_DESC_DATA channel_desc_data,channel_desc_data_ex;
	if(ia_type==0)
	{
		if (server_conf.debug)
			printf("IA error\n");
	}
	if(ia_type==1)
	{
		if (server_conf.debug)
			printf("IA!!!");
		fflush(stdout);
		channel_desc.channel_type=(Channel_type)ia_data[0];
		if(channel_desc.channel_type!=8)
			return;
		channel_desc.subchannel=(short int)ia_data[1];
		channel_desc.fn=ia_data[2];
		channel_desc.timeslot=(short int)ia_data[3];
		channel_desc.time_advance=(short int)ia_data[8];
		channel_desc.hopping=(bool)ia_data[4];
		channel_desc_data.decoded_frames =0;
		channel_desc_data.missed_frames=0;
		channel_desc_data.passed_times =0;
		time(&now);  
		channel_desc.icurtime = (int)now ;
		if(channel_desc.hopping==true)
		{
			channel_desc.maio=(short int)ia_data[6];
			channel_desc.hsn=(short int)ia_data[7];
			channel_desc.mobile_len=(short int)ia_data[9];
			channel_desc.mobile_allocation=(unsigned char *)(ia_data+10);
			channel_desc.arfcn=-1;
		}
		else
		{
			channel_desc.arfcn=ia_data[5];
			channel_desc.maio=-1;
			channel_desc.hsn=-1;
		}
		is_exist_ia(channel_desc.arfcn, channel_desc.timeslot, channel_desc.subchannel,channel_desc.hopping ,channel_desc.maio ,channel_desc.hsn);
		re = add_channel_desc(channel_desc,channel_desc_data);
		if (server_conf.debug)
		{
			printf("Add IA @%d channel_type:%d arfcn:%d \n", channel_desc.fn ,channel_desc.channel_type,channel_desc.arfcn);
			printf("Number of IA: %d \n", re);
			fflush(stdout);
		}
	}
	if(ia_type==2)
	{
		printf("IA_EX0!!!");
		fflush(stdout);
		channel_desc.channel_type=(Channel_type)ia_data[0];
		if(channel_desc.channel_type!=8)
			return ;
		channel_desc.subchannel=(short int)ia_data[1];
		channel_desc.fn=ia_data[2];
		channel_desc.timeslot=(short int)ia_data[3];
		channel_desc.time_advance=(short int)ia_data[8];
		channel_desc.hopping=(bool)ia_data[4];
		channel_desc_data.decoded_frames =0;
		channel_desc_data.missed_frames=0;
		channel_desc_data.passed_times =0;
		if(channel_desc.hopping==true)
		{
			channel_desc.maio=(short int)ia_data[6];
			channel_desc.hsn=(short int)ia_data[7];
			channel_desc.mobile_len=(short int)ia_data[9];
			channel_desc.mobile_allocation=(unsigned char *)(ia_data+10);
		}
		else
		{
			channel_desc.arfcn=ia_data[5];
			channel_desc.maio=-1;
			channel_desc.hsn=-1;
		}

		if (server_conf.debug)
			printf("IA_EX1!!!");
		channel_desc_ex.channel_type=(Channel_type)ia_data[0];
		if(channel_desc_ex.channel_type!=8)
			return ;
		channel_desc_ex.subchannel=(short int)ia_data[1];
		channel_desc_ex.fn=ia_data[2];
		channel_desc_ex.timeslot=(short int)ia_data[3];
		channel_desc_ex.time_advance=(short int)ia_data[8];
		channel_desc_ex.hopping=(bool)ia_data[4];
		channel_desc_data_ex.decoded_frames =0;
		channel_desc_data_ex.missed_frames=0;
		channel_desc_data_ex.passed_times =0;
		if(channel_desc.hopping==true)
		{
			channel_desc_ex.maio=(short int)ia_data[6];
			channel_desc_ex.hsn=(short int)ia_data[7];
			channel_desc_ex.mobile_len=(short int)ia_data[9];
			channel_desc_ex.mobile_allocation=(unsigned char *)(ia_data+10);
		}
		else
		{
			channel_desc_ex.arfcn=ia_data[5];
			channel_desc.maio=-1;
			channel_desc.hsn=-1;
		}
		is_exist_ia(channel_desc.arfcn, channel_desc.timeslot, channel_desc.subchannel,channel_desc.hopping ,channel_desc.maio ,channel_desc.hsn);
		is_exist_ia(channel_desc_ex.arfcn, channel_desc_ex.timeslot, channel_desc_ex.subchannel,channel_desc_ex.hopping ,channel_desc_ex.maio ,channel_desc_ex.hsn);
		re = add_channel_desc(channel_desc,channel_desc_data);
		re = add_channel_desc(channel_desc_ex,channel_desc_data_ex);
		if (server_conf.debug)
		{
			printf("Add IA @%d channel_type:%d\n", channel_desc.fn ,channel_desc.channel_type);
			printf("Number of IA: %d \n", re);
			printf("Add IA @%d channel_type:%d\n", channel_desc_ex.fn ,channel_desc_ex.channel_type);
			printf("Number of IA: %d \n", re);
			fflush(stdout);
		}
	}

}



void output_xml(unsigned char* gsm_frame, int arfcn,LOGICAL_CHANNEL_TYPE logicalchannel,int fn,int mfn,int timeslot=9)
{
	FILE *pFILE = fopen(my_time, "a");
	time_t 			rawtime;
	unsigned char *end = gsm_frame + packet_len;
	unsigned char logicalchannel_num=0;
	struct tm * time_xml;
	time ( &rawtime );
	time_xml = localtime ( &rawtime );
	if(logicalchannel==BCCH)
		logicalchannel_num=80;
	else if(logicalchannel==CCCH)
			logicalchannel_num=96;
	else if(logicalchannel==SDCCH)
			logicalchannel_num=128;
	else if(logicalchannel==SACCH)
	{
			logicalchannel_num=112;
			end-=2;
	}
	else if(logicalchannel==unknown)
			logicalchannel_num=0;
	fprintf(pFILE,"<l1 direction=\"down\" physicalchannel=\"%d\" logicalchannel=\"%d\" sequence=\"%d\" error=\"0\" timeshift=\"0\" bsic=\"5\" data=\"", \
			arfcn,logicalchannel_num,fn);
	while (gsm_frame < end)
	{
		fprintf(pFILE,"%.02x",*gsm_frame);
		gsm_frame++;
	}
	fprintf(pFILE,"\" timeslot=\"%d\" mfn= \"%d\" time= \"%s\">",timeslot,mfn,asctime (time_xml));
	fprintf(pFILE,"\n</l1>\n");
	fclose(pFILE);
}


/**
 *  Being called by TCP server, i.e. a burst is received
 *	used fo store burst into maps
 */
void do_process(unsigned char* burstbits, int arfcn, int timeslot, int fn) {

	LOGICAL_CHANNEL_TYPE lch_type;
	BURSTKEY burstkey;
	int ret;
	unsigned int len;
	unsigned char *gsm_frame;
	unsigned int last_arfcn=0;
	struct gs_ts_ctx* ts_ctx;
	int fpos = -1;
	unsigned char *IA_frame;
	d_mfn = fn % 51;
	d_tc = fn / 51 % 8;
	d_fn = burstkey.ifn = fn ;
	burstkey.iarfcn = arfcn;
	int physical_channel_num =0;

	ARFCNSET::iterator iter_set;
	pthread_mutex_lock(&mutex_physical_channel_num);
	physical_channel_num=ccch_conf.physical_channel_num;
	pthread_mutex_unlock(&mutex_physical_channel_num);

	if((cell_chan_dec==0) && (arfcn == fcch_arfcn))
	{
		if (timeslot == 0) {
			if(get_system_info_frame(burstbits, d_mfn) == 4) {
				ts_ctx = &ctx.ts_ctx[0];
				gsm_frame = decode_cch(&ctx, ts_ctx->burst, &len);
				if (gsm_frame == NULL) {
					memset(ts_ctx, 0, sizeof(*ts_ctx));
					return;
				}
				else
				{
					if(*(gsm_frame+2)==0x19)//system info 1 found!!!!
					{
						output_xml(gsm_frame, arfcn, (LOGICAL_CHANNEL_TYPE)BCCH,d_fn, d_fn%51,timeslot);
						if((*(gsm_frame+3)&0xC0)==0x00)
						{
							if (server_conf.debug)
									printf("bit map 0\n");
							if(server_conf.autoarfcn)
							{
								decode_bit_map_0(gsm_frame);
								printf("\n");
								fflush(stdout);
								exit(0);
							}
							cell_chan_dec=1;
						}
						else if((*(gsm_frame+3)&0xC8)==0x80)
						{
							if (server_conf.debug)
								printf("bit map 1024\n");
							if(server_conf.autoarfcn)
							{
								 decode_bit_map_1024(gsm_frame);
								 printf("\n");
								 fflush(stdout);
								 exit(0);
							}
						cell_chan_dec=1;
						}
						else if((*(gsm_frame+3)&0xCE)==0x88)
						{
							if (server_conf.debug)
								printf("bit map 512\n");
							if(server_conf.autoarfcn)
							{
								decode_bit_map_512(gsm_frame);
								printf("\n");
								fflush(stdout);
								exit(0);
							}
						cell_chan_dec=1;
						}
						else if((*(gsm_frame+3)&0xCE)==0x8A)
						{


							//unsigned char count=0;
							if (server_conf.debug)
								printf("bit map 256\n");
							if(server_conf.autoarfcn)
							{
								decode_bit_map_256(gsm_frame);
								printf("\n");
								fflush(stdout);
								exit(0);
							}
						cell_chan_dec=1;
						}
						else if((*(gsm_frame+3)&0xCE)==0x8C)
						{
							if (server_conf.debug)
								printf("bit map 128\n");
							if(server_conf.autoarfcn)
							{
								decode_bit_map_128(gsm_frame);
								printf("\n");
								fflush(stdout);
								exit(0);
							}
						cell_chan_dec=1;
						}
						else if((*(gsm_frame+3)&0xCE)==0x8E)
						{
							if(server_conf.autoarfcn)
							{
								decode_bit_map_Variable(gsm_frame);
								printf("\n");
								fflush(stdout);
								exit(0);
							}
							cell_chan_dec=1;
						}
					}
				} // if gsm_frame == null
				memset(ts_ctx, 0, sizeof(*ts_ctx));
			}

		}
	}
	else
	{
		if (physical_channel_num == -1) { // system info 3 not yet found
			if (timeslot == 0 && arfcn == fcch_arfcn) {
				if (get_system_info_frame(burstbits, d_mfn) == 4) {
					ts_ctx = &ctx.ts_ctx[0];
					gsm_frame = decode_cch(&ctx, ts_ctx->burst, &len);
					if (gsm_frame == NULL) {
						memset(ts_ctx, 0, sizeof(*ts_ctx));
						return;
					}
					else
					{

						if(*(gsm_frame+2)==0x1b)//system info 3 found!!!!
						{
//							output_xml(gsm_frame, arfcn, BCCH, d_fn, d_fn%51,timeslot);
							if (server_conf.debug)
								printf("system info 3 found!!!!");
							pthread_mutex_lock(&mutex_physical_channel_num);
							ccch_conf.physical_channel_num =get_ccch_conf(gsm_frame);//get ccch-conf value
							if(ccch_conf.physical_channel_num < 0)//if ccch-conf value wrong?
								ccch_conf.physical_channel_num=0;
							else {
								for (int j=0; j<ccch_conf.physical_channel_num; ++j) {
									ccch_conf.is_ccch_ts[j*2]=true;
								}
							}
							pthread_mutex_unlock(&mutex_physical_channel_num);
						} else {
						}

					} // if gsm_frame == null
					memset(ts_ctx, 0, sizeof(*ts_ctx));
				}
			}
		}
		else
		{ // store all data to MAP
			fifo->add_burst(arfcn, fn, timeslot, burstbits);
			if(timeslot==7)//stroe the most recently received fn.
			{
				pthread_mutex_lock(&mutex_arfcnnum);
				store_fn(arfcn,fn);
				pthread_mutex_unlock(&mutex_arfcnnum);
			}
		}
	}
	usleep(1);


}


LOGICAL_CHANNEL_TYPE get_logical_channeltype(int arfcn, int timeslot, int fn, int &burst_position) {

	bool is_ccch = false;

	int mfn = fn % 51;
	short tcchn = sizeof(CCH_FRAMES)/sizeof(unsigned);

	if (arfcn == fcch_arfcn) { // FCCH ARFCN
		if (timeslot == 0) {
			if (mfn == 2) {
				burst_position = 0;
				return BCCH;
			} else if (mfn == 3) {
				burst_position = 1;
				return BCCH;
			} else if (mfn == 4) {
				burst_position = 2;
				return BCCH;
			} else if (mfn == 5) {
				burst_position = 3;
				return BCCH;
			}
		} else { // FCCH arfcn , but timeslot != 0
			//check other timeslot for CCCH
			if (ccch_conf.physical_channel_num > 1) {
				for(int i = 1; i< ccch_conf.physical_channel_num; ++i) {
					int ts = i*2;

					return CCCH;
				}
			}
		}
	}
	return unknown;
}

/**
 * return position[0..3] burst in a gsm ccch frame
 * -1 for for not ccch
 * otherwise, return burst position,
 */
int is_ccch_fframes(int mfn) {

	int _n = sizeof(CCH_FRAMES)/sizeof(unsigned); //num of items in the array
	//can add codes for checking for NOT bcch frames
	for (int j=0; j<_n; ++j) {
		if (mfn == CCH_FRAMES[j]) {
			return CCH_POS[j];
		}
	}
	return -1;
}

int is_sdcch_fframes(int mfn) {

	int _n = sizeof(SDCCH_FRAMES)/sizeof(unsigned);
	//can add codes for checking for NOT bcch frames
	for (int j=0; j<_n; ++j) {
		if (mfn == SDCCH_FRAMES[j]) {
			return SDCCH_POS[j];
		}
	}
	return -1;
}

int is_sdcch_up_fframes(int mfn) {

	int _n = sizeof(SDCCH_FRAMES_UP)/sizeof(unsigned);
	//can add codes for checking for NOT bcch frames
	for (int j=0; j<_n; ++j) {
		if (mfn == SDCCH_FRAMES_UP[j]) {
			return SDCCH_POS[j];
		}
	}
	return -1;
}


bool is_first_burst(LOGICAL_CHANNEL_TYPE t, int fn) {

	return false;
}



void print_burst(unsigned char* b) {
	for(int k =0;k<BURST_SIZE;k++) {
		printf("%d",*(b+k));
	}
	printf("\n");
}

/**
 * Take care not to follow decode_cch() directly
 * as it might returns NULL for fail decoding,
 * it's a resaon for segmenation fault.
 */
void print_frame(unsigned char* gsm_frame) {

	out_gsmdecode2(gsm_frame, 23);
}

void store_fn(int arfcn,int fn)
{
	for(int t_arfcn=0;t_arfcn<g_itotal_arfcn;t_arfcn++)
	{
		if(d_arfcn_list[t_arfcn]==arfcn)
		{
			arfcn_fn[t_arfcn]=fn;
		}
	}
}

int latest_sync_fn() {

	int _latest_sync_fn=arfcn_fn[0];
	for(int i=1;i<d_arfcn_list.size();i++) {
		if (arfcn_fn[i] < _latest_sync_fn)
			_latest_sync_fn=arfcn_fn[i];
	}
	return _latest_sync_fn;
}

int get_arfcn_fn(int arfcn)
{

	for(int t_arfcn=0;t_arfcn<g_itotal_arfcn;t_arfcn++)
	{
		//fprintf(stderr," g_itotal_arfcn %d arfcn %d \n",g_itotal_arfcn,d_arfcn_list[t_arfcn]);
		if(d_arfcn_list[t_arfcn]==arfcn)
		{
			return arfcn_fn[t_arfcn];
		}
	}

}


void decode_bit_map_0(unsigned char *gsm_frame)
{

	for(int co=0;co<16;co++)
	{
		for(int j=0;j<8;j++)   //Count the 1 in N
		{
			if(gsm_frame[co+3]&(1<<j))
			{
				if(server_conf.autoarfcn)
				{
					if((fcch_arfcn!=(((15-co)*8)+j+1))&&((((15-co)*8)+j+1)<125))
					{
						//pthread_mutex_lock(&mutex_arfcnnum);
						d_arfcn_list.push_back(((15-co)*8)+j+1);
						//pthread_mutex_unlock(&mutex_arfcnnum);
					}
				}
			}
		}
	}
	g_itotal_arfcn = d_arfcn_list.size();
	pthread_mutex_unlock(&mutex_arfcnnum);
	printf("Number of input arfcn:%d\n",(int)d_arfcn_list.size());
	printf("List of ARFCN(s)->>");
	BOOST_FOREACH(int arfcn, d_arfcn_list)
	{
		printf("%d ",arfcn);
		g_itotal_arfcn = d_arfcn_list.size();
	}
	fflush(stdout);
	sort(d_arfcn_list.begin(),d_arfcn_list.end());

}
void decode_bit_map_1024(unsigned char *gsm_frame)
{
	unsigned int w[17];
	w[0]=(*(gsm_frame+3)&0x03)<<8;
	w[0]|=*(gsm_frame+4);
	w[1]=w[0];
	pthread_mutex_unlock(&mutex_arfcnnum);
	if(w[0]!=fcch_arfcn)
		d_arfcn_list.push_back(w[0]);
	if(w[1]!=0)
	{
		w[2]=*(gsm_frame+5)<<1;
		w[2]|=((*(gsm_frame+6)&0x80)>>7);
	}
	if(w[2]!=0)
	{
		int tmp;
		tmp=(w[1] - 512 + w[2]) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[3]=(*(gsm_frame+6)&0x7f)<<2;
		w[3]|=((*(gsm_frame+7)&0xC0)>>6);
	}
	if(w[3]!=0)
	{

		int tmp;
		tmp= (w[1]+ w[3]) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[4]=((*(gsm_frame+7)&0x3f)<<2);
		w[4]|=((*(gsm_frame+8)&0xc0)>>6);
	}
	if(w[4]!=0)
	{
		int tmp;
		//	│    F4 = (W1 - 512 + (W2 - 256 + W4) smod 511) smod 1023 │
		tmp=(w[1] - 512 + (w[2] - 256 + w[4]) % 511) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[5]=((*(gsm_frame+8)&0x3f)<<2);
		w[5]|=((*(gsm_frame+9)&0xc0)>>6);
	}
	if(w[5]!=0)
	{
		int tmp;
		tmp=(w[1]+ (w[3] - 256 + w[5]) % 511) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[6]=((*(gsm_frame+9)&0x3f)<<2);
		w[6]|=((*(gsm_frame+10)&0xc0)>>6);
	}
	if(w[6]!=0)
	{
		int tmp;

		tmp= (w[1] - 512 + (w[2]+ w[6]) % 511) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[7]=*(gsm_frame+10)&0x3f<<2;
		w[7]|=((*(gsm_frame+11)&0xc0)>>6);
	}
	if(w[7]!=0)
	{

		int tmp;
		tmp = (w[1]       + (w[3]       + w[7]) % 511) % 1023 ;

		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[8]=((*(gsm_frame+11)&0x3f)<<1);
		w[8]|=((*(gsm_frame+12)&0x80)>>7);
	}
	if(w[8]!=0)
	{
		int tmp;
		tmp= (w[1] - 512 + (w[2] - 256 + (w[4] - 128 + w[8] )% 255)%511)%1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[9]=(*(gsm_frame+12)&0x7f);
	}
	if(w[9]!=0)
	{
		int tmp;
		tmp=(w[1]+ (w[3] - 256 + (w[5] - 128 + w[9] )% 255) % 511) % 1023;
		d_arfcn_list.push_back(tmp);
		w[10]=(*(gsm_frame+13)&0xfe)>>1;
	}
	if(w[10]!=0)
	{

		int tmp;
		tmp=(w[1] - 512 + (w[2]       + (w[6] - 128 + w[10])% 255) % 511) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[11]=(*(gsm_frame+13)&0x01)<<6;
		w[11]=(*(gsm_frame+14)&0xfc)>>2;
	}
	if(w[11]!=0)
	{
		int tmp;
		tmp=(w[1]+ (w[3]+ (w[7] - 128 + w[11])% 255) % 511) %1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[12]=(*(gsm_frame+14)&0x03)<<5;
		tmpe=(*(gsm_frame+15)&0xf8)>>3;
		w[12]|=tmpe;
	}
	if(w[12]!=0)
	{
		int tmp;
		tmp=(w[1]-512 + (w[2] - 256 + (w[4]+ w[12])% 255) % 511) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[13]=(*(gsm_frame+15)&0x07)<<4;
		tmpe=(*(gsm_frame+16)&0xf0)>>4;
		w[13]|=tmpe;
	}
	if(w[13]!=0)
	{
		int tmp;
		tmp=(w[1]+ (w[3] - 256 + (w[5] + w[13])% 255)% 511) %1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[14]=(*(gsm_frame+16)&0x0f)<<4;
		w[14]|=(*(gsm_frame+17)&0xe0)>>4;
	}
	if(w[14]!=0)
	{
		int tmp;
		tmp= (w[1] - 512 + (w[2]+ (w[6]+ w[14])%255)%511)%1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[15]=(*(gsm_frame+17)&0x8f)<<2;
		w[15]=(*(gsm_frame+18)&0xc0)>>6;
	}
	if(w[15]!=0)
	{
		int tmp;
		tmp=(w[1] + (w[3] + (w[7]+ w[15]) % 255) % 511) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[16]=(*(gsm_frame+18)&0x3f);
	}
	if(w[16]!=0)
	{
		int tmp;
		tmp=(w[1] - 512 + (w[2] - 256 + (w[4] - 128 + (w[8] - 64 + w[16]) % 127) % 255) % 511) % 1023;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
	}
	g_itotal_arfcn = d_arfcn_list.size();
	pthread_mutex_unlock(&mutex_arfcnnum);
	printf("Number of input arfcn:%d\n",(int)d_arfcn_list.size());
	printf("List of ARFCN(s)->>");
	BOOST_FOREACH(int arfcn, d_arfcn_list)
	{
		printf("%d ",arfcn);
		g_itotal_arfcn = d_arfcn_list.size();
	}
	fflush(stdout);
	sort(d_arfcn_list.begin(),d_arfcn_list.end());
}
void decode_bit_map_512(unsigned char *gsm_frame)
{
	unsigned int w[18];
	w[0]=(*(gsm_frame+3)&0x01)<<8;
	w[0]|=*(gsm_frame+4);
	w[0]<<=1;
	w[0]|=((*(gsm_frame+5)&0x80)>>7);
	pthread_mutex_unlock(&mutex_arfcnnum);
	if(w[0]!=fcch_arfcn)
		d_arfcn_list.push_back(w[0]);
	w[1]=(*(gsm_frame+5)&0x7f)<<2;
	w[1]|=((*(gsm_frame+6)&0xC0)>>6);
	if(w[1]!=0)
	{
		if((w[0] + w[1])%1024!=fcch_arfcn)
				d_arfcn_list.push_back((w[0] + w[1])%1024);
		w[2]=(*(gsm_frame+6)&0x3f)<<2;
		w[2]|=((*(gsm_frame+7)&0xC0)>>6);
	}
	if(w[2]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 256 + w[2]) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[3]=(*(gsm_frame+7)&0x3f)<<2;
		w[3]|=((*(gsm_frame+8)&0xC0)>>6);
	}
	if(w[3]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] + w[3]) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[4]=(*(gsm_frame+8)&0x3f)<<1;
		w[4]|=((*(gsm_frame+9)&0x80)>>7);
	}
	if(w[4]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 256 + (w[2] - 128 + w[4]) % 255) % 511)% 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[5]=(*(gsm_frame+9)&0x7f);
	}
	if(w[5]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]+ (w[3] - 128 + w[5]) % 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
		d_arfcn_list.push_back(tmp);
		w[6]=(*(gsm_frame+10)&0xfe)>>1;
	}
	if(w[6]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 256 + (w[2]      + w[6]) % 255) % 511)% 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[7]=(*(gsm_frame+10)&0x01)<<6;
		tmpe=(*(gsm_frame+11)&0xfc)>>2;
		w[7]|=tmpe;
	}
	if(w[7]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]       + (w[3]      + w[7]) % 255) % 511)  % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[8]=(*(gsm_frame+11)&0x03)<<4;
		tmpe=(*(gsm_frame+12)&0xf0)>>4;
		w[8]|=tmpe;
	}
	if(w[8]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 256 + (w[2] - 128 + (w[4] - 64 + w[8] ) % 127)% 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[9]=(*(gsm_frame+12)&0x0f)<<2;
		tmpe=(*(gsm_frame+13)&0xC0)>>6;
		w[9]|=tmpe;
	}
	if(w[9]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]  + (w[3] - 128 + (w[5] - 64 + w[9] ) % 127)% 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[10]=(*(gsm_frame+13)&0x3f);
	}
	if(w[10]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 256 + (w[2]  + (w[6] - 64 + w[10] ) % 127)% 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[11]=(*(gsm_frame+14)&0xfc)>>2;
	}
	if(w[11]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]  + (w[3]+ (w[7] - 64 + w[11] ) % 127)% 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[12]=(*(gsm_frame+14)&0x03)<<4;
		tmpe=(*(gsm_frame+15)&0xf0)>>4;
		w[12]|=tmpe;
	}
	if(w[12]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 256 + (w[2] - 128 + (w[4]  + w[12] ) % 127)% 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[13]=(*(gsm_frame+15)&0x0f)<<2;
		tmpe=(*(gsm_frame+16)&0xc0)>>6;
		w[13]|=tmpe;
	}
	if(w[13]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]  + (w[3] - 128 + (w[5]  + w[13] ) % 127)% 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[14]=(*(gsm_frame+16)&0x3f);
	}
	if(w[14]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 256 + (w[2] + (w[6] + w[14] ) % 127)% 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[15]=(*(gsm_frame+17)&0xfc)>>2;;
	}
	if(w[15]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]  + (w[3] + (w[7]  + w[15] ) % 127)% 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[16]=(*(gsm_frame+17)&0x03)<<3;
		tmpe=(*(gsm_frame+18)&0xe0)>>5;
		w[16]|=tmpe;
		//count=16;
	}
	if(w[16]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 256 + (w[2] - 128 + (w[4] - 64 + (w[8]  - 32 + w[16])% 63) % 127) % 255) % 511) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[17]=(*(gsm_frame+18)&0x1f);
		if(w[17]!=0)
		{
			tmp=(w[0] + (w[1]+ (w[3] - 128 + (w[5] - 64 + (w[9]  - 32 + w[17])% 63) % 127) % 255) % 511) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
		}
	}
	g_itotal_arfcn = d_arfcn_list.size();
	pthread_mutex_unlock(&mutex_arfcnnum);
	printf("Number of input arfcn:%d\n",(int)d_arfcn_list.size());
	printf("List of ARFCN(s)->>");
	BOOST_FOREACH(int arfcn, d_arfcn_list)
	{
		printf("%d ",arfcn);
		g_itotal_arfcn = d_arfcn_list.size();
	}
	fflush(stdout);
	sort(d_arfcn_list.begin(),d_arfcn_list.end());
}

void decode_bit_map_256(unsigned char *gsm_frame)
{

	unsigned int w[22]={0};
		w[0]=(*(gsm_frame+3)&0x01)<<8;
		w[0]|=*(gsm_frame+4);
		w[0]<<=1;
		w[0]|=((*(gsm_frame+5)&0x80)>>7);
		pthread_mutex_unlock(&mutex_arfcnnum);
		if(w[0]!=fcch_arfcn)
			d_arfcn_list.push_back(w[0]);
		w[1]=(*(gsm_frame+5)&0x7f)<<1;
		w[1]|=((*(gsm_frame+6)&0x80)>>7);
		if(w[1]!=0)
		{
			if((w[0] + w[1])%1024!=fcch_arfcn)
			{
				d_arfcn_list.push_back((w[0] + w[1])%1024);
			}
			w[2]=*(gsm_frame+6)&0x7f;
		}
		if(w[2]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + w[2]) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[3]=(*(gsm_frame+7)&0xfe)>>1;
		}
		if(w[3]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] + w[3]) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			tmpe=(*(gsm_frame+7)&0x01);
			tmpe=tmpe<<5;
			w[4]=(*(gsm_frame+8)&0xf8)>>3;
			w[4]|=tmpe;
		}
		if(w[4]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2] - 64 + w[4]) % 127) % 255)% 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[5]=(*(gsm_frame+9)&0xe0)>>5;
			tmpe=(*(gsm_frame+8)&0x07)<<3;
			w[5]|=tmpe;
		}
		if(w[5]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1]       + (w[3] - 64 + w[5]) % 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[6]=(*(gsm_frame+9)&0x1f)<<1;
			tmpe=(*(gsm_frame+10)&0x80)>>7;
			w[6]|=tmpe;
		}
		if(w[6]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2]      + w[6]) % 127) % 255)% 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[7]=(*(gsm_frame+10)&0x7e)>>1;
		}
		if(w[7]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1]       + (w[3]      + w[7]) % 127) % 255)  % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[8]=(*(gsm_frame+10)&0x01)<<4;
			tmpe=(*(gsm_frame+11)&0xf0)>>4;
			w[8]|=tmpe;
			//count=5;
		}
		if(w[8]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2] - 64 + (w[4] - 32 + w[8] ) % 63)% 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[9]=(*(gsm_frame+11)&0x0f)<<1;
			tmpe=(*(gsm_frame+12)&0x80)>>7;
			w[9]|=tmpe;
			//count=9;
		}
		if(w[9]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1]  + (w[3] - 64 + (w[5] - 32 + w[9] ) % 63)% 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[10]=(*(gsm_frame+12)&0x7C)>>2;
		}
		if(w[10]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2]  + (w[6] - 32 + w[10] ) % 63)% 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[11]=(*(gsm_frame+12)&0x03)<<3;
			tmpe=(*(gsm_frame+13)&0xe0)>>5;
			w[11]|=tmpe;
		}
		if(w[11]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1]  + (w[3]+ (w[7] - 32 + w[11] ) % 63)% 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[12]=(*(gsm_frame+13)&0x1f);
		}
		if(w[12]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2] - 64 + (w[4]  + w[12] ) % 63)% 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[13]=(*(gsm_frame+14)&0xf8)>>3;
		}
		if(w[13]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1]  + (w[3] - 64 + (w[5]  + w[13] ) % 63)% 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[14]=(*(gsm_frame+14)&0x07)<<2;
			tmpe=(*(gsm_frame+15)&0xc0)>>6;
			w[14]|=tmpe;
		}
		if(w[14]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2] + (w[6] + w[14] ) % 63)% 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[15]=(*(gsm_frame+15)&0x3E)>>1;;
		}
		if(w[15]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1]  + (w[3] + (w[7]  + w[15] ) % 63)% 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[16]=(*(gsm_frame+15)&0x01)<<3;
			tmpe=(*(gsm_frame+16)&0xe0)>>5;
			w[16]|=tmpe;
		}
		if(w[16]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2] - 64 + (w[4] - 32 + (w[8]  - 16 + w[16])% 31) % 63) % 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[17]=(*(gsm_frame+16)&0x1e)>>1;
		}
		if(w[17]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1]+ (w[3] - 64 + (w[5] - 32 + (w[9]  - 16 + w[17])% 31) % 63) % 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[18]=(*(gsm_frame+16)&0x01)<<3;
			tmpe=(*(gsm_frame+17)&0xe0)>>5;
			w[18]|=tmpe;
		}
		if(w[18]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2]  + (w[6] - 32 + (w[10]  - 16 + w[18])% 31) % 63) % 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[19]=(*(gsm_frame+17)&0x1e)>>1;
		}
		if(w[19]!=0){
			int tmp;
			tmp=(w[0] + (w[1]+ (w[3]+ (w[7] - 32 + (w[11]  - 16 + w[19])% 31) % 63) % 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			unsigned char tmpe;
			w[20]=(*(gsm_frame+17)&0x01)<<3;
			tmpe=(*(gsm_frame+18)&0xe0)>>5;
			w[20]|=tmpe;
		}

		if(w[20]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1] - 128 + (w[2] - 64 + (w[4]  + (w[12]  - 16 + w[20])% 31) % 63) % 127) % 255) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[21]=(*(gsm_frame+18)&0x1e)>>1;
			if(w[21]!=0)
					{
						int tmp;
						tmp=(w[0] + (w[1]+ (w[3] - 64 + (w[5] + (w[13]  - 16 + w[21])% 31) % 63) % 127) % 255) % 1024;
						if(tmp!=fcch_arfcn)
							d_arfcn_list.push_back(tmp);
					}
		}

		g_itotal_arfcn = d_arfcn_list.size();
		pthread_mutex_unlock(&mutex_arfcnnum);
		printf("Number of input arfcn:%d\n",(int)d_arfcn_list.size());
		printf("List of ARFCN(s)->>");
		BOOST_FOREACH(int arfcn, d_arfcn_list)
		{
			printf("%d ",arfcn);
			g_itotal_arfcn = d_arfcn_list.size();
		}
		fflush(stdout);
		sort(d_arfcn_list.begin(),d_arfcn_list.end());
}
void decode_bit_map_128(unsigned char *gsm_frame)
{
	unsigned int w[29];
	w[0]=(*(gsm_frame+3)&0x01)<<8;
	w[0]|=*(gsm_frame+4);
	w[0]<<=1;
	w[0]|=((*(gsm_frame+5)&0x80)>>7);
	pthread_mutex_unlock(&mutex_arfcnnum);
	if(w[0]!=fcch_arfcn)
		d_arfcn_list.push_back(w[0]);

	w[1]=*(gsm_frame+5)&0x7f;
	if(w[1]!=0)
	{
		if((w[0] + w[1])%1024!=fcch_arfcn)
		{
			d_arfcn_list.push_back((w[0] + w[1])%1024);
		}
		w[2]=(*(gsm_frame+6)&0xfc)>>2;
	}
	if(w[2]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + w[2]) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		tmpe=(*(gsm_frame+6)&03)<<4;
		w[3]=(*(gsm_frame+7)&0xf0)>>4;
		w[3]|=tmpe;
	}
	if(w[3]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] + w[3]) % 127)%1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		tmpe=(*(gsm_frame+7)&0x0f)<<1;
		w[4]=(*(gsm_frame+8)&0x80)>>7;
		w[4]|=tmpe;
	}
	if(w[4]!=0)
	{

		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2] - 32 + w[4]) % 63) % 127)% 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[5]=(*(gsm_frame+8)&0x7c)>>2;
	}
	if(w[5]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]       + (w[3] - 32 + w[5]) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[6]=(*(gsm_frame+8)&0x03)<<3;
		tmpe=(*(gsm_frame+9)&0xe0)>>5;
		w[6]|=tmpe;
	}
	if(w[6]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2]      + w[6]) % 63) % 127)% 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[7]=(*(gsm_frame+9)&0x1f);
	}
	if(w[7]!=0)
	{
			int tmp;
			tmp=(w[0] + (w[1]       + (w[3]      + w[7]) % 63) % 127)  % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
			w[8]=(*(gsm_frame+10)&0xf0)>>4;
	}
	if(w[8]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2] - 32 + (w[4] - 16 + w[8] ) % 31)% 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[9]=(*(gsm_frame+10)&0x0f);
	}
	if(w[9]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]  + (w[3] - 32 + (w[5] - 16 + w[9] ) % 31)% 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[10]=(*(gsm_frame+11)&0xf0)>>4;
	}
	if(w[10]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2]  + (w[6] - 16 + w[10] ) % 31)% 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[11]=(*(gsm_frame+11)&0x0f);
	}
	if(w[11]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]  + (w[3]+ (w[7] - 16 + w[11] ) % 31)% 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[12]=(*(gsm_frame+12)&0xf0)>>4;
	}
	if(w[12]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2] - 32 + (w[4]  + w[12] ) % 31)% 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[13]=(*(gsm_frame+12)&0x0f);
	}
	if(w[13]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]  + (w[3] - 32 + (w[5]  + w[13] ) % 31)% 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[14]=(*(gsm_frame+13)&0xf0)>>4;
	}
	if(w[14]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2] + (w[6] + w[14] ) % 31)% 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[15]=(*(gsm_frame+13)&0x0f);
	}
	if(w[15]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]  + (w[3] + (w[7]  + w[15] ) % 31)% 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[16]=(*(gsm_frame+14)&0xe0)>>5;
	}
	if(w[16]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2] - 32 + (w[4] - 16 + (w[8]  - 8 + w[16])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[17]=(*(gsm_frame+14)&0x1c)>>2;
	}
	if(w[17]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]+ (w[3] - 32 + (w[5] - 16 + (w[9]  - 8 + w[17])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[18]=(*(gsm_frame+14)&0x03)<<1;
		tmpe=(*(gsm_frame+15)&0x80)>>7;
		w[18]|=tmpe;
	}
	if(w[18]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2]  + (w[6] - 16 + (w[10]  - 8 + w[18])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[19]=(*(gsm_frame+15)&0x70)>>4;
	}
	if(w[19]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]+ (w[3]+ (w[7] - 16 + (w[11]  - 8 + w[19])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[20]=(*(gsm_frame+15)&0x0e)>>1;
	}
	if(w[20]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1] - 64 + (w[2] - 32 + (w[4]  + (w[12]  - 8 + w[20])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[21]=(*(gsm_frame+15)&0x01)<<2;
		tmpe=(*(gsm_frame+16)&0xc0)>>6;
		w[21]|=tmpe;
	}
	if(w[21]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]+ (w[3] - 32 + (w[5] + (w[13]  - 8 + w[21])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[22]=(*(gsm_frame+16)&0x38)>>3;
	}
	if(w[22]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]-64+ (w[2] + (w[6] + (w[14]  - 8 + w[22])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[23]=(*(gsm_frame+16)&0x07);
	}
	if(w[23]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]+ (w[3] + (w[7] + (w[15]  - 8 + w[23])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[24]=(*(gsm_frame+16)&0xe0)>>5;
	}
	if(w[24]!=0)
	{
		int tmp;

		tmp=(w[0] + (w[1]- 64+ (w[2]- 32 + (w[4] -16+ (w[8]  + w[24])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[25]=(*(gsm_frame+17)&0x1c)>>2;
	}
	if(w[25]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]+ (w[3]- 32 + (w[5] -16+ (w[9]  + w[25])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		unsigned char tmpe;
		w[26]=(*(gsm_frame+17)&0x03)<<1;
		tmpe=(*(gsm_frame+18)&0x80)>>7;
		w[26]|=tmpe;
	}
	if(w[26]!=0)
	{
		int tmp;
		tmp=(w[0] + (w[1]-64+ (w[2]+ (w[6] -16+ (w[10]  + w[26])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[27]=(*(gsm_frame+18)&0x70)>>4;

	}
	if(w[27]!=0)
	{

		int tmp;
		tmp=(w[0] + (w[1]+ (w[3]+ (w[7] -16+ (w[11]  + w[27])% 15) % 31) % 63) % 127) % 1024;
		if(tmp!=fcch_arfcn)
			d_arfcn_list.push_back(tmp);
		w[28]=(*(gsm_frame+18)&0x0e)>>1;
		if(w[28]!=0)
		{
			int tmp;
			tmp=(w[0] + (w[1]-64+ (w[2]- 32 + (w[4]+ (w[12]  + w[28])% 15) % 31) % 63) % 127) % 1024;
			if(tmp!=fcch_arfcn)
				d_arfcn_list.push_back(tmp);
		}

	}
	g_itotal_arfcn = d_arfcn_list.size();
	pthread_mutex_unlock(&mutex_arfcnnum);
	printf("Number of input arfcn:%d\n",(int)d_arfcn_list.size());
	printf("List of ARFCN(s)->>");
	BOOST_FOREACH(int arfcn, d_arfcn_list)
	{
		printf("%d ",arfcn);
		g_itotal_arfcn = d_arfcn_list.size();
	}
	fflush(stdout);
	sort(d_arfcn_list.begin(),d_arfcn_list.end());
}
void decode_bit_map_Variable(unsigned char *gsm_frame)
{
	unsigned int orig_arfcn;
	orig_arfcn=(*(gsm_frame+3)&0x01)<<8;
	orig_arfcn|=*(gsm_frame+4);
	orig_arfcn<<=1;
	orig_arfcn|=((*(gsm_frame+5)&0x80)>>7);
	pthread_mutex_unlock(&mutex_arfcnnum);
	if(orig_arfcn!=fcch_arfcn)
		d_arfcn_list.push_back(orig_arfcn);

	for(int co=5;co<19;co++)
	{
		for(int j=0;j<8;j++)   //Count the 1 in N
		{
			if(gsm_frame[co]&(0x80>>j))
			{
				if((fcch_arfcn!=(orig_arfcn+((co-5)*8+j))%1024)&&(((co-5)*8+j)%1024!=0))
				{
					d_arfcn_list.push_back((orig_arfcn+((co-5)*8+j))%1024);
				}
			}
		}

	}
	g_itotal_arfcn = d_arfcn_list.size();
	//sort(d_arfcn_list.begin(),d_arfcn_list.end());
	pthread_mutex_unlock(&mutex_arfcnnum);
	printf("Number of input arfcn:%d\n",(int)d_arfcn_list.size());
	printf("List of ARFCN(s)->>");
	BOOST_FOREACH(int arfcn, d_arfcn_list)
	{
		printf("%d ",arfcn);
		g_itotal_arfcn = d_arfcn_list.size();
	}
	fflush(stdout);
	sort(d_arfcn_list.begin(),d_arfcn_list.end());
}
