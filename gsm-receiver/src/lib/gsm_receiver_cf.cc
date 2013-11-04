/* -*- c++ -*- */
/*
 * @file
 * @author Piotr Krysik <pkrysik@stud.elka.pw.edu.pl>
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gr_io_signature.h>
#include <gr_math.h>
#include <math.h>
#include <Assert.h>
#include <boost/circular_buffer.hpp>
#include <algorithm>
#include <numeric>
#include <gsm_receiver_cf.h>
#include <viterbi_detector.h>
#include <string.h>
#include <decoder/sch.h>
#include <decoder/a5-1-2.h>//!!

#include "RxBurst.h"
#include "GSMCommon.h"
#include "SDRini.h"

#define SYNC_SEARCH_RANGE 30

typedef std::vector<float> vector_float;

typedef boost::circular_buffer<float> circular_buffer_float;

gsm_receiver_cf_sptr
gsm_make_receiver_cf(gr_feval_dd *tuner, gr_feval_dd *synchronizer, int osr, std::string key, \
		std::string configuration, SDRconfiguration sdr_config)
{
  return gsm_receiver_cf_sptr(new gsm_receiver_cf(tuner, synchronizer, osr, key, configuration, sdr_config));
}

static const int MIN_IN = 1; // mininum number of in_0 streams
static const int MAX_IN = 2; // maximum number of in_0 streams
static const int MIN_OUT = 0; // minimum number of output streams
static const int MAX_OUT = 1; // maximum number of output streams

/*
 * The private constructor
 */
gsm_receiver_cf::gsm_receiver_cf(gr_feval_dd *tuner, gr_feval_dd *synchronizer, int osr, std::string key, \
		std::string configuration, SDRconfiguration sdr_config)
    : gr_block("gsm_receiver",
               gr_make_io_signature(MIN_IN, MAX_IN, sizeof(gr_complex)),
               gr_make_io_signature(MIN_OUT, MAX_OUT, 142 * sizeof(float))),
    d_OSR(osr),
    d_chan_imp_length(CHAN_IMP_RESP_LENGTH),
    d_tuner(tuner),
    d_synchronizer(synchronizer),
    d_counter(0),
    d_fcch_start_pos(0),
    d_freq_offset(0),
    d_state(first_fcch_search),
    d_burst_nr(osr),
    d_failed_sch(0),
    d_trace_sch(true),
    d_symbol_counter(0),
    d_arfcn(sdr_config.ch0_arfcn),
    d_ch1_arfcn(sdr_config.ch1_arfcn),
    d_net_ip(sdr_config.net_ip),
    d_net_port(sdr_config.net_port),
	sb_e(0),
	nb_e(0),
	d_sdr_config(sdr_config)
{
  const unsigned char amr_nb_magic[6] = { 0x23, 0x21, 0x41, 0x4d, 0x52, 0x0a };
  int i;
  size_t conn_result;

  scan_bts_flag.scan_flag = d_sdr_config.scanBTSmode;
  if(scan_bts_flag.scan_flag == 1)
  {
	  std::cout << "scan BTS mode!!!" << std::endl;
  }
  else
  {
	  std::cout << "Normal mode!!!" << std::endl;
  }

  gmsk_mapper(SYNC_BITS, N_SYNC_BITS, d_sch_training_seq, gr_complex(0.0, -1.0));
  for (i = 0; i < TRAIN_SEQ_NUM; i++) {
    gr_complex startpoint;
    if (i == 6 || i == 7) {                           //this is nasty hack
      startpoint = gr_complex(-1.0, 0.0);   //if I don't change it here all bits of normal bursts for BTSes with bcc=6 will have reversed values
    } else {
      startpoint = gr_complex(1.0, 0.0);    //I've checked this hack for bcc==0,1,2,3,4,6
    }                                       //I don't know what about bcc==5 and 7 yet
    //TODO:find source of this situation - this is purely mathematical problem I guess
    gmsk_mapper(train_seq[i], N_TRAIN_BITS, d_norm_training_seq[i], startpoint);
  }

  //std::cout << "configuration : "<< configuration <<std::endl;

  conn_result = 0; // set it to false first

  if(scan_bts_flag.scan_flag != 1) {
	  while (!conn_result) {
		  printf("Connecting to server: %d \n",conn_result );
		  conn_result = iconnectserver((char *)d_net_ip.c_str(), d_net_port);
		  if (!conn_result) {
			  printf("Cannot connect to server. Try again\n");

			  sleep(3);
		  } else {
			  	  printf("Server Connected.\n");
		  }
	  }//while
  }

}

/*
 * Virtual destructor.
 */
gsm_receiver_cf::~gsm_receiver_cf()
{


}

