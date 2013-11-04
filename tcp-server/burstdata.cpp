#include "burstdata.h"
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include "burst_process.h"
//ARFCNSET g_arfcnset;

BURSTMAP g_burstmap[8] ;
BURSTMAP g_chanmap[8] ;
CHANNEL_DESC_MAP g_channeldescmap ; 

pthread_mutex_t mutex_burstmap[8] ; 
pthread_mutex_t mutex_chanmap[8] ;
pthread_mutex_t mutex_channeldescmap ;
pthread_mutex_t mutex_arfcnnum ;
pthread_mutex_t mutex_smspdumap ;
pthread_mutex_t mutex_physical_channel_num ;

int  g_itotal_arfcn ;

void init_mutex_burst()
{
	int i ; 
	for(i=0;i<8;i++)
	{
		pthread_mutex_init(&mutex_burstmap[i] , NULL) ; 
	}
	for(i=0;i<8;i++)
	{
		pthread_mutex_init(&mutex_chanmap[i] , NULL) ;
	}
	pthread_mutex_init(&mutex_channeldescmap , NULL) ;
	pthread_mutex_init(&mutex_arfcnnum , NULL) ;
	pthread_mutex_init(&mutex_physical_channel_num , NULL) ;
}

void initial()  // 初始化函数
{
	init_mutex_burst(); 
	
}
