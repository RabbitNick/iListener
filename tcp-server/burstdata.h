#ifndef _BURSTDATA_H
#define _BURSTDATA_H

#include <stdio.h>
#include "global.h"
#include <set>
#include <map>
#include <vector>

#include <pthread.h>
#include <unistd.h>

typedef std::set<int>ARFCNSET;

typedef enum
{
	TCHF_ACCHs = 1 ,
	TCHH_ACCHs=2,
	SDCCH4_SACCH4_or_CHCH=4,
	SDCCH8_SACCH8_or_CBCH=8
}Channel_type;

typedef struct _CHANNEL_DESC_DATA
{
	int decoded_frames;
	int passed_times;  //number of times this IA passed in assemble_scch()
	int missed_frames;
}CHANNEL_DESC_DATA ; 

typedef struct _CHANNEL_DESC
{

	Channel_type channel_type;
	int fn;  //frame number of the IA
	short subchannel;
	short timeslot;
	bool hopping;
	int arfcn;
	short training_sequence;
	short maio;
	short hsn;
	short time_advance;
	short mobile_len;
	unsigned char *mobile_allocation;
	int icurtime ; 
	
	friend bool operator < (const _CHANNEL_DESC& stchannel_desc1 , const _CHANNEL_DESC& stchannel_desc2 ) ;  
}CHANNEL_DESC ; 

inline bool operator < (const _CHANNEL_DESC& stchannel_desc1 , const _CHANNEL_DESC& stchannel_desc2 )
{
	if(stchannel_desc1.fn < stchannel_desc2.fn)
	{
		return true ; 
	}
	else if(stchannel_desc1.fn > stchannel_desc2.fn)
	{
		return false ; 
	}
	
	if(stchannel_desc1.arfcn < stchannel_desc2.arfcn )
	{
		return true ; 
	}
	else if(stchannel_desc1.arfcn > stchannel_desc2.arfcn )
	{
		return false ; 
	}
	
	if(stchannel_desc1.timeslot < stchannel_desc2.timeslot )
	{
		return true ; 
	}
	else if(stchannel_desc1.timeslot > stchannel_desc2.timeslot )
	{
		return false ; 
	}
	
	return false ; 
}


//typedef unsigned char BURSTDATA[BURST_SIZE] ; 
typedef struct _BURSTDATA
{
	unsigned char abburstdata[BURST_SIZE] ; 
	int icurtime ; 

}BURSTDATA116 ; 

typedef struct _BURSTKEY
{
	int ifn ; 
  int iarfcn ; 
	
	friend bool operator < (const _BURSTKEY& stburstkey1 , const _BURSTKEY& stburstkey2 ) ;  
}BURSTKEY ; 

typedef struct _STBURSTDATA
{
	int ifn ; 
	int iarfcn ; 
	int itimeslot ; 
	unsigned char abburstdata[BURST_SIZE] ; 
}STBURSTDATA ; 

inline bool operator < (const _BURSTKEY& stburstkey1 , const _BURSTKEY& stburstkey2 )
{
    if(stburstkey1.ifn < stburstkey2.ifn)
    {
    	return true ; 
    }
    else if(stburstkey1.ifn > stburstkey2.ifn)
    {
    	return false ; 
    }
	
	if(stburstkey1.iarfcn < stburstkey2.iarfcn)
    {
    	return true ; 
    }
    else if(stburstkey1.iarfcn > stburstkey2.iarfcn)
    {
    	return false ; 
    }
       
    return false ; 
}; 

typedef std::map<BURSTKEY , BURSTDATA116> BURSTMAP ; 
typedef std::set<BURSTKEY> BURSTKEYSET ; 

typedef std::map<CHANNEL_DESC , CHANNEL_DESC_DATA> CHANNEL_DESC_MAP ;

typedef std::vector<CHANNEL_DESC> CHANNEL_DESC_VEC ; 
	
extern BURSTMAP g_burstmap[8] ;
extern BURSTMAP g_chanmap[8] ;
extern CHANNEL_DESC_MAP g_channeldescmap ;

extern pthread_mutex_t mutex_burstmap[8] ; 
extern pthread_mutex_t mutex_chanmap[8] ;
extern pthread_mutex_t mutex_channeldescmap ; // g_channeldescmap
extern pthread_mutex_t mutex_arfcnnum ;

void init_mutex_burst();
void initial();

#endif 