void gsm_receiver_cf::forecast(int noutput_items, gr_vector_int &nitems_items_required)
{
	nitems_items_required[1] = noutput_items * floor((TS_BITS + 2 * GUARD_PERIOD) * d_OSR);
	nitems_items_required[0] = noutput_items * floor((TS_BITS + 2 * GUARD_PERIOD) * d_OSR);
}


int
gsm_receiver_cf::general_work(int noutput_items,
                              gr_vector_int &nin_0_items,
                              gr_vector_const_void_star &in_0_items,
                              gr_vector_void_star &output_items)
{
	const gr_complex *in_0, *in_1;
	in_0 = (const gr_complex *) in_0_items[0];
	in_1 = (const gr_complex *) in_0_items[1];



	int consumable = nin_0_items[0] > nin_0_items[1]?nin_0_items[1]:nin_0_items[0];

	int produced_out = 0;  //how many output elements were produced - this isn't used yet

	switch (d_state) {
		//bootstrapping
		case first_fcch_search: {
			//printf("Search for fcch! \n");
			if (find_fcch_burst(in_0, in_1, consumable)) { //find frequency correction burst in the in_0 buffer


				set_frequency(d_freq_offset);                //if fcch search is successful set frequency offset
			//	printf("==============Freq correction done ! \n");
			//	printf("First Freq: %f \n", d_freq_offset);
				if(scan_bts_flag.scan_flag == 1)
				{
				//	std::cout << "scan BTS mode!!!" << std::endl;
					scan_bts_flag.fcch_count++;
					scan_bts_flag.fcch_found = 1;
					scan_bts_flag.fcch_unfcount = 0;

			//		std::cout << "scan BTS find first fcch!!!" << std::endl;
				}
				d_state = next_fcch_search;



			} else {

				if(scan_bts_flag.scan_flag == 1)
				{
			//		std::cout << "scan BTS mode!!!" << std::endl;
					scan_bts_flag.fcch_unfcount++;
					scan_bts_flag.fcch_found = 0;
				//	scan_bts_flag.fcch_count = 0;

					scan_bts_flag.sch_unfcount = 0; //can not find fcch so it can not find sch

			//		std::cout << "scan BTS can not find first fcch!!!" << std::endl;
					if(scan_bts_flag.fcch_unfcount >= d_sdr_config.fcch_timeout_boundary)
					{
						scan_bts_flag.fcch_timeout = 1;
				//		set_scanbts_flag(0);
						d_freq_offset = 0;
						scan_bts_flag.scan_count++;
					//	scan_bts_flag.fcch_unfcount = 0;
						scan_bts_flag.fcch_count = 0;

						scan_bts_flag.d_freq_offset_p = d_freq_offset;

						set_frequency(0);

				//		std::cout << "scan BTS finding fcch is timeout!" << std::endl;
					}

//					if(abs(d_freq_offset)> 100000)
//					{
//						d_state = first_fcch_search;
//						scan_bts_flag.fcch_count = 0;
//						scan_bts_flag.fcch_found = 0;
//						scan_bts_flag.fcch_timeout = 0;
//						scan_bts_flag.fcch_unfcount = 0;
//						d_freq_offset = 0;
//
//					}
				}

				d_state = first_fcch_search;
			}
			break;
		}

		case next_fcch_search: {                          //this state is used because it takes some time (a bunch of buffered samples)
			float prev_freq_offset = d_freq_offset;        //before previous set_frequqency cause change
			if (find_fcch_burst(in_0, in_1, consumable)) {



				if (abs(prev_freq_offset - d_freq_offset) > FCCH_MAX_FREQ_OFFSET) {
					set_frequency(d_freq_offset);              //call set_frequncy only frequency offset change is greater than some value
				}
				//printf("============Sec Freq correction done ! \n");
			//	printf("next d_freq_offset: %f \n", d_freq_offset);



				if(scan_bts_flag.scan_flag == 1)
				{
			//		std::cout << "scan BTS mode!!!" << std::endl;
					scan_bts_flag.fcch_unfcount = 0;
					scan_bts_flag.fcch_count++;
					scan_bts_flag.fcch_found = 2;
			//		std::cout << "scan BTS find second fcch!!!" << std::endl;
				}

				d_state = sch_search;
			} else {

				d_state = next_fcch_search;


				if(scan_bts_flag.scan_flag == 1)
				{

				//	std::cout << "scan BTS mode!!!" << std::endl;
					scan_bts_flag.fcch_count = 0;
					scan_bts_flag.fcch_unfcount++;
					scan_bts_flag.fcch_found = 1;

				//	std::cout << "scan BTS can not find second fcch!!!" << std::endl;


					if(scan_bts_flag.fcch_unfcount >= d_sdr_config.fcch_timeout_boundary)
					{
						d_freq_offset = 0;
						scan_bts_flag.fcch_unfcount = 0;
						scan_bts_flag.fcch_timeout = 1;
					//	scan_bts_flag.sch_unfcount = 0;
						scan_bts_flag.scan_count++;

						scan_bts_flag.d_freq_offset_p = d_freq_offset;

						//set_scanbts_flag(0);
						set_frequency(0);



				//		std::cout << "scan BTS finding second fcch is timeout!" << std::endl;
					//	std::cout << "scan BTS finding second fcch is timeout!" << std::endl;


						d_state = first_fcch_search;

					}
				}
			}
			break;
		}

		case sch_search: {
			int t1, t2, t3, tt;
			vector_complex channel_imp_resp(CHAN_IMP_RESP_LENGTH*d_OSR);

			int burst_start = 0;
			unsigned char output_binary[BURST_SIZE];

			//printf("SCH Search!  consumable: %d\n", consumable);
			if (reach_sch_burst(consumable)) {                              //wait for a SCH burst
				burst_start = get_sch_chan_imp_resp(in_0, &channel_imp_resp[0]); //get channel impulse response from it
				//printf("SCH Burst start %d\n", burst_start);

				detect_burst(in_0, &channel_imp_resp[0], burst_start, output_binary); //detect bits using MLSE detection
				if (decode_sch(&output_binary[3], &t1, &t2, &t3, &d_ncc, &d_bcc) == 0) { //decode SCH burst
					tt = ((t3 + 26) - t2) % 26;
					d_fn = (51 * 26 * t1) + (51 * tt) + t3;//set counter of bursts value
				//	printf("!!!!!!!!!miaomiaomiao!!! burst start: %d fn: %d mfn: %d bcc: %d !\n", burst_start, d_fn, d_fn%51, d_bcc);
					consume_each(burst_start + BURST_SIZE * d_OSR);   //consume samples up to next guard period
					d_timeslot=1; // set next timeslot to be 1
					d_mfn = t3;
					d_state = synchronized;


//					if(scan_bts_flag.scan_flag == 0)
//						std::cout << "YYYYYYYY" << std::endl;

					if(scan_bts_flag.scan_flag == 1)
					{
						//set_scanbts_flag(0);
					//	std::cout << "scan BTS mode!!!" << std::endl;
					//	std::cout << "scan BTS scan_count: " << scan_bts_flag.scan_count << std::endl;
						scan_bts_flag.sch_count++;
						scan_bts_flag.sch_unfcount = 0;
					//	std::cout << "scan BTS find sch!!!" << std::endl;
						scan_bts_flag.sch_found = 1;
						//scan_bts_flag.scan_count++;

						scan_bts_flag.d_freq_overflow = 0;
				//		if(scan_bts_flag.sch_count >= 3)
			//			std::cout << "scan BTS find sch!!!" << std::endl;
				//		scan_bts_flag.sch_found = 1;
						//scan_bts_flag.scan_count++;

						d_state = first_fcch_search;
						if(scan_bts_flag.sch_count >= 3)
						{
					//		scan_bts_flag.scan_count++;
							//set_scanbts_flag(0);
							scan_bts_flag.sch_found = 3;
						//	std::cout << "scan BTS d_freq_offset" <<d_freq_offset <<std::endl;
							if(abs(d_freq_offset) > 50000)
							{
								scan_bts_flag.d_freq_offset_p = d_freq_offset;
								scan_bts_flag.d_freq_overflow = 1;
							}
							scan_bts_flag.scan_count++;
							//set_scanbts_flag(0);
							scan_bts_flag.sch_found = 3;
	//						std::cout << "scan BTS d_freq_offset" <<d_freq_offset <<std::endl;
							d_freq_offset = 0;

							set_frequency(0);
	//scan_bts_flag.scan_count = 0;


							scan_bts_flag.sch_count = 0;
							scan_bts_flag.sch_unfcount = 0;
							scan_bts_flag.sch_timeout = 0;

							//scan_bts_flag.scan_flag = 0;

							scan_bts_flag.fcch_count = 0;
							scan_bts_flag.fcch_found = 0;
							scan_bts_flag.fcch_timeout = 0;
							scan_bts_flag.fcch_unfcount = 0;

					//		std::cout << "scan BTS find 3 schs!!!" << std::endl;

						}
					}

				} else {

					d_state = next_fcch_search;                       //if there is error in the sch burst go back to fcch search phase


					if(scan_bts_flag.scan_flag == 1)
					{
				//		std::cout << "scan BTS mode!!!" << std::endl;
				//		std::cout << "scan BTS can not find sch!!!" << std::endl;

						scan_bts_flag.sch_unfcount++;
						scan_bts_flag.sch_count = 0;

						if(scan_bts_flag.sch_unfcount >= d_sdr_config.sch_timeout_boundary)
						{

							d_freq_offset = 0;
							scan_bts_flag.sch_found = 0;
							scan_bts_flag.sch_count = 0;
							scan_bts_flag.sch_unfcount = 0;
							scan_bts_flag.sch_timeout = 1;
							set_frequency(0);
					//		std::cout << "scan BTS finding sch is timeout" << std::endl;
							d_state = first_fcch_search;
						}
					}
				}
			} else {
				d_state = sch_search;

			}


			break;
		}



		case synchronized: {




			vector_complex channel_imp_resp(CHAN_IMP_RESP_LENGTH*d_OSR);
			int burst_start;
			int offset = 0;
			int to_consume = 0;
			double power;
			unsigned char output_binary[BURST_SIZE];

			burst_type b_type = get_burst_type(0, d_fn, d_timeslot);
//			printf("FN: %d, MFN: %d, Timeslot: %d, burst_type: %d    ", d_fn, d_mfn, d_timeslot, b_type);
//
//			printf("<<<<<<<<<<<<<<<<<<<<<<< Signal Sync! \n ");
//			printf("burst type: %d \n", b_type);

			switch (b_type) {
				case fcch_burst: {
		//			printf("FCCH     ");
					const unsigned first_sample = ceil((GUARD_PERIOD + 2 * TAIL_BITS) * d_OSR) + 1;
					const unsigned last_sample = first_sample + USEFUL_BITS * d_OSR - TAIL_BITS * d_OSR;
					double freq_offset = compute_freq_offset(in_0, first_sample, last_sample);       //extract frequency offset from it

//					printf("Freq offset: %f \n", freq_offset);
					//fprintf(stderr,"FN: %d, MFN: %d,Freq offset: %f \n",d_fn,d_mfn,freq_offset);
					break;
				}
				case sch_burst: {
//					printf("Synced!  SCH \n");

					int t1, t2, t3, tt;
					burst_start = get_sch_chan_imp_resp(in_0, &channel_imp_resp[0]);                //get channel impulse response
					detect_burst(in_0, &channel_imp_resp[0], burst_start, output_binary);           //MLSE detection of bits

	//				printf("sync sch start: %d     ", burst_start);
					if (decode_sch(&output_binary[3], &t1, &t2, &t3, &d_ncc, &d_bcc) == 0) {         //and decode SCH data                                              //but only to check if burst_start value is correct
//						printf("------------SCH------------------------ start: %d\n", burst_start);
						d_failed_sch = 0;
						offset =  burst_start - floor((GUARD_PERIOD) * d_OSR);                         //compute offset from burst_start - burst should start after a guard period
						tt = ((t3 + 26) - t2) % 26;
						d_fn = (51 * 26 * t1) + (51 * tt) + t3;
						d_mfn = t3;
						//d_timeslot=1; // set next timeslot to be 1

//						printf("bcc: %d, t3: %d fn: %d offset: %d\n", d_bcc, t3,d_fn, offset);
						to_consume += offset;                                                          //adjust with offset number of samples to be consumed

					} else {
						//printf("Decode error!\n");
						d_failed_sch++;
						if (d_failed_sch >= MAX_SCH_ERRORS) {
							d_state = next_fcch_search;        //TODO: this isn't good, the receiver is going wild when it goes back to next_fcch_search from here
//							d_freq_offset_vals.clear();
						}
					}

					break;
				}


				case normal_burst: {
					int n_fn, n_timeslot;
					int is_normal_burst=0;
//					double power = 0;



					if(d_sdr_config.ch0_upload == 1)
					{
						burst_start = get_norm_chan_imp_resp(in_0, &channel_imp_resp[0], d_bcc); //get channel impulse response for given training sequence number - d_bcc
						detect_burst(in_0, &channel_imp_resp[0], burst_start, output_binary);          //MLSE detection of bits
						process_network_burst(d_fn, d_arfcn, d_timeslot, output_binary);

					}

					if (d_channel2_type == none) {
						break;
					}
					//
					//  Channel 2
					//
					if(d_sdr_config.ch1_upload == 1)
					{
						burst_start = get_norm_chan_imp_resp(in_1, &channel_imp_resp[0], d_bcc); //get channel impulse response for given training sequence number - d_bcc
						detect_burst(in_1, &channel_imp_resp[0], burst_start, output_binary);            //MLSE detection of bits
						if (d_channel2_type == downlink) {
							n_fn = d_fn;
							n_timeslot = d_timeslot; // add 10 for timeslots in channel 2 are downlink
						} else if (d_channel2_type == uplink) {
							n_timeslot = d_timeslot ; //net yet implemented
							if (n_timeslot < 3)
							{
								n_timeslot +=5;
								n_fn = d_fn - 1;
							}
							else
							{
								n_timeslot -=3;
								n_fn = d_fn;
							}
							n_timeslot = n_timeslot + 20;  // add 20 for timeslots in channel 2 are uplink
						}
						process_network_burst(n_fn, d_ch1_arfcn, n_timeslot, output_binary);
					}

					break;
				}


				case dummy_or_normal: {
					break;
				}

				case rach_burst:
					break;

				case dummy:                                                         //if it's dummy
					break;

				case empty:   //if it's empty burst
					break;      //do nothing

			} //switch b_type close {}


			to_consume += TS_BITS * d_OSR;  //consume samples of the burst up to next guard period
			to_consume += d_OSR*0.25;

			if (d_timeslot == 7) {
				d_timeslot = 0;
				d_fn++;
				d_mfn=d_fn%51;
			} else d_timeslot++;
			gsm_receiver_cf::consume_each(to_consume);



			break;
		}
	} // end of case

	return produced_out;
}

