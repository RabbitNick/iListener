#ifndef _DEALIA_H
#define _DEALIA_H

#include "burstdata.h"

int 	add_channel_desc(CHANNEL_DESC channel_desc , CHANNEL_DESC_DATA channel_desc_data);  //return sizeof queue
int 	num_of_channel_desc(); //i.e. size of queue
int		get_oldest_ia_fn();
//bool	is_exist_ia(int arfcn, int timeslot, int sub_channel,int hopping ,int maio ,int hsn);
void 	is_exist_ia(int arfcn, int timeslot, int sub_channel,int hopping ,int maio ,int hsn);
bool	remove(CHANNEL_DESC_MAP::iterator iter_channel_desc_map);
extern SERVER_CONF server_conf;

#endif
