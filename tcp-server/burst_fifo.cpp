#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "burst_fifo.h"
#include "burstdata.h"
#include <time.h>

//BURSTMAP g_burstmap[TIMESLOT];
extern CCCH_CONF ccch_conf;
using namespace std ; 

Burst_fifo::Burst_fifo() {

}

Burst_fifo::~Burst_fifo() {

}

int Burst_fifo::add_burst(int arfcn, int fn, int timeslot,unsigned char* bits)
{
	BURSTMAP::iterator iter_burst ; 
	BURSTDATA116 burstdata ; 
	
	BURSTKEY burstkey ; 
	burstkey.ifn = fn ; 
	burstkey.iarfcn = arfcn ; 

	//g_burstmap[itimeslot] lock g_burstmap
	pthread_mutex_lock(&mutex_burstmap[timeslot]) ;
	
	iter_burst = g_burstmap[timeslot].find(burstkey) ; //Are there data in map?
	if(iter_burst != g_burstmap[timeslot].end()) // clean the data which is out of date
	{
		g_burstmap[timeslot].erase(iter_burst) ;
	}



	memcpy(burstdata.abburstdata , bits, BURST_SIZE) ; 
	time_t now;
	time(&now);  
	burstdata.icurtime = (int)now ;

	// add info into MAP
	g_burstmap[timeslot].insert(BURSTMAP::value_type(burstkey , burstdata)) ;

	//write_burst(burstkey.iarfcn, timeslot, burstkey.ifn, burstdata.abburstdata, "burst_fifo.txt");

	// unlock g_burstmap[itimeslot]
	pthread_mutex_unlock(&mutex_burstmap[timeslot]) ;
	
	return 0 ; 
}




void Burst_fifo::erase_burst(int arfcn, int fn, int itimeslot)
{
	if(fn<4)
		return;
	else
	{
		BURSTMAP::iterator iter_burst ;
		BURSTKEY burstkey ;
		burstkey.ifn = fn ;
		burstkey.iarfcn = arfcn ;
		pthread_mutex_lock(&mutex_burstmap[itimeslot]) ;
		iter_burst = g_burstmap[itimeslot].find(burstkey) ;
		if(iter_burst!=g_burstmap[itimeslot].end())
				g_burstmap[itimeslot].erase(iter_burst) ;
		pthread_mutex_unlock(&mutex_burstmap[itimeslot]);
	}
	return;
}






// find nothing return -1��find data return 0
int 	Burst_fifo::retrieve(int arfcn, int fn, int itimeslot , unsigned char *pbburstdata)
{
	BURSTMAP::iterator iter_burst ; 
		
	BURSTKEY burstkey ; 
	burstkey.ifn = fn ; 
	burstkey.iarfcn = arfcn ; 
	unsigned char abburstdata[BURST_SIZE];
	
	// lock g_burstmap[itimeslot]
	pthread_mutex_lock(&mutex_burstmap[itimeslot]) ; 
	
	iter_burst = g_burstmap[itimeslot].find(burstkey) ; //Are there data in map?
	if(iter_burst == g_burstmap[itimeslot].end()) // if not exist��return NULL
	{
		// unlock g_burstmap[itimeslot]
		pthread_mutex_unlock(&mutex_burstmap[itimeslot]) ;
		return -1 ; 
	}
	else
	{
		memcpy(pbburstdata ,  (*iter_burst).second.abburstdata , BURST_SIZE) ; 
	}
	
	// unlock g_burstmap[itimeslot]
	pthread_mutex_unlock(&mutex_burstmap[itimeslot]) ; 
	
	return 0 ;
}

int	Burst_fifo::get_first_fn()
{
	int i ; 
	int ifn_min = 0x296fff ;
	int isize ; 
	
	BURSTMAP::iterator iter_burst ;

	for(i=0;i<8;i++)
	{
		// lock g_burstmap[i]
		pthread_mutex_lock(&mutex_burstmap[i]) ; 
		isize = g_burstmap[i].size() ; 
		if(isize>0)
		{
			iter_burst = g_burstmap[i].begin();
			if(ifn_min>(*iter_burst).first.ifn)
			{
				ifn_min = (*iter_burst).first.ifn ;
			}
		}
		// unlock g_burstmap[i]
		pthread_mutex_unlock(&mutex_burstmap[i]) ; 
	}
	
	if(ifn_min==0x296fff)
	{
		ifn_min = 0  ;
	}

	return ifn_min ;  
}



int	Burst_fifo::size()
{
	int i ; 
	int isize = 0 ; 
	
	for(i=0;i<8;i++)
	{
		// lock g_burstmap[i]
		pthread_mutex_lock(&mutex_burstmap[i]) ; 
		isize += g_burstmap[i].size() ; 
		// unlock g_burstmap[i]
		pthread_mutex_unlock(&mutex_burstmap[i]) ; 
	}
	
	return isize ; 
}

int	Burst_fifo::fn_len() {
	int i;
	pthread_mutex_lock(&mutex_burstmap[7]);
	i = g_burstmap[7].size();
	pthread_mutex_unlock(&mutex_burstmap[7]);
	//fprintf(stderr,"system start! %d\n",g_itotal_arfcn);
	return i/g_itotal_arfcn;
}