bool gsm_receiver_cf::process_network_burst(int fn, int arfcn, int timeslot, unsigned char* burst_binary) {
	unsigned char network_binary[NETWORK_BURST_BIT];
	int j;

	memcpy(network_binary,burst_binary+3,58*sizeof(char));
	memcpy(network_binary+58,burst_binary+87,58*sizeof(char));
	isenddata(timeslot, fn, arfcn, network_binary, NETWORK_BURST_BIT);
	return 1;
}

bool gsm_receiver_cf::find_fcch_burst(const gr_complex *in_0, const gr_complex *in2, const int nitems)
{
  circular_buffer_float phase_diff_buffer(FCCH_HITS_NEEDED * d_OSR); //circular buffer used to scan throug signal to find
  //best match for FCCH burst
  float phase_diff = 0;
  gr_complex conjprod;
  int start_pos = -1;
  int hit_count = 0;
  int miss_count = 0;
  float min_phase_diff;
  float max_phase_diff;
  double best_sum = 0;
  float lowest_max_min_diff = 99999;

  int to_consume = 0;
  int sample_number = 0;
  bool end = false;
  bool result = false;
  circular_buffer_float::iterator buffer_iter;

  /**@name Possible states of FCCH search algorithm*/
  //@{
  enum states {
    init,               ///< initialize variables
    search,             ///< search for positive samples
    found_something,    ///< search for FCCH and the best position of it
    fcch_found,         ///< when FCCH was found
    search_fail         ///< when there is no FCCH in the in_0 vector
  } fcch_search_state;
  //@}

  fcch_search_state = init;

  while (!end) {
    switch (fcch_search_state) {

      case init: //initialize variables
        hit_count = 0;
        miss_count = 0;
        start_pos = -1;
        lowest_max_min_diff = 99999;
        phase_diff_buffer.clear();
        fcch_search_state = search;

        break;

      case search: // search for positive samples
        sample_number++;

        if (sample_number > nitems - FCCH_HITS_NEEDED * d_OSR) { //if it isn't possible to find FCCH because
          //there's too few samples left to look into,
          to_consume = sample_number;                            //don't do anything with those samples which are left
          //and consume only those which were checked
          fcch_search_state = search_fail;
        } else {
          phase_diff = compute_phase_diff(in_0[sample_number], in_0[sample_number-1]);

          if (phase_diff > 0) {                                 //if a positive phase difference was found
            to_consume = sample_number;
            fcch_search_state = found_something;                //switch to state in which searches for FCCH
          } else {
            fcch_search_state = search;
          }
        }

        break;

      case found_something: {// search for FCCH and the best position of it
          if (phase_diff > 0) {
            hit_count++;       //positive phase differencies increases hits_count
          } else {
            miss_count++;      //negative increases miss_count
          }

          if ((miss_count >= FCCH_MAX_MISSES * d_OSR) && (hit_count <= FCCH_HITS_NEEDED * d_OSR)) {
            //if miss_count exceeds limit before hit_count
            fcch_search_state = init;       //go to init
            continue;
          } else if (((miss_count >= FCCH_MAX_MISSES * d_OSR) && (hit_count > FCCH_HITS_NEEDED * d_OSR)) || (hit_count > 2 * FCCH_HITS_NEEDED * d_OSR)) {
            //if hit_count and miss_count exceeds limit then FCCH was found
            fcch_search_state = fcch_found;
            continue;
          } else if ((miss_count < FCCH_MAX_MISSES * d_OSR) && (hit_count > FCCH_HITS_NEEDED * d_OSR)) {
            //find difference between minimal and maximal element in the buffer
            //for FCCH this value should be low
            //this part is searching for a region where this value is lowest
            min_phase_diff = * (min_element(phase_diff_buffer.begin(), phase_diff_buffer.end()));
            max_phase_diff = * (max_element(phase_diff_buffer.begin(), phase_diff_buffer.end()));

            if (lowest_max_min_diff > max_phase_diff - min_phase_diff) {
              lowest_max_min_diff = max_phase_diff - min_phase_diff;
              start_pos = sample_number - FCCH_HITS_NEEDED * d_OSR - FCCH_MAX_MISSES * d_OSR; //store start pos
              best_sum = 0;

              for (buffer_iter = phase_diff_buffer.begin();
                   buffer_iter != (phase_diff_buffer.end());
                   buffer_iter++) {
                best_sum += *buffer_iter - (M_PI / 2) / d_OSR;   //store best value of phase offset sum
              }
            }
          }

          sample_number++;

          if (sample_number >= nitems) {    //if there's no single sample left to check
            fcch_search_state = search_fail;//FCCH search failed
            continue;
          }

          phase_diff = compute_phase_diff(in_0[sample_number], in_0[sample_number-1]);
          phase_diff_buffer.push_back(phase_diff);
          fcch_search_state = found_something;
        }
        break;

      case fcch_found: {
          DCOUT("fcch found on position: " << d_counter + start_pos);
          to_consume = start_pos + FCCH_HITS_NEEDED * d_OSR + 1; //consume one FCCH burst

          d_fcch_start_pos = d_counter + start_pos;

          //compute frequency offset
          double phase_offset = best_sum / FCCH_HITS_NEEDED;
          double freq_offset = phase_offset * 1625000.0 / (12.0 * M_PI);
          d_freq_offset -= freq_offset;
          DCOUT("freq_offset: " << d_freq_offset);

          end = true;
          result = true;
          break;
        }

      case search_fail:
        end = true;
        result = false;
        break;
    }
  }

  d_counter += to_consume;
  consume_each(to_consume);

  return result;
}

