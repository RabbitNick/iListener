/*
 * tcp-server-thread.cc
 *
 *  Created on: 2012-8-12
 *      Author: NickChan
 */



#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


#include <string.h>
#include <stdlib.h>
#include <pthread.h>



#include <math.h>
#include <netdb.h>
#include <pwd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <iostream>
#include <string>
#include <cstring>

#include "tcp-server.h"
#include "global.h"
#include "burst_process.h"
#include "burstdata.h"

extern SERVER_CONF server_conf;

void *ThreadMain(void *threadArgs);
int handle_socket(int SocketFD, const char* IPAddr);
int idecompressburstdata1_8(unsigned char *pbcompress , unsigned char *pbburstdata);

struct ThreadArgs
{
	int clntSock;
	char clntName[INET_ADDRSTRLEN];
};

void *tcpserver_thread(void* p)
{
	//(struct TCP_SERVER_CONF*)&receiver[i]);
	struct TCP_SERVER_CONF *_server_conf=(struct TCP_SERVER_CONF*)p;

	unsigned short wserverport= _server_conf->port;
	char buf[BUF_SIZE],tmp_buf[BUF_SIZE];

	int fd;//temp fd

	struct sockaddr_in stSockAddr;
	struct sockaddr_in clientAddr;
	int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(-1 == SocketFD)
	{
		perror("can not create socket");
	   exit(EXIT_FAILURE);
	}

    memset(&stSockAddr, 0, sizeof(stSockAddr));
    memset(&clientAddr, 0, sizeof(clientAddr));


	unsigned short wport ;
	wport = SERVER_PORT ;
	if(wserverport!=0)
	{
		wport = wserverport ;
	}

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(wport);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;

    int ctrl_flag = 1;
    setsockopt(SocketFD,SOL_SOCKET,SO_REUSEADDR,&ctrl_flag,sizeof(ctrl_flag));

    if(-1 == bind(SocketFD,(struct sockaddr *)&stSockAddr, sizeof(stSockAddr)))
    {
      perror("error bind failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }



   if(-1 == listen(SocketFD, LISTEN_NUM))
    {
      perror("error listen failed");
      close(SocketFD);
      exit(EXIT_FAILURE);
    }
    if (server_conf.debug)
	   printf("server start !!!\n");
	unsigned int childProcCount = 0;

	socklen_t client_len = sizeof(clientAddr);

	while(1)
	{


		int ConnectFD = accept(SocketFD, (struct sockaddr *)&clientAddr,&client_len);
		//printf("ConnectFD : %d \n", ConnectFD);

		char clntName[INET_ADDRSTRLEN];
		if(inet_ntop(AF_INET, &clientAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL);
		//printf("client : %s \n", clntName);

		struct ThreadArgs *threadArgs = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
		if(threadArgs == NULL)
		{
			perror("malloc() failed");
		}
		threadArgs->clntSock = ConnectFD;
		strcpy(threadArgs->clntName, clntName);

		pthread_t threadID;
		int returnValue = pthread_create(&threadID, NULL, ThreadMain, threadArgs);
		if(returnValue != 0)
		{
			perror("pthread_create() failed");
		}
		if (server_conf.debug)
			printf("with thread %lu\n", (unsigned long int) threadID);
		usleep(1);

	}
		puts("Hello World!\n");
		return EXIT_SUCCESS;
}



void *ThreadMain(void *threadArgs)
{
	pthread_detach(pthread_self());
	int clntSock = ((struct ThreadArgs *) threadArgs)->clntSock;
	char clntName[INET_ADDRSTRLEN];
	strcpy(clntName, ((struct ThreadArgs *) threadArgs)->clntName);
	free(threadArgs);
	handle_socket(clntSock,clntName);
	return (NULL);
}

int handle_socket(int SocketFD, const char* IPAddr)
{
		#define BURST_SIZE 116
		int fd = SocketFD;
		char buf[1024];
		int iarfcn;
		int ifn;
		unsigned char abburstdata[BURST_SIZE] ;
		int recvLen = 0;
		int d_tc;
		int i = 0;
		char fd_str[10] = {0};

		sprintf(fd_str, "NO.%d", fd);
		while(1)
	    {
	    	for(int index = 0; index < 32; index = index + recvLen)
	    	{

	    		recvLen = recv(fd,&buf[index],32 - index ,0);
			//	printf("\n recLen : %d\n", recvLen);
	    		if(recvLen<=0)
	    		{

	    			if (server_conf.debug)
	    				printf("recvlen is less than 0 \n");
	    			break;
	    		}

	    	}

			if(recvLen<=0)
			{

				if (server_conf.debug)
					printf("recvlen is less than 0 \n");
				return 1;
			}
			else
			{
				int itimeslot, mfn ;
				unsigned char btimeslot ;
				unsigned short warfcn ;
				btimeslot = buf[0] ;
				itimeslot = btimeslot ;
				memcpy(&ifn , &buf[1] , 4) ;
				memcpy(&warfcn , &buf[5] , 2) ;
				iarfcn = warfcn ;

				unsigned char abcompress[20] ;
				memset(abcompress , 0 , sizeof(abcompress)) ;
				memcpy(abcompress , &buf[7] , 15 ) ;
				memset(abburstdata , 0 , sizeof(abburstdata)) ;
				idecompressburstdata1_8(abcompress , abburstdata);

				if (server_conf.printburstinput) {
					printf("\n-------------------\n");
					printf("timeslot : %d\tifn : %d\t mfn:%d\t tc: %d arfcn : %d\n",itimeslot , ifn , ifn%51, d_tc, iarfcn) ;
					printf("burst size: %d\n", BURST_SIZE);
					for(int i =0; i< BURST_SIZE; i++) {
						printf("%d", *(abburstdata+i));
					}
					fflush(stdout);
					printf("\n");
				}
				if(itimeslot>7)
				{
					iarfcn+=2000;
					itimeslot-=20;	

				}
				do_process((unsigned char* )abburstdata, iarfcn, itimeslot, ifn);
			}

	    }
		return 1;

}

int idecompressburstdata1_8(unsigned char *pbcompress , unsigned char *pbburstdata)
{
	memset(pbburstdata , 0 , sizeof(pbburstdata)) ;
		int i , j ;
		int iwz = 0 ;
		for(i=0;i<14;i++)
		{
			for(j=0;j<8;j++)
			{
				pbburstdata[iwz] = (pbcompress[i]>>(7-j))&1 ;
				iwz ++ ;
			}
		}
		for(j=0;j<4;j++)
		{
			pbburstdata[iwz] = (pbcompress[i]>>(3-j))&1 ;
			iwz ++ ;
		}
		return 0 ;
}


