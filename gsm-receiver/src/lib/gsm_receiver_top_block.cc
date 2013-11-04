/*
 * gsm_receiver_top_block.cc
 *
 *  Created on: 2012��10��22��
 *      Author: hhh
 */

#include "gsm_receiver_top_block.h"




gsm_receiver_top_block_sptr make_gsm_receiver_top_block(
									struct SDRconfiguration sdr_config
												  )
{
	 return gnuradio::get_initial_sptr(new gsm_receiver_top_block(sdr_config));
}



gsm_receiver_top_block::gsm_receiver_top_block(struct SDRconfiguration sdr_config):
												gr_top_block("gsm_receiver_top_block")
{

	// -HW: build everything, blocks here, if the configuration are too much,
	//	seperated it into a function
	d_sdr_config = sdr_config;
	rates_element.clock_rate = d_sdr_config.freq;
	rates_element.input_rate = d_sdr_config.freq / d_sdr_config.decim;
	rates_element.gsm_symb_rate = 1625000.0 / 6.0;
	rates_element.sps = (rates_element.input_rate / rates_element.gsm_symb_rate) / d_sdr_config.osr;
	filter_parameter.filter_cutoff = 135e3;
	filter_parameter.filter_t_width = 10e3;
	filter_parameter.offset = 0;

	tuner_p = new class tuner(this);
	synchronizer_p = new class synchronizer(this);

	if(d_sdr_config.is_double_downlink == true)
	{
		grc_config = "DD";
	}
	else if(d_sdr_config.is_double_downlink == false)
	{
		grc_config = "DU";
	}else
	{
		grc_config = "DU";
	}

	if(true == sdr_config.realtime)
	{
		usrp_source = set_usrp_source();
	}
	else
	{
		std::cout << d_sdr_config.ch0_infile << std::endl;
		std::cout << d_sdr_config.ch1_infile << std::endl;
		file_source[0] = set_source(d_sdr_config.ch0_infile);
		file_source[1] = set_source(d_sdr_config.ch1_infile);
	}
	interpolator[0] = set_interpolator();
	interpolator[1] = set_interpolator();
	set_double_filter();
	gsm_receiver = set_gsm_receiver_cf(
										tuner_p,
										synchronizer_p,
										"",
										grc_config,
										d_sdr_config);

	//std::cout << "grc_config :" << grc_config << std::endl;

	if(d_sdr_config.decim > 128)
	{
		std::vector<float> fir_tap;
		double input_rate;
		input_rate = 52e6 / (d_sdr_config.decim / 2);

		class gr_firdes firdes;

		fir_tap = firdes.low_pass(1.0, input_rate, 270e3, 10e3, firdes.WIN_HAMMING);//implement low pass fir filter


		gr_fir_filter_ccf_sptr my_swdecim[2];
		my_swdecim[0] = gr_make_fir_filter_ccf(2, fir_tap);
		my_swdecim[1] = gr_make_fir_filter_ccf(2, fir_tap);
		if(true == d_sdr_config.realtime)
		{
			connect(usrp_source,0, deinter, 0);

			connect(deinter, 0, my_swdecim[0], 0);
			connect(deinter, 1, my_swdecim[1], 0);
		}
		else
		{

			connect(file_source[0], 0, my_swdecim[0], 0);
			connect(file_source[1], 0, my_swdecim[1], 0);
		}

		connect(my_swdecim[0], 0, filter[0], 0);

		connect(my_swdecim[1], 0, filter[1], 0);
	}
	else
	{

		if(true == d_sdr_config.realtime)
		{
			//usrp_source->serial_number();
			connect(usrp_source,0, deinter, 0);
			connect(deinter, 0, filter[0], 0); //downlink channel
			connect(deinter, 1, filter[1], 0);
		}
		else
		{
			connect(file_source[0], 0, filter[0], 0); //downlink channel
			connect(file_source[1], 0, filter[1], 0);
		}
	}




	connect(filter[0], 0, interpolator[0], 0);


	connect(filter[1], 0, interpolator[1], 0);

	if(d_sdr_config.fcch_ch == 0)
	{
		connect(interpolator[0], 0, gsm_receiver, 0);
		connect(interpolator[1], 0, gsm_receiver, 1);
	}
	else
	{
		connect(interpolator[1], 0, gsm_receiver, 0);
		connect(interpolator[0], 0, gsm_receiver, 1);
	}


}