float gsm_receiver_cf::correlate_pattern(const unsigned char *pattern,unsigned char *output_binary)
{

	//need to save these for later printing, etc
	//TODO: not much need for function params when we have the member vars
	d_corr_pattern = pattern;
	d_corr_max = 0.0;
	for (int i = 1; i < 26; i++) {	//Start a 1 to skip first bit due to diff encoding
		d_corr_max += output_binary[58+3+i]==pattern[i]? 1:0;
	}
	d_corr_max /= (26 - 1); //normalize, -1 for skipped first bit
	return d_corr_max;
}

double gsm_receiver_cf::compute_freq_offset(const gr_complex * in_0, unsigned first_sample, unsigned last_sample)
{
  double phase_sum = 0;
  unsigned ii;

  for (ii = first_sample; ii < last_sample; ii++) {
    double phase_diff = compute_phase_diff(in_0[ii], in_0[ii-1]) - (M_PI / 2) / d_OSR;
    phase_sum += phase_diff;
  }

  double phase_offset = phase_sum / (last_sample - first_sample);
  double freq_offset = phase_offset * 1625000.0 / (12.0 * M_PI);
  return freq_offset;
}

void gsm_receiver_cf::set_frequency(double freq_offset)
{
  d_tuner->calleval(freq_offset);
}



