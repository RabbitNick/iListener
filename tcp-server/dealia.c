#include "dealia.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "burstdata.h"

int 	add_channel_desc(CHANNEL_DESC channel_desc , CHANNEL_DESC_DATA channel_desc_data)  //return sizeof queue
{
	int isize ; 
	pthread_mutex_lock(&mutex_channeldescmap) ; 
	
	g_channeldescmap.insert(CHANNEL_DESC_MAP::value_type(channel_desc,channel_desc_data)) ; 
	isize = g_channeldescmap.size() ; 
	
	pthread_mutex_unlock(&mutex_channeldescmap) ; 
	
	return isize ; 
}


int 	num_of_channel_desc() //i.e. size of queue
{
	int isize ; 

	pthread_mutex_lock(&mutex_channeldescmap) ; 
	
	isize = g_channeldescmap.size() ; 
	
	pthread_mutex_unlock(&mutex_channeldescmap) ; 
	
	return isize ; 
}


void is_exist_ia(int arfcn, int timeslot, int sub_channel,int hopping ,int maio ,int hsn)
{
	CHANNEL_DESC_MAP::iterator iter_chan_desc;
	CHANNEL_DESC channel_desc ;
	memset(&channel_desc , 0 , sizeof(CHANNEL_DESC)) ;
	channel_desc.arfcn = arfcn ;
	channel_desc.timeslot = timeslot ;
	channel_desc.subchannel = sub_channel ;
	channel_desc.hopping = hopping ;
	channel_desc.maio = maio ;
	channel_desc.hsn = hsn ;
	pthread_mutex_lock(&mutex_channeldescmap) ;
	for(iter_chan_desc = g_channeldescmap.begin(); iter_chan_desc!= g_channeldescmap.end(); )
	{
		if(hopping==0)
		{
			if(((*iter_chan_desc).first.subchannel==sub_channel)&&((*iter_chan_desc).first.timeslot==timeslot)&&((*iter_chan_desc).first.arfcn==arfcn))
			{
				g_channeldescmap.erase(iter_chan_desc++);
				if (server_conf.debug)
					printf("IA existed!!!\n");
			}
			else
			{
				++iter_chan_desc ;
			}
		}
		else
		{
			if(((*iter_chan_desc).first.subchannel==sub_channel)&&((*iter_chan_desc).first.timeslot==timeslot)&&((*iter_chan_desc).first.maio==maio)&&((*iter_chan_desc).first.hsn==hsn))
			{
				g_channeldescmap.erase(iter_chan_desc++);
				if (server_conf.debug)
					printf("IA existed!!!\n");
			}
			else
			{
			   ++iter_chan_desc ;
			}
		}
	}
	pthread_mutex_unlock(&mutex_channeldescmap) ;

}


bool	remove(CHANNEL_DESC_MAP::iterator iter_channel_desc_map)
{
	bool breturn = true ; 
	g_channeldescmap.erase(iter_channel_desc_map) ; 
	return breturn ; 
}