gsm_receiver_top_block::~gsm_receiver_top_block()
{
	delete tuner_p;
	delete synchronizer_p;
}

bool gsm_receiver_top_block::set_usrp_freq(double chan0_freq, double chan1_freq, double offset, class gsm_receiver_top_block *p)
{
	double tmp_freq[2];
	double tmp_offset = 0;
	tmp_offset = offset;

	tmp_freq[0] = chan0_freq;
	tmp_freq[0] = tmp_freq[0] - tmp_offset;

	tmp_freq[1] = chan1_freq;
	tmp_freq[1] = tmp_freq[1] - tmp_offset;



	usrp_tune_result r;
	if(this->d_sdr_config.fcch_ch == 0)
	{
		p->usrp_source->tune(0, p->subdev[0], tmp_freq[0], &r);
		p->usrp_source->tune(1, p->subdev[1], tmp_freq[1], &r);
	}
	else
	{
		p->usrp_source->tune(1, p->subdev[1], tmp_freq[0], &r);
		p->usrp_source->tune(0, p->subdev[0], tmp_freq[1], &r);
	}

	return true;
}

double gsm_receiver_top_block::get_freq_from_arfcn(int chan)
{
	double freq = 0.0;
	if((chan >= 0) && (chan <= 124))
	{
		freq = 890.0e6 + 0.2e6 * chan + 45.0e6;
	}
	else
	if((chan >= 128) && (chan <= 251))
	{
		freq = 824.2e6 + 0.2e6 * (chan - 128) + 45.0e6;
	}
	else
	if((chan >= 512) && (chan <= 885))
	{
		freq = 1710.2e6 + 0.2e6 * (chan - 512) + 95.0e6;
	}

	return freq;
}

gr_file_source_sptr gsm_receiver_top_block::set_source(const std::string &inputfile)
{
	return gr_make_file_source(sizeof(gr_complex), inputfile.c_str(), false);
}

usrp_subdev_spec	gsm_receiver_top_block::str_to_subdev(std::string spec_str)
{
  usrp_subdev_spec spec;
  if(spec_str == "A" || spec_str == "A:0" || spec_str == "0:0") {
    spec.side = 0;
    spec.subdev = 0;
  }
  else if(spec_str == "A:1" || spec_str == "0:1") {
    spec.side = 0;
    spec.subdev = 1;
  }
  else if(spec_str == "B" || spec_str == "B:0" || spec_str == "1:0") {
    spec.side = 1;
    spec.subdev = 0;
  }
  else if(spec_str == "B:1" || spec_str == "1:1") {
    spec.side = 1;
    spec.subdev = 1;
  }
  else {
    throw std::range_error("Incorrect subdevice specifications.\n");
  }

  return spec;
}


usrp_source_c_sptr gsm_receiver_top_block:: set_usrp_source(void)
{
	usrp_subdev_spec spec;
	double tmp_freq = 0.0;
	tmp_freq = get_freq_from_arfcn(d_sdr_config.ch0_arfcn);
	//std::cout << "freq : " << tmp_freq << std::endl;
	//std::cout << "decim : " << d_sdr_config.decim << std::endl;

	int tmp_decim = 0;
	if(d_sdr_config.decim > 128)
	{
		tmp_decim = d_sdr_config.decim / 2;
	}
	else
	{
		tmp_decim = d_sdr_config.decim;
	}
	//std::cout << "tmp_decim : " << tmp_decim << std::endl;

	usrp_source_c_sptr usrp = usrp_make_source_c(d_sdr_config.which_board, tmp_decim, 1, -1, 0, 4096, 16);



	  if(d_sdr_config.width8 == 1) {
	    int sample_width = 8;
	    int sample_shift = 8;
	    int format = usrp->make_format(sample_width, sample_shift);
	    int r = usrp->set_format(format);
	    printf("width8: format=%d  r=%d\n", format, r);
	  }

	unsigned int mymux = 0x00003210;
	usrp->set_mux(mymux);

	usrp->set_nchannels(2);

	usrp_subdev_spec myspec1(0,0);
	db_base_sptr mysubdev1 = usrp->selected_subdev(myspec1);
	subdev[0] = mysubdev1;
	usrp_tune_result r;
	bool ok = usrp->tune(0, mysubdev1, tmp_freq, &r);//DDC0
	if(!ok) {
	    throw std::runtime_error("Could not set frequency.");
	}
	mysubdev1->set_gain(d_sdr_config.rxa_gain);


	usrp_subdev_spec myspec2(1,0);
	db_base_sptr mysubdev2 = usrp->selected_subdev(myspec2);
	subdev[1] = mysubdev2;

	double uplink_freq = tmp_freq;//freq - 45000000.0;
	ok = usrp->tune(1, mysubdev2, uplink_freq, &r);//DDC1
	if(!ok) {
	   throw std::runtime_error("Could not set frequency.");
	}

	mysubdev2->set_gain(d_sdr_config.rxb_gain);

	deinter = gr_make_deinterleave(sizeof(gr_complex));

	return usrp;
}