void gsm_receiver_cf::set_scanbts_flag(double freq_offset)
{
	d_synchronizer->calleval(freq_offset);
}


inline float gsm_receiver_cf::compute_phase_diff(gr_complex val1, gr_complex val2)
{
  gr_complex conjprod = val1 * conj(val2);
  return gr_fast_atan2f(imag(conjprod), real(conjprod));
}

bool gsm_receiver_cf::reach_sch_burst(const int nitems)
{
  //it just consumes samples to get near to a SCH burst
  int to_consume = 0;
  bool result = false;
  unsigned sample_nr_near_sch_start = d_fcch_start_pos + (FRAME_BITS - SAFETY_MARGIN) * d_OSR;

  //consume samples until d_counter will be equal to sample_nr_near_sch_start
  if (d_counter < sample_nr_near_sch_start) {
    if (d_counter + nitems >= sample_nr_near_sch_start) {
      to_consume = sample_nr_near_sch_start - d_counter;
    } else {
      to_consume = nitems;
    }
    result = false;
  } else {
    to_consume = 0;
    result = true;
  }

  d_counter += to_consume;
 // printf("Reach SCH consume: %d \n", to_consume);
  consume_each(to_consume);
  return result;
}

int gsm_receiver_cf::get_sch_chan_imp_resp(const gr_complex *in_0, gr_complex * chan_imp_resp)
{
  vector_complex correlation_buffer;
  vector_float power_buffer;
  vector_float window_energy_buffer;

  int strongest_window_nr;
  int burst_start = 0;
  int chan_imp_resp_center = 0;
  float max_correlation = 0;
  float energy = 0;

  for (int ii = SYNC_POS * d_OSR; ii < (SYNC_POS + SYNC_SEARCH_RANGE) *d_OSR; ii++) {
    gr_complex correlation = correlate_sequence(&d_sch_training_seq[5], N_SYNC_BITS - 10, &in_0[ii]);
    correlation_buffer.push_back(correlation);
    power_buffer.push_back(pow(abs(correlation), 2));
  }

  //compute window energies
  vector_float::iterator iter = power_buffer.begin();
  bool loop_end = false;
  while (iter != power_buffer.end()) {
    vector_float::iterator iter_ii = iter;
    energy = 0;

    for (int ii = 0; ii < (d_chan_imp_length) *d_OSR; ii++, iter_ii++) {
      if (iter_ii == power_buffer.end()) {
        loop_end = true;
        break;
      }
      energy += (*iter_ii);
    }
    if (loop_end) {
      break;
    }
    iter++;
    window_energy_buffer.push_back(energy);
  }

  strongest_window_nr = max_element(window_energy_buffer.begin(), window_energy_buffer.end()) - window_energy_buffer.begin();
//   d_channel_imp_resp.clear();

  max_correlation = 0;
  for (int ii = 0; ii < (d_chan_imp_length) *d_OSR; ii++) {
    gr_complex correlation = correlation_buffer[strongest_window_nr + ii];
    if (abs(correlation) > max_correlation) {
      chan_imp_resp_center = ii;
      max_correlation = abs(correlation);
    }
//     d_channel_imp_resp.push_back(correlation);
    chan_imp_resp[ii] = correlation;
  }

  burst_start = strongest_window_nr + chan_imp_resp_center - 48 * d_OSR - 2 * d_OSR + 2 + SYNC_POS * d_OSR;
  return burst_start;
}

