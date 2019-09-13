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

#include "xf_headers.h"
#include "xf_boundingbox_config.h"
/*#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
# include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>*/
#include "ap_int.h"
using namespace std;


int main(int argc, char** argv)
{
	cv::Mat in_img,in_img1, out_img,diff;

	//struct timespec start_time;
	//struct timespec end_time;

	if (argc != 3)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

#if GRAY
	/*  reading in the gray image  */
	in_img = cv::imread(argv[1],0);
	in_img1=in_img.clone();
	int num_box=atoi(argv[2]);
#else
	/*  reading in the color image  */
	in_img = cv::imread(argv[1],1);
	int num_box=atoi(argv[2]);
	cvtColor(in_img,in_img,cv::COLOR_BGR2RGBA);
	in_img1=in_img.clone();
#endif

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}



	unsigned int x_loc[MAX_BOXES],y_loc[MAX_BOXES],ROI_height[MAX_BOXES],ROI_width[MAX_BOXES];

/////////////////////////////////////Feeding ROI/////////////////////////////////////////

		x_loc[0]=50;											  // only 3-ROI are feeded, should be modified according to NUM_BOX
		y_loc[0]=150;
		ROI_height[0]=480;
		ROI_width[0]=640;

		x_loc[1]=0;
		y_loc[1]=0;
		ROI_height[1]=100;
		ROI_width[1]=200;

		x_loc[2]=50;
		y_loc[2]=50;
		ROI_height[2]=300;
		ROI_width[2]=300;

		x_loc[3]=45;
		y_loc[3]=45;
		ROI_height[3]=670;
		ROI_width[3]=670;

		x_loc[4]=67;
		y_loc[4]=67;
		ROI_height[4]=100;
		ROI_width[4]=100;

	//	int num_box= 3;
//////////////////////////////////end of Feeding ROI///////////////////////////////////////
#if GRAY
	int color_info[MAX_BOXES][3]={{255},{255},{255},{150},{56}};
#else
	int color_info[MAX_BOXES][4]={{255,0,0,255},{0,255,0,255},{0,0,255,255},{123,234,108,255},{122,255,167,255}}; //Feeding color information for each boundary should be modified if MAX_BOXES varies
#endif

#if GRAY
		out_img.create(in_img.rows,in_img.cols,in_img.depth());
		diff.create(in_img.rows,in_img.cols,in_img.depth());

#else
		diff.create(in_img.rows,in_img.cols,CV_8UC4);
		out_img.create(in_img.rows,in_img.cols,CV_8UC4);
#endif


	////////////////  reference code  ////////////////
		//clock_gettime(CLOCK_MONOTONIC, &start_time);

#if GRAY
		for(int i=0;i<num_box;i++)
		{
			for(int c=0;c<XF_CHANNELS(TYPE,NPIX);c++)
			{
			cv::rectangle(in_img1,cv::Rect(x_loc[i],y_loc[i],ROI_width[i],ROI_height[i]),cv::Scalar(color_info[i][0],0,0),1); //BGR format
			}
		}
#else
		for(int i=0;i<num_box;i++)
		{
			for(int c=0;c<XF_CHANNELS(TYPE,NPIX);c++)
			{
			cv::rectangle(in_img1,cv::Rect(x_loc[i],y_loc[i],ROI_width[i],ROI_height[i]),cv::Scalar(color_info[i][0],color_info[i][1],color_info[i][2],255),1); //BGR format
			}
		}
#endif


		//clock_gettime(CLOCK_MONOTONIC, &end_time);
		////float diff_latency = (end_time.tv_nsec - start_time.tv_nsec)/1e9 + end_time.tv_sec - start_time.tv_sec;
		//printf("\latency: %f ", diff_latency);

		cv::imwrite( "ocv_ref.jpg",in_img1);  // reference image

   //////////////////  end opencv reference code//////////

	////////////////////// HLS TOP function call ////////////////////////////

	 xf::Mat<TYPE, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows,in_img.cols);
	 xf::Mat<TYPE, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows,in_img.cols);

#if __SDSCC__

	  xf::Rect_ <int> *roi=( xf::Rect_ <int>*)sds_alloc_non_cacheable(MAX_BOXES*sizeof( xf::Rect_<int>));
	  xf::Scalar <4, unsigned char > *color=( xf::Scalar <4,unsigned char > *)sds_alloc_non_cacheable(MAX_BOXES*sizeof( xf::Scalar<4,unsigned char >));
#else

	  xf::Rect_ <int> *roi=( xf::Rect_ <int>*)malloc(MAX_BOXES*sizeof( xf::Rect_<int>));
	  xf::Scalar <4, unsigned char > *color=( xf::Scalar <4,unsigned char > *)malloc(MAX_BOXES*sizeof( xf::Scalar<4,unsigned char >));
#endif


	  	for(int i=0,j=0;i<(num_box);j++,i++)
	  		{
	  			roi[i].x  = x_loc[i];
	  			roi[i].y= y_loc[i];
	  			roi[i].height= ROI_height[i];
	  			roi[i].width = ROI_width[i];
	  		}

		for(int i=0;i<(num_box);i++)
		{
			for(int j=0;j<XF_CHANNELS(TYPE,NPIX);j++)
			{
				color[i].val[j]  = color_info[i][j];
			}
		}

	imgInput.copyTo(in_img.data);

	#if __SDSCC__

	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	boundingbox_accel(imgInput, roi,color,num_box);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif


	out_img.data=imgInput.copyFrom();
	cv::imwrite("hls_out.jpg",out_img);

	cv::absdiff(out_img, in_img1, diff);
	cv::imwrite("diff.jpg",diff); // Save the difference image for debugging purpose

//	 Find minimum and maximum differences.

	double minval = 256, maxval1 = 0;
	int cnt = 0;
	for (int i = 0; i < in_img1.rows; i++)
	{
		for(int j = 0; j < in_img1.cols;j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v > 1)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval1 < v)
				maxval1 = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img1.rows*in_img1.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval1,err_per);


	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
