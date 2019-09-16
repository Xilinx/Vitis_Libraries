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
#include "xf_crop_config.h"

using namespace std;


int main(int argc, char** argv)
{
	struct timespec start_time;
	struct timespec end_time;
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, out_img[NUM_ROI], ocv_ref[NUM_ROI], in_gray[NUM_ROI], diff[NUM_ROI], out_img1,in_gray1, diff1, ocv_ref1;

	/*  reading in the input image  */
#if GRAY
	in_img = cv::imread(argv[1],0);
#else
	in_img = cv::imread(argv[1],1);
#endif

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}


	unsigned int x_loc[NUM_ROI];
	unsigned int y_loc[NUM_ROI];
	unsigned int ROI_height[NUM_ROI];
	unsigned int ROI_width[NUM_ROI];

	//	1ST ROI
	x_loc[0]=0;
	y_loc[0]=0;
	ROI_height[0]=480;
	ROI_width[0]=640;
	//	2nd ROI
	x_loc[1]=50;
	y_loc[1]=50;
	ROI_height[1]=100;
	ROI_width[1]=200;

	x_loc[2]=48;
	y_loc[2]=48;
	ROI_height[2]=300;
	ROI_width[2]=301;


	for(int i=0; i< NUM_ROI; i++)
	{
#if GRAY
		out_img[i].create(ROI_height[i],ROI_width[i],in_img.depth());
		ocv_ref[i].create(ROI_height[i],ROI_width[i],in_img.depth());
		diff[i].create(ROI_height[i],ROI_width[i],in_img.depth());
#else
		out_img[i].create(ROI_height[i],ROI_width[i],CV_8UC3);
		ocv_ref[i].create(ROI_height[i],ROI_width[i],CV_8UC3);
		diff[i].create(ROI_height[i],ROI_width[i],CV_8UC3);
#endif

	}

	////////////////  reference code  ////////////////

	cv::Rect ROI(x_loc[0],y_loc[0],ROI_width[0],ROI_height[0]);
	ocv_ref[0]=in_img(ROI);
	cv::Rect ROI1(x_loc[1],y_loc[1],ROI_width[1],ROI_height[1]);
	ocv_ref[1]=in_img(ROI1);
	cv::Rect ROI2(x_loc[2],y_loc[2],ROI_width[2],ROI_height[2]);
	ocv_ref[2]=in_img(ROI2);
   //////////////////  end opencv reference code//////////

	////////////////////// HLS TOP function call ////////////////////////////

		 xf::Mat<TYPE, HEIGHT, WIDTH, NPC> imgInput(in_img.rows,in_img.cols);
		 xf::Mat<TYPE, HEIGHT, WIDTH, NPC> imgOutput[NUM_ROI];//(ROI_height,ROI_width);

		xf::Rect_ <unsigned int> roi[NUM_ROI];

	for(int i=0;i<NUM_ROI;i++)
		{
			roi[i].height=ROI_height[i];
			roi[i].width=ROI_width[i];
			roi[i].x=x_loc[i];
			roi[i].y=y_loc[i];
		}

	for(int i=0;i<NUM_ROI;i++)
	{
		imgOutput[i].rows=roi[i].height;
		imgOutput[i].cols=roi[i].width;
	}

	imgInput.copyTo(in_img.data);

	#if __SDSCC__

	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	crop_accel(imgInput, imgOutput, roi);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif


	for(int i=0;i<NUM_ROI;i++)
	{
		out_img[i].data=imgOutput[i].copyFrom();
	}
	FILE *fp=fopen("data.txt","w");
		for(int i=0;i<(ROI_height[2]);i++)
		{
			for(int j=0;j<ROI_width[2];j++)
			{
				uchar ocv_val=ocv_ref[2].at<unsigned char>(i,j);
				uchar hls_val=out_img[2].at<unsigned char>(i,j);
				fprintf(fp,"%d %d\n",ocv_val,hls_val);

			}


		}
	fclose(fp);

	 char hls_strg[30];
	 char ocv_strg[30];
	 char diff_strg[30];

	// Write output image
	 for(int i=0;i<NUM_ROI;i++)
	 {
		 sprintf(hls_strg,"out_img[%d].jpg",i);
		 sprintf(ocv_strg,"ocv_ref[%d].jpg",i);
		 sprintf(diff_strg,"diff_img[%d].jpg",i);
		 cv::imwrite(hls_strg, out_img[i]);  // hls image
		 cv::imwrite(ocv_strg, ocv_ref[i]);  // reference image
		 cv::absdiff(ocv_ref[i], out_img[i], diff[i]);
		 cv::imwrite(diff_strg,diff[i]); // Save the difference image for debugging purpose
	 }

//	 Find minimum and maximum differences.
	for(int roi=0;roi<NUM_ROI;roi++)
	{
		double minval = 256, maxval1 = 0;
		int cnt = 0;
		for (int i = 0; i < ocv_ref[roi].rows; i++)
		{
			for(int j = 0; j < ocv_ref[roi].cols;j++)
			{
				uchar v = diff[roi].at<uchar>(i,j);
				if (v > 1)
					cnt++;
				if (minval > v )
					minval = v;
				if (maxval1 < v)
					maxval1 = v;
			}
		}
		float err_per = 100.0*(float)cnt/(ocv_ref[roi].rows*ocv_ref[roi].cols);
		fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval1,err_per);


		if(err_per > 0.0f)
		{
			return 1;
		}
	}
	return 0;
}