void gsm_receiver_cf::detect_burst(const gr_complex * in_0, gr_complex * chan_imp_resp, int burst_start, unsigned char * output_binary)
{
  float output[BURST_SIZE];
  gr_complex rhh_temp[CHAN_IMP_RESP_LENGTH*d_OSR];
  gr_complex rhh[CHAN_IMP_RESP_LENGTH];
  gr_complex filtered_burst[BURST_SIZE];
  int start_state = 3;
  unsigned int stop_states[2] = {4, 12};

  autocorrelation(chan_imp_resp, rhh_temp, d_chan_imp_length*d_OSR);
  for (int ii = 0; ii < (d_chan_imp_length); ii++) {
    rhh[ii] = conj(rhh_temp[ii*d_OSR]);
  }

  mafi(&in_0[burst_start], BURST_SIZE, chan_imp_resp, d_chan_imp_length*d_OSR, filtered_burst);

  viterbi_detector(filtered_burst, BURST_SIZE, rhh, start_state, stop_states, 2, output);

  for (int i = 0; i < BURST_SIZE ; i++) {
    output_binary[i] = (output[i] > 0);
  }
}

//TODO consider placing this funtion in a separate class for signal processing
void gsm_receiver_cf::gmsk_mapper(const unsigned char * in_0, int nitems, gr_complex * gmsk_output, gr_complex start_point)
{
  gr_complex j = gr_complex(0.0, 1.0);

  int current_symbol;
  int encoded_symbol;
  int previous_symbol = 2 * in_0[0] - 1;
  gmsk_output[0] = start_point;

  for (int i = 1; i < nitems; i++) {
    //change bits representation to NRZ
    current_symbol = 2 * in_0[i] - 1;
    //differentially encode
    encoded_symbol = current_symbol * previous_symbol;
    //and do gmsk mapping
    gmsk_output[i] = j * gr_complex(encoded_symbol, 0.0) * gmsk_output[i-1];
    previous_symbol = current_symbol;
  }
}