int	Burst_fifo::drop(int n)
{
	int ifn_min ; 
	ifn_min = get_first_fn() ; 
	
	int ifn_bz ; 
	ifn_bz = ifn_min + n ;
	
	int i ; 
	BURSTMAP::iterator iter_burst ; 
		
	for(i=0;i<8;i++)
	{
		// lock g_burstmap[i]
		pthread_mutex_lock(&mutex_burstmap[i]) ; 
		for(iter_burst = g_burstmap[i].begin() ; iter_burst!= g_burstmap[i].end() ; iter_burst++)
		{
			if((*iter_burst).first.ifn<ifn_bz)
			{
				g_burstmap[i].erase(iter_burst) ; 
			}
			else
			{
				break ; 
			}
		}
		// unlock g_burstmap[i]
		pthread_mutex_unlock(&mutex_burstmap[i]) ; 
	}
	
	return 0 ; 
}
int Burst_fifo::getminfn(int iarfcn , int itimeslot)
{
	int imintime ;
	int iminfn ;
	int ibz = 0 ;

	BURSTMAP::iterator iter_burst ;
	CHANNEL_DESC_MAP::iterator iter_chan_desc;
	pthread_mutex_lock(&mutex_burstmap[itimeslot]) ;

	imintime = -1 ;
	iminfn = -1 ;
	for(iter_burst = g_burstmap[itimeslot].begin() ; iter_burst!= g_burstmap[itimeslot].end() ; iter_burst++)
	{
		if((*iter_burst).first.iarfcn==iarfcn)
		{
			if(ibz==0)
			{
				imintime = (*iter_burst).second.icurtime ;
				iminfn = (*iter_burst).first.ifn ;
				ibz = 1 ;
				pthread_mutex_unlock(&mutex_burstmap[itimeslot]) ;
				return iminfn ;
			}
			else
			{
				if((*iter_burst).second.icurtime < imintime)
				{
					imintime = (*iter_burst).second.icurtime ;
					iminfn = (*iter_burst).first.ifn ;
				}
			}
		}
	}

	pthread_mutex_unlock(&mutex_burstmap[itimeslot]) ;
	return iminfn ;
}


int Burst_fifo::gettotalburstbyarfcn(int arfcn,int itimeslot)
{
	BURSTMAP::iterator iter_burst ;
	int isum ;

	isum = 0 ;
	pthread_mutex_lock(&mutex_burstmap[itimeslot]) ;
	for(iter_burst = g_burstmap[itimeslot].begin() ; iter_burst!= g_burstmap[itimeslot].end() ; iter_burst++)
	{
	if((*iter_burst).first.iarfcn == arfcn)
	{
	isum ++ ;
	}
	}

	pthread_mutex_unlock(&mutex_burstmap[itimeslot]) ;

	return isum ;
}

int Burst_fifo::getburstsize_fn(int iarfcn , int ifn , int itimeslot)
{
 BURSTMAP::iterator iter_burst ;
 BURSTKEY burstkey ;
 int ibz_time ;

 burstkey.ifn = ifn ;
 burstkey.iarfcn = iarfcn ;

 int icishu = 0 ;
 int ixuhuan = 0 ;
 int isize = 0 ;
 // 对g_burstmap[itimeslot]加锁
 pthread_mutex_lock(&mutex_burstmap[itimeslot]) ;

 while(1)
 {
  iter_burst = g_burstmap[itimeslot].find(burstkey) ;
  if(iter_burst != g_burstmap[itimeslot].end())
  {
   ibz_time = (*iter_burst).second.icurtime ;
   break ;
  }
  else
  {
	 burstkey.ifn ++ ;
   if(burstkey.ifn > 0x296ffe)
   {
    burstkey.ifn = 0  ;
    ixuhuan ++ ;
    if(ixuhuan > 1) // 循环多于1次，说明有问题，直接跳出，返回-1
    {
     isize = -1 ;
     pthread_mutex_unlock(&mutex_burstmap[itimeslot]) ;
     return -1 ;
    }
   }
   icishu ++ ;
  }
 }

 if(icishu == 0) // 说明是找到的参数中的fn
 {
  for(iter_burst = g_burstmap[itimeslot].begin() ; iter_burst!= g_burstmap[itimeslot].end() ; iter_burst++)
  {

   if(((*iter_burst).first.iarfcn == iarfcn)&&((*iter_burst).second.icurtime>ibz_time))
   {
    isize ++ ;
   }
  }
 }
 else // 说明找到的是参数中的fn后面的fn，
 {
  for(iter_burst = g_burstmap[itimeslot].begin() ; iter_burst!= g_burstmap[itimeslot].end() ; iter_burst++)
  {

   if(((*iter_burst).first.iarfcn == iarfcn)&&((*iter_burst).second.icurtime>=ibz_time))
   {
    isize ++ ;
   }
  }
 }

 pthread_mutex_unlock(&mutex_burstmap[itimeslot]) ;

 return isize ;
}