gr_block_sptr gsm_receiver_top_block::set_sink(const std::string &outpufile)
{
	return gr_make_file_sink(sizeof(float), outpufile.c_str());
}

gr_freq_xlating_fir_filter_ccf_sptr gsm_receiver_top_block::set_filter(void)
{
	class gr_firdes _gr_firdes;
	filter_parameter.fliter_taps = _gr_firdes.low_pass(
																1.0,
																rates_element.input_rate,
																filter_parameter.filter_cutoff,
																filter_parameter.filter_t_width,
																_gr_firdes.WIN_HAMMING
																);
	filter_parameter._filter = gr_make_freq_xlating_fir_filter_ccf(
																			1,
																			filter_parameter.fliter_taps,
																			filter_parameter.offset,
																			rates_element.input_rate
																			);
	return filter_parameter._filter;
}

bool gsm_receiver_top_block::set_double_filter(void)
{
	filter[0] = set_filter();
	filter[1] = set_filter();
	return true;
}


gr_fractional_interpolator_cc_sptr gsm_receiver_top_block::set_interpolator(void)
{
	return gr_make_fractional_interpolator_cc(0, rates_element.sps);
}

bool gsm_receiver_top_block::set_double_chan_center_frequency(float center_freq)
{
	filter[0]->set_center_freq(center_freq);
	filter[1]->set_center_freq(center_freq);
	return true;
}


bool gsm_receiver_top_block::timing(float center_freq)
{
	return true;
}

gr_vector_to_stream_sptr gsm_receiver_top_block::set_convert(void)
{
	return gr_make_vector_to_stream(sizeof(float), 142);
}


gsm_receiver_cf_sptr gsm_receiver_top_block::set_gsm_receiver_cf(
												gr_feval_dd *tuner,
												gr_feval_dd *synchronizer,
												std::string key,
												std::string configuration,
												struct SDRconfiguration sdr_config
												)
{
	return gsm_make_receiver_cf(tuner, synchronizer, sdr_config.osr, key, configuration,\
									sdr_config);
}