//TODO consider use of some generalized function for correlation and placing it in a separate class  for signal processing
gr_complex gsm_receiver_cf::correlate_sequence(const gr_complex * sequence, int length, const gr_complex * in_0)
{
  gr_complex result(0.0, 0.0);
  int sample_number = 0;

  for (int ii = 0; ii < length; ii++) {
    sample_number = (ii * d_OSR) ;
    result += sequence[ii] * conj(in_0[sample_number]);
  }

  result = result / gr_complex(length, 0);
  return result;
}

//computes autocorrelation for positive arguments
//TODO consider placing this funtion in a separate class for signal processing
inline void gsm_receiver_cf::autocorrelation(const gr_complex * in_0, gr_complex * out, int nitems)
{
  int i, k;
  for (k = nitems - 1; k >= 0; k--) {
    out[k] = gr_complex(0, 0);
    for (i = k; i < nitems; i++) {
      out[k] += in_0[i] * conj(in_0[i-k]);
    }
  }
}

//TODO consider use of some generalized function for filtering and placing it in a separate class  for signal processing
inline void gsm_receiver_cf::mafi(const gr_complex * in_0, int nitems, gr_complex * filter, int filter_length, gr_complex * output)
{
  int ii = 0, n, a;

  for (n = 0; n < nitems; n++) {
    a = n * d_OSR;
    output[n] = 0;
    ii = 0;

    while (ii < filter_length) {
      if ((a + ii) >= nitems*d_OSR)
        break;
      output[n] += in_0[a+ii] * filter[ii];
      ii++;
    }
  }
}

