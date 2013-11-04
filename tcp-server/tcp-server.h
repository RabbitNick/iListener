/*
 * tcp-server.h
 *
 *  Created on: Jun 21, 2012
 *      Author: hhh
 */

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_


//int tcpserver(char *pcserverip , unsigned short wserverport);
void *tcpserver_thread(void* p);
int idecompressburstdata1_8(unsigned char *pbcompress , unsigned char *pbburstdata) ; 

#endif /* TCP_SERVER_H_ */
