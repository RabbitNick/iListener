#include "tcpclientdata.h"
#include <time.h>

//#include "getmd5fromprog.h"

struct sockaddr_in g_server_sin;
struct hostent *g_host;
int g_sock_fd;
unsigned char g_abkey[128] ; 
int g_ikey_fn ; 
int g_ikey_timeslot ; 

//void getkey_client()
//{
//	unsigned char abmd5[16] ;
//	memset(abmd5 , 0 , sizeof(abmd5)) ;
//
//	memset(g_abkey, 0 , sizeof(g_abkey)) ;
//	int i , j ;
//	for(i=0;i<16;i++)
//	{
//		for(j=0;j<8;j++)
//		{
//			g_abkey[i*8+j] = (abmd5[i]>>j)&1 ;
//		}
//	}
//
//	g_ikey_fn = 0 ;
//	for(i=0;i<9;i++)
//	{
//		g_ikey_fn = (g_ikey_fn<<1)+g_abkey[116+i] ;
//	}
//
//	g_ikey_timeslot = 0 ;
//	for(i=0;i<3;i++)
//	{
//		g_ikey_timeslot = (g_ikey_timeslot<<1) + g_abkey[125+i] ;
//	}
//}

/**
 * Return none 0 if connect successed
 */
int iconnectserver(char *pcserverip , unsigned short wport)
{
	//getkey_client() ;
	
	memset(&g_server_sin,0,sizeof(g_server_sin));
	g_server_sin.sin_family=AF_INET;
	g_server_sin.sin_port=htons(wport);
	
	if((g_host=gethostbyname(pcserverip))==NULL)
	{
			perror("gethostbyname");
		return 0;
	}
	
	g_server_sin.sin_addr.s_addr = inet_addr(pcserverip);

	if((g_sock_fd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket");
		return 0;
	}


	if(connect(g_sock_fd,(struct sockaddr *)&g_server_sin,sizeof(g_server_sin))<0)
	{
		perror("connect");
		close(g_sock_fd);
		return 0;
	}
	
	return 1;
}


void write_burst(unsigned char* u, int arfcn, int timeslot, int fn,double power) {



	FILE *pFILE = fopen((char*)"rec_raw_burst.txt", "a");

	fprintf(pFILE, "A: %d TS: %d  FN: %d mfn: %d \n", arfcn, timeslot, fn, fn%51);
	for(int k = 0;k < 116; k++) {
		fprintf(pFILE, "%d",*(u+k));
	}
	fprintf(pFILE, "\n\n");

	fclose(pFILE);
}


int isenddata(int itimeslot , int ifn , int iarfcn , unsigned char *pbburstdata , int iburstdatalen )
{
	unsigned char abcompressdata[20] ; 
	int icompresslen ; 
	unsigned char btimeslot ;
	unsigned short warfcn ;
	unsigned char abdata[200] ;
	static unsigned char tmp  = 0;

	memset(abcompressdata , 0 , sizeof(abcompressdata)) ;
	icompresslen = 20 ;
	compressburstdata8_1(pbburstdata , iburstdatalen , abcompressdata , icompresslen) ; 

	btimeslot = itimeslot & 0xff ;
	warfcn = iarfcn & 0xffff ; 

	memset(abdata , 0 , sizeof(abdata)) ; 
	abdata[0] = btimeslot ;
	memcpy(&abdata[1] , &ifn , 4) ; 
	memcpy(&abdata[5] , &warfcn , 2 ) ; 
	memcpy(&abdata[7] , abcompressdata ,  icompresslen) ;

	send(g_sock_fd,(char *)abdata,24+sizeof(double),0);

	return 0 ; 
}

void disconnectserver()
{
	close(g_sock_fd);
}


int compressburstdata8_1(unsigned char *pbburstdata , int iburstdatalen , unsigned char *pbcompressdata , int &icompressdatalen)
{
	if((iburstdatalen&7)==0)
	{
		icompressdatalen = iburstdatalen>>3 ; 
	}
	else
	{
		icompressdatalen = (iburstdatalen>>3) + 1; 
	}
	
	int i , j ; 
	int iwz = 0 ; 
	unsigned char btemp ; 
	for(i=0;i<icompressdatalen-1;i++)
	{
		btemp = 0 ; 
		for(j=0;j<8;j++)
		{
			btemp = (btemp<<1)|pbburstdata[iwz] ; 
			iwz ++ ; 
		}
		pbcompressdata[i] = btemp ; 
	}
	btemp = 0 ; 
	for(j=0;j<8;j++)
	{
		if(iwz<iburstdatalen)
		{
			btemp = (btemp<<1)|pbburstdata[iwz] ; 
			iwz ++ ; 
		}
		else
		{
			break ; 
		}
	}
	pbcompressdata[i] = btemp ; 
	
	return 0 ; 
}