//TODO: get_norm_chan_imp_resp is similar to get_sch_chan_imp_resp - consider joining this two functions
//TODO: this is place where most errors are introduced and can be corrected by improvements to this fuction
//especially computations of strongest_window_nr
int gsm_receiver_cf::get_norm_chan_imp_resp(const gr_complex *in_0, gr_complex * chan_imp_resp, int bcc)
{
  vector_complex correlation_buffer;
  vector_float power_buffer;
  vector_float window_energy_buffer;

  int strongest_window_nr;
  int burst_start = 0;
  int chan_imp_resp_center = 0;
  float max_correlation = 0;
  float energy = 0;

  int search_center = (int)((TRAIN_POS + GUARD_PERIOD) * d_OSR);
  int search_start_pos = search_center + 1;
//   int search_start_pos = search_center -  d_chan_imp_length * d_OSR;
  int search_stop_pos = search_center + d_chan_imp_length * d_OSR + 2 * d_OSR;

  for (int ii = search_start_pos; ii < search_stop_pos; ii++) {
    gr_complex correlation = correlate_sequence(&d_norm_training_seq[bcc][TRAIN_BEGINNING], N_TRAIN_BITS - 10, &in_0[ii]);

    correlation_buffer.push_back(correlation);
    power_buffer.push_back(pow(abs(correlation), 2));
  }

  //compute window energies
  vector_float::iterator iter = power_buffer.begin();
  bool loop_end = false;
  while (iter != power_buffer.end()) {
    vector_float::iterator iter_ii = iter;
    energy = 0;

    for (int ii = 0; ii < (d_chan_imp_length - 2)*d_OSR; ii++, iter_ii++) {
      if (iter_ii == power_buffer.end()) {
        loop_end = true;
        break;
      }
      energy += (*iter_ii);
    }
    if (loop_end) {
      break;
    }
    iter++;

    window_energy_buffer.push_back(energy);
  }
  //!why doesn't this work
  strongest_window_nr = max_element(window_energy_buffer.begin(), window_energy_buffer.end()) - window_energy_buffer.begin();
  strongest_window_nr = 3; //! so I have to override it here

  max_correlation = 0;
  for (int ii = 0; ii < (d_chan_imp_length)*d_OSR; ii++) {
    gr_complex correlation = correlation_buffer[strongest_window_nr + ii];
    if (abs(correlation) > max_correlation) {
      chan_imp_resp_center = ii;
      max_correlation = abs(correlation);
    }
//     d_channel_imp_resp.push_back(correlation);
    chan_imp_resp[ii] = correlation;
  }
  // We want to use the first sample of the impulseresponse, and the
  // corresponding samples of the received signal.
  // the variable sync_w should contain the beginning of the used part of
  // training sequence, which is 3+57+1+6=67 bits into the burst. That is
  // we have that sync_t16 equals first sample in bit number 67.

  burst_start = search_start_pos + chan_imp_resp_center + strongest_window_nr - TRAIN_POS * d_OSR;

  // GMSK modulator introduces ISI - each bit is expanded for 3*Tb
  // and it's maximum value is in the last bit period, so burst starts
  // 2*Tb earlier
  burst_start -= 2 * d_OSR;
  burst_start += 2;
  //std::cout << " burst_start: " << burst_start << " center: " << ((float)(search_start_pos + strongest_window_nr + chan_imp_resp_center)) / d_OSR << " stronegest window nr: " <<  strongest_window_nr << "\n";

  return burst_start;
}

burst_type gsm_receiver_cf::get_burst_type(int uchannel, int fn, int timeslot)
{
	int mfn=fn%51;
	if (timeslot == 0)
	{
		if (mfn % 10 == 0)return fcch_burst;
		else if ((mfn % 10) == 1)return sch_burst;
		else return normal_burst;
	}
	else return normal_burst;
}

void gsm_receiver_cf::diff_encode(const float *in,float *out,int length,float lastbit) {

	for (int i=0; i < length; i++) {
		out[i] = in[i] * lastbit;
		lastbit = out [i];
	}
}

void gsm_receiver_cf::consume_each(int how_many_items) {

	gr_block::consume(0, how_many_items);
	gr_block::consume(1, how_many_items);
	d_symbol_counter+=how_many_items;
	//printf("Total Drops: %u ; drops: %d \n ", d_symbol_counter, how_many_items);

}
