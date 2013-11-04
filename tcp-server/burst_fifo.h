/*
 * burst_fifo.h
 *
 *  Created on: Jun 25, 2012
 *      Author: hhh
 */

#ifndef BURST_FIFO_H_
#define BURST_FIFO_H_

#include <map>
#include "global.h"
#include "burstdata.h"
#include "burst_process.h"
class Burst_fifo {

private:
	int _narfcn;



protected:


public:
	Burst_fifo();
	Burst_fifo(int narfcn);
	~Burst_fifo();

	map<int, TDMA_FRAME> burstmap;  //

	int 			add_burst(int arfcn, int fn, int timeslot, unsigned char* bits);
	int 			retrieve(int arfcn, int fn, int timeslot , unsigned char *pbburstdata) ;
	int				get_first_fn();
	int				size();
	int				fn_len();
	int				drop(int n);
	int 			gettotalburstbyarfcn(int arfcn,int itimeslot=7);
	void            erase_burst(int arfcn, int fn, int itimeslot);
	int 			getminfn(int iarfcn , int itimeslot);
	int 			getburstsize_fn(int iarfcn , int ifn , int itimeslot);
};

#endif /* BURST_FIFO_H_ */
