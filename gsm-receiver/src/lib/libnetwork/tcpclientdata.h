#ifndef _TCPCLIENTDATA_H
#define _TCPCLIENTDATA_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> //gethostbyname()Ҫ�õ�
#include <pthread.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define NET_IP "127.0.0.1"
#define NET_PORT 8080

extern struct sockaddr_in g_server_sin;
extern struct hostent *g_host;
extern int g_sock_fd;
extern unsigned char g_abkey[128] ; 
extern int g_ikey_fn ; 
extern int g_ikey_timeslot ; 

// ���ӷ����� 0�ɹ� 1ʧ��
int iconnectserver(char *pcserverip , unsigned short wport) ; 

// ����BURST���
int isenddata(int itimeslot , int ifn , int iarfcn , unsigned char *pbburstdata , int iburstdatalen );

// ��ѹ��BURST��ݣ�8--->1
int compressburstdata8_1(unsigned char *pbburstdata , int iburstdatalen , unsigned char *pbcompressdata , int &icompressdatalen) ; 

// ��������Ͽ�����
void disconnectserver() ; 

#endif
