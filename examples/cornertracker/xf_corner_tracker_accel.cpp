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
#include "xf_corner_tracker_config.h"

void cornerTracker(xf::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1> & flow, xf::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1> & flow_iter, xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1> mat_imagepyr1[NUM_LEVELS] , xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1> mat_imagepyr2[NUM_LEVELS] , xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &inHarris, xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &outHarris, unsigned int *list, unsigned long *listfixed, int pyr_h[NUM_LEVELS], int pyr_w[NUM_LEVELS], unsigned int *num_corners, unsigned int harrisThresh, bool *harris_flag)
{
	uint16_t Thresh = harrisThresh;
	float K_f = 0.04;
	uint16_t k = K_f*(1<<16);
	uint32_t nCorners = 0;
	for(int l=0; l<NUM_LEVELS; l++)
	{
		mat_imagepyr1[l].rows = pyr_h[l];
		mat_imagepyr1[l].cols = pyr_w[l];
		mat_imagepyr1[l].size = pyr_h[l]*pyr_w[l];
		mat_imagepyr2[l].rows = pyr_h[l];
		mat_imagepyr2[l].cols = pyr_w[l];	
		mat_imagepyr2[l].size = pyr_h[l]*pyr_w[l];
	}
	
	//Computing the image pyramid and harris corners in parallel
	if(*harris_flag == true)
	{
	#pragma SDS async(1)
		xf::cornerHarris<FILTER_WIDTH,BLOCK_WIDTH,NMS_RADIUS,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(inHarris, outHarris, Thresh, k);
	#pragma SDS async(2)
		xf::cornersImgToList<MAXCORNERS,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(outHarris, list, &nCorners);
	}
	
	//creating image pyramid
	for(int pyr_comp=0;pyr_comp<NUM_LEVELS-1; pyr_comp++)
	{
	#pragma SDS async(3)
	#pragma SDS resource(1)
		xf::pyrDown<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(mat_imagepyr1[pyr_comp], mat_imagepyr1[pyr_comp+1]);
	#pragma SDS async(4)
	#pragma SDS resource(2)
		xf::pyrDown<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(mat_imagepyr2[pyr_comp], mat_imagepyr2[pyr_comp+1]);
	#pragma SDS wait(3)
	#pragma SDS wait(4)	
	}

	ap_uint<1> init_flag = 0;
	bool flag_flowin = 1;
	flow.rows = pyr_h[NUM_LEVELS-1];
	flow.cols = pyr_w[NUM_LEVELS-1];
	flow.size = pyr_h[NUM_LEVELS-1]*pyr_w[NUM_LEVELS-1];
	flow_iter.rows = pyr_h[NUM_LEVELS-1];
	flow_iter.cols = pyr_w[NUM_LEVELS-1];
	flow_iter.size = pyr_h[NUM_LEVELS-1]*pyr_w[NUM_LEVELS-1];

	for (int l=NUM_LEVELS-1; l>=0; l--) {
		
		//compute current level height
		int curr_height = pyr_h[l];
		int curr_width = pyr_w[l];
		
		//compute the flow vectors for the current pyramid level iteratively
		for(int iterations=0;iterations<NUM_ITERATIONS; iterations++)
		{
			bool scale_up_flag = (iterations==0)&&(l != NUM_LEVELS-1);
			int next_height = (scale_up_flag==1)?pyr_h[l+1]:pyr_h[l]; 
			int next_width  = (scale_up_flag==1)?pyr_w[l+1]:pyr_w[l];
			float scale_in = (next_height - 1)*1.0/(curr_height - 1);
			if((iterations==0) && (l==NUM_LEVELS-1))
			{
				init_flag = 1;
			}
			else
			{
				init_flag = 0;
			}
			if(flag_flowin)
			{
				flow.rows = pyr_h[l];
				flow.cols = pyr_w[l];
				flow.size = pyr_h[l]*pyr_w[l];
				xf::densePyrOpticalFlow<NUM_LEVELS, NUM_LINES_FINDIT, WINSIZE_OFLOW, TYPE_FLOW_WIDTH, TYPE_FLOW_INT, XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(mat_imagepyr1[l], mat_imagepyr2[l], flow_iter, flow, l, scale_up_flag, scale_in, init_flag);
				flag_flowin = 0;
			}
			else
			{
				flow_iter.rows = pyr_h[l];
				flow_iter.cols = pyr_w[l];
				flow_iter.size = pyr_h[l]*pyr_w[l];
				xf::densePyrOpticalFlow<NUM_LEVELS, NUM_LINES_FINDIT, WINSIZE_OFLOW, TYPE_FLOW_WIDTH, TYPE_FLOW_INT, XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(mat_imagepyr1[l], mat_imagepyr2[l], flow, flow_iter, l, scale_up_flag, scale_in, init_flag);
				flag_flowin = 1;
			}
		}//end iterative coptical flow computation
	} // end pyramidal iterative optical flow HLS computation
	if(*harris_flag == true)
	{
	#pragma SDS wait(1)
	#pragma SDS wait(2)	
		*num_corners = nCorners;
	}
	if(flag_flowin)
	{
		xf::cornerUpdate<MAXCORNERS,XF_32UC1,HEIGHT,WIDTH,XF_NPPC1>(listfixed, list, *num_corners, flow_iter, (ap_uint<1>)(*harris_flag));
	}                                                                                
	else                                                                             
	{                                                                                
		xf::cornerUpdate<MAXCORNERS,XF_32UC1,HEIGHT,WIDTH,XF_NPPC1>(listfixed, list, *num_corners, flow, (ap_uint<1>)(*harris_flag));
	}

	if(*harris_flag == true)
	{
		*harris_flag = false;
	}
	return;
}