bool gsm_receiver_top_block:: scan_bts_next_arfcn(class gsm_receiver_top_block *p)
{
	class gsm_receiver_top_block *_p = p;
	int count = 0;
	static std::vector<int> history;
	if(_p->gsm_receiver->scan_bts_flag.scan_flag == 1)
	{
/*
		std::cout << "tunner : " << std::endl;
		std::cout << "scan_bts_flag.fcch_found : " << _p->gsm_receiver->scan_bts_flag.fcch_found << std::endl;
		std::cout << "scan_bts_flag.fcch_count : " << _p->gsm_receiver->scan_bts_flag.fcch_count << std::endl;
		std::cout << "scan_bts_flag.fcch_timeout : " << _p->gsm_receiver->scan_bts_flag.fcch_timeout << std::endl;
		std::cout << "scan_bts_flag.fcch_unfcount : " << _p->gsm_receiver->scan_bts_flag.fcch_unfcount << std::endl;




		std::cout << "scan_bts_flag.sch_count : " << _p->gsm_receiver->scan_bts_flag.sch_count << std::endl;
		std::cout << "scan_bts_flag.sch_found : " << _p->gsm_receiver->scan_bts_flag.sch_found << std::endl;
		std::cout << "scan_bts_flag.sch_timeout : " << _p->gsm_receiver->scan_bts_flag.sch_timeout << std::endl;
		std::cout << "scan_bts_flag.sch_unfcount : " << _p->gsm_receiver->scan_bts_flag.sch_unfcount << std::endl;

		std::cout << "scan_bts_flag.scan_count : " << _p->gsm_receiver->scan_bts_flag.scan_count << std::endl;
		std::cout << "scan_bts_flag.d_freq_offset_p : " << (_p->gsm_receiver->scan_bts_flag.d_freq_offset_p) << std::endl;
*/

		if(_p->gsm_receiver->scan_bts_flag.sch_found == 3)
		{
			//_p->gsm_receiver->scan_bts_flag.scan_flag = 0;
			//count = 0;
			count = _p->gsm_receiver->scan_bts_flag.scan_count;
			_p->d_sdr_config.ch0_arfcn =  _p->d_sdr_config.scan_bts_chanstart + _p->gsm_receiver->scan_bts_flag.scan_count;
			if(_p->gsm_receiver->scan_bts_flag.d_freq_overflow == 0)
			{
				history.push_back(_p->d_sdr_config.ch0_arfcn - 1);
			}
			else
			{
				_p->gsm_receiver->scan_bts_flag.d_freq_overflow = 0;
			}
			memset(&_p->gsm_receiver->scan_bts_flag, 0, sizeof(struct SCAN_BTS));
			_p->gsm_receiver->scan_bts_flag.scan_flag = 1;
			_p->gsm_receiver->scan_bts_flag.scan_count = count;
		//	std::cout << "set freq :scan BTS arfcn: " << count << std::endl;
		//	std::cout << "set freq :scan BTS find 3 sch!!!" << std::endl;
		}


		if(_p->gsm_receiver->scan_bts_flag.fcch_timeout == 1)
		{
			_p->gsm_receiver->scan_bts_flag.fcch_timeout = 0;
			_p->gsm_receiver->scan_bts_flag.fcch_unfcount = 0;
			_p->d_sdr_config.ch0_arfcn = _p->d_sdr_config.scan_bts_chanstart + _p->gsm_receiver->scan_bts_flag.scan_count;
		//	count++;
		}


		if(_p->gsm_receiver->scan_bts_flag.sch_timeout == 1)
		{
			_p->gsm_receiver->scan_bts_flag.sch_timeout = 0;
			_p->d_sdr_config.ch0_arfcn = _p->d_sdr_config.scan_bts_chanstart + _p->gsm_receiver->scan_bts_flag.scan_count;
		//	count++;
		}


		if((_p->d_sdr_config.ch0_arfcn > _p->d_sdr_config.scan_bts_chanstop) &&
			(_p->gsm_receiver->scan_bts_flag.scan_count != 0))  // if (ch0_arfcn > scan_bts_chanstop) without (count != 0) at .ini, the Scan BTS will stop.
		{


			_p->gsm_receiver->scan_bts_flag.scan_flag = 0;
			memset(&_p->gsm_receiver->scan_bts_flag, 0, sizeof(_p->gsm_receiver->scan_bts_flag));
		//	count = 0;
			for(unsigned int i = 0; i < history.size(); i++)
			{

				if((history[i]>=96&&history[i]<=124)||(history[i]>=637&&history[i]<=736))
				{
					std::cout <<std::endl;
					std::cout << "history BTS :" << history[i] << "(UNICOM)";
				}
				else
				{
					std::cout <<std::endl;
					std::cout << "history BTS :" << history[i] << "(CMCC)";

				}
			}

			char output_arfcns[100];
			char encoded_block[200];

			memset(output_arfcns, '\0', 100*sizeof(char));
			memset(encoded_block, '\0', 200*sizeof(char));
			for(unsigned int i = 0; i < history.size(); i++) {
				sprintf(output_arfcns, "%s %d", output_arfcns, history[i]);

			}


			encodeblock((unsigned char*)output_arfcns, \
					(unsigned char *)encoded_block, 100 );


			printf("\n->>");
			printf("%s", output_arfcns);
			printf("<<-\n");


			std::cout <<std::endl;
			std::cout << "stop scan-bts!" << std::endl;
			exit(0);

		}
		//std::cout << "arfcn next arfcn : " << _p->d_sdr_config.ch0_arfcn << std::endl;
	}

}


// callback class detail

