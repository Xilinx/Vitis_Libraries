/***************************************************************************
 Copyright (c) 2019, Xilinx, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 3. Neither the name of the copyright holder nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/
#ifndef _XF_SW_UTILS_H_
#define _XF_SW_UTILS_H_

#include "xf_common.h"
#include <iostream>
#include <cmath>

#if __SDSCC__
#include "sds_lib.h"


class perf_counter
{
public:
     uint64_t tot, cnt, calls;
     perf_counter() : tot(0), cnt(0), calls(0) {};
     inline void reset() { tot = cnt = calls = 0; }
     inline void start() { cnt = sds_clock_counter(); calls++; };
     inline void stop() { tot += (sds_clock_counter() - cnt); };
     inline uint64_t avg_cpu_cycles() { std::cout << "elapsed time "<< ((tot+(calls>>1)) / calls) << std::endl; return ((tot+(calls>>1)) / calls); };
};
#endif

namespace xf{

	template <int _PTYPE, int _ROWS, int _COLS, int _NPC>
	xf::Mat<_PTYPE, _ROWS, _COLS, _NPC> imread(char *img,  int type){

		cv::Mat img_load = cv::imread(img,type);

		xf::Mat<_PTYPE, _ROWS, _COLS, _NPC> input(img_load.rows, img_load.cols);

		if(img_load.data == NULL){
			std::cout << "\nError : Couldn't open the image at " << img << "\n" << std::endl;
			exit(-1);
		}

		input.copyTo(img_load.data);

		return input;
	}

	template <int _PTYPE, int _ROWS, int _COLS, int _NPC>
	void imwrite(const char *str, xf::Mat<_PTYPE, _ROWS, _COLS, _NPC> &output){

		int list_ptype[] = {CV_8UC1, CV_16UC1, CV_16SC1, CV_32SC1, CV_32FC1,  CV_32SC1, CV_16UC1, CV_32SC1, CV_8UC1, CV_8UC3, CV_16UC3, CV_16SC3};
		int _PTYPE_CV = list_ptype[_PTYPE];

			cv::Mat input(output.rows, output.cols, _PTYPE_CV);
			input.data = output.copyFrom();
			cv::imwrite(str,input);
	}

	template <int _PTYPE, int _ROWS, int _COLS, int _NPC>
	void absDiff(cv::Mat &cv_img, xf::Mat<_PTYPE, _ROWS, _COLS, _NPC>& xf_img, cv::Mat &diff_img ){

		assert((cv_img.rows == xf_img.rows) && (cv_img.cols == xf_img.cols) && "Sizes of cv and xf images should be same");
		assert((xf_img.rows == diff_img.rows) && (xf_img.cols == diff_img.cols) && "Sizes of xf and diff images should be same");
		assert(((_NPC == XF_NPPC8) || (_NPC == XF_NPPC4) || (_NPC == XF_NPPC2) || (_NPC == XF_NPPC1)) && "Only XF_NPPC1, XF_NPPC2, XF_NPPC4, XF_NPPC8 are supported");
		assert((cv_img.channels() == XF_CHANNELS(_PTYPE, _NPC)) && "Number of channels of cv and xf images does not match");

		int cv_bitdepth = 8;
		int num_chnls   = cv_img.channels();
		int cv_nbytes   = 1;

		if(cv_img.depth()==CV_8U){
			cv_bitdepth = 8;
		}
		else if(cv_img.depth()==CV_16S || cv_img.depth()==CV_16U){
			cv_bitdepth = 16;
		}
		else if(cv_img.depth()==CV_32S || cv_img.depth()==CV_32F){
			cv_bitdepth = 32;
		}
		else{
			printf("OpenCV image's depth not supported");
			return;
		}

		int cv_pixdepth = cv_bitdepth*num_chnls;
		cv_nbytes       = cv_bitdepth/8;

		int ch 	       = 0;
		int xf_npc_idx = 0;
		int diff_ptr   = 0;
		int xf_ptr     = 0;
		int cv_ptr     = 0;
		XF_CTUNAME(_PTYPE, _NPC) cv_val = 0, xf_val = 0, diff_val = 0;
		XF_TNAME(_PTYPE, _NPC) xf_val_total = 0;

		for(int r=0, xf_ptr=0; r<cv_img.rows;++r){
			for(int c=0; c<cv_img.cols >> XF_BITSHIFT(_NPC);++c){
#ifdef __SDSVHLS__
				xf_val_total = xf_img.data[xf_ptr++];

				for(int b=0; b < _NPC; ++b){
					for(int c=0; c < num_chnls; ++c){
						for (int l=0; l < cv_nbytes; ++l,++xf_npc_idx) {
							cv_val.range(((l+1)*8)-1, l*8)  = cv_img.data[cv_ptr++];
							xf_val.range(((l+1)*8)-1, l*8)  = xf_val_total.range(((xf_npc_idx+1)*8)-1, xf_npc_idx*8);
						}
						diff_val = __ABS((int)cv_val - (int)xf_val);
						for (int l=0; l < cv_nbytes; ++l) {
							diff_img.data[diff_ptr++] = diff_val.range(((l+1)*8)-1, l*8);
						}
					}
				}
				xf_npc_idx = 0;

#else
				for(int xf_npc_idx = 0; xf_npc_idx < _NPC; ++xf_npc_idx){
					for (int ch = 0; ch < num_chnls; ++ch) {

						xf_val = xf_img.data[xf_ptr].chnl[xf_npc_idx][ch];

						for (int b=0; b < cv_nbytes; ++b) {
							cv_val.range(((b+1)*8)-1, b*8) = cv_img.data[cv_ptr++];
						}
						diff_val = __ABS((int)cv_val- (int)xf_val);
						for (int b=0; b < cv_nbytes; ++b) {
							diff_img.data[diff_ptr++] = diff_val.range(((b+1)*8)-1, b*8);
						}
					}
				}
				++xf_ptr;
#endif
			}
		}
	}

void analyzeDiff(cv::Mat &diff_img, int err_thresh, float &err_per)
{
	int cv_bitdepth;
	if(diff_img.depth()==CV_8U){
		cv_bitdepth = 8;
	}
	else if(diff_img.depth()==CV_16U || diff_img.depth()==CV_16S){
		cv_bitdepth = 16;
	}
	else if(diff_img.depth()==CV_32S || diff_img.depth()==CV_32F){
		cv_bitdepth = 32;
	}else{
		printf("OpenCV image's depth not supported for this function");
		return;
	}

	int cnt = 0;
	double minval=std::pow(2.0,cv_bitdepth),maxval=0;
	int max_fix = (int) (std::pow(2.0,cv_bitdepth) - 1.0);
	for (int i=0; i<diff_img.rows; i++) {
		for(int j=0; j<diff_img.cols; j++) {
			int v = 0;
			for (int k=0; k<diff_img.channels(); k++) {
				int v_tmp;float v_tmp1;
				if (diff_img.channels() == 1) {
					if (cv_bitdepth == 8)
						v_tmp = (int)diff_img.at<unsigned char>(i,j);
					else if(cv_bitdepth == 16 && diff_img.depth()==CV_16U)  // 16 bitdepth
						v_tmp = (int)diff_img.at<unsigned short>(i,j);
					else if(cv_bitdepth == 16 && diff_img.depth()==CV_16S)  // 16 bitdepth
						v_tmp = (int)diff_img.at<short>(i,j);
					else if(cv_bitdepth == 32 && diff_img.depth()==CV_32S)
						v_tmp = (int)diff_img.at<int>(i,j);
					else
						v_tmp1 = (float)diff_img.at<float>(i,j);
				}
				else // 3 channels
					v_tmp = (int)diff_img.at<cv::Vec3b>(i,j)[k];

				if (v_tmp > v)
					v = v_tmp;
			}
			if (v>err_thresh)
			{
				cnt++;
				if(diff_img.depth()==CV_8U)
					diff_img.at<unsigned char>(i,j) = max_fix;
				else if (diff_img.depth()==CV_16U)
					diff_img.at<unsigned short>(i,j) = max_fix;
				else if (diff_img.depth()==CV_16S)
					diff_img.at<short>(i,j) = max_fix;
				else if (diff_img.depth()==CV_32S)
					diff_img.at<int>(i,j) = max_fix;
				else
					diff_img.at<float>(i,j) = (float)max_fix;
			}
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;

		}
	}
	err_per = 100.0*(float)cnt/(diff_img.rows*diff_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);
}
}

#endif