tuner::tuner(gsm_receiver_top_block *p)
{
	_p = p;
}



double tuner::eval(double x)
{
	double target_freq[2];
	//std::cerr << _p->d_whether_realtime << std::endl;

	_p->scan_bts_next_arfcn(_p);


	if(false == _p->d_sdr_config.realtime)
	{
		_p->set_double_chan_center_frequency(x);
	//	getchar();
	}
	else
	{
		if(true == _p->d_sdr_config.is_double_downlink)
		{
			target_freq[0] = _p->get_freq_from_arfcn(_p->d_sdr_config.ch0_arfcn);
			target_freq[1] = _p->get_freq_from_arfcn(_p->d_sdr_config.ch1_arfcn);
			_p->set_usrp_freq(target_freq[0], target_freq[1], x, _p);
		}
		else
		{
			target_freq[0] = _p->get_freq_from_arfcn(_p->d_sdr_config.ch0_arfcn);
			target_freq[1] = _p->get_freq_from_arfcn(_p->d_sdr_config.ch1_arfcn);
			if((1 <= _p->d_sdr_config.ch1_arfcn) && (_p->d_sdr_config.ch1_arfcn <= 124))
			{
				target_freq[1] -= 45000000.0;
			}
			if((512 <= _p->d_sdr_config.ch1_arfcn) && (_p->d_sdr_config.ch1_arfcn <= 885))
			{
				target_freq[1] -= 95000000.0;
			}

			_p->set_usrp_freq(target_freq[0], target_freq[1], x, _p);
		}
	}

	return x;
}

synchronizer::synchronizer(class gsm_receiver_top_block *p)
{
	_p = p;
}

double synchronizer::eval(double x)
{

	/*
	static int count = 0;
	if(_p->gsm_receiver->scan_bts_flag.scan_flag == 1)
	{
		std::cout << "scan_bts_flag.fcch_found : " << _p->gsm_receiver->scan_bts_flag.fcch_found << std::endl;
		std::cout << "scan_bts_flag.fcch_count : " << _p->gsm_receiver->scan_bts_flag.fcch_count << std::endl;
		std::cout << "scan_bts_flag.fcch_timeout : " << _p->gsm_receiver->scan_bts_flag.fcch_timeout << std::endl;
		std::cout << "scan_bts_flag.fcch_unfcount : " << _p->gsm_receiver->scan_bts_flag.fcch_unfcount << std::endl;


		std::cout << "scan_bts_flag.sch_count : " << _p->gsm_receiver->scan_bts_flag.sch_count << std::endl;
		std::cout << "scan_bts_flag.sch_found : " << _p->gsm_receiver->scan_bts_flag.sch_found << std::endl;
		std::cout << "scan_bts_flag.sch_timeout : " << _p->gsm_receiver->scan_bts_flag.sch_timeout << std::endl;
		std::cout << "scan_bts_flag.sch_unfcount : " << _p->gsm_receiver->scan_bts_flag.sch_unfcount << std::endl;


		if(_p->gsm_receiver->scan_bts_flag.fcch_timeout == 1)
		{
			_p->gsm_receiver->scan_bts_flag.fcch_timeout = 0;
			_p->d_sdr_config.ch0_arfcn = _p->d_sdr_config.scan_bts_chanstart + count;
			count++;
		}

		if(_p->gsm_receiver->scan_bts_flag.sch_timeout == 1 ||
		   _p->gsm_receiver->scan_bts_flag.sch_found == 2)
		{
			_p->d_sdr_config.ch0_arfcn = _p->d_sdr_config.scan_bts_chanstart + count;
			count++;
		}

		if(_p->gsm_receiver->scan_bts_flag.sch_found == 1)
		{
			_p->gsm_receiver->scan_bts_flag.scan_flag = 0;
			count = 0;
			std::cout << "found sch!!!" << std::endl;
		}

		if(_p->d_sdr_config.ch0_arfcn > _p->d_sdr_config.scan_bts_chanstop)
		{
			_p->gsm_receiver->scan_bts_flag.scan_flag = 0;
			count = 0;
			std::cout << "no bts!" << std::endl;
		}
		std::cout << "arfcn count : " << _p->d_sdr_config.ch0_arfcn << std::endl;



	}  */
//	exit(1);

	_p->timing(x);
	return x;
}













