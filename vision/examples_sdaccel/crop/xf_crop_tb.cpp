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

#include <CL/cl.h>
#include "xcl2.hpp" 

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
#include <sys/types.h>
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

#if GRAY
	/*  reading in the gray image  */
	in_img = cv::imread(argv[1],0);
#else
	in_img = cv::imread(argv[1],1);
	cvtColor(in_img,in_img,cv::COLOR_BGR2RGBA);
#endif

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

//	1ST ROI
	unsigned int x_loc[NUM_ROI];
	unsigned int y_loc[NUM_ROI];
	unsigned int ROI_height[NUM_ROI];
	unsigned int ROI_width[NUM_ROI];

//	2nd ROI
	x_loc[0]=0;
	y_loc[0]=0;
	ROI_height[0]=480;
	ROI_width[0]=640;

	x_loc[1]=0;
	y_loc[1]=0;
	ROI_height[1]=100;
	ROI_width[1]=200;

	x_loc[2]=64;
	y_loc[2]=64;
	ROI_height[2]=300;
	ROI_width[2]=301;


	for(int i=0; i< NUM_ROI; i++)
	{
		out_img[i].create(ROI_height[i],ROI_width[i],in_img.type());
		ocv_ref[i].create(ROI_height[i],ROI_width[i],in_img.type());
		diff[i].create(ROI_height[i],ROI_width[i],in_img.type());

	}

	////////////////  reference code  ////////////////
//		#if __SDSCC__
//
//		perf_counter hw_ctr;
//		hw_ctr.start();
//		#endif
	clock_gettime(CLOCK_MONOTONIC, &start_time);
	cv::Rect ROI(x_loc[0],y_loc[0],ROI_width[0],ROI_height[0]);
	ocv_ref[0]=in_img(ROI);
	cv::Rect ROI1(x_loc[1],y_loc[1],ROI_width[1],ROI_height[1]);
	ocv_ref[1]=in_img(ROI1);
	cv::Rect ROI2(x_loc[2],y_loc[2],ROI_width[2],ROI_height[2]);
	ocv_ref[2]=in_img(ROI2);
	clock_gettime(CLOCK_MONOTONIC, &end_time);
//	float diff_latency = (end_time.tv_nsec - start_time.tv_nsec)/1e9 + end_time.tv_sec - start_time.tv_sec;
//	printf("\latency: %f ", diff_latency);
//		#if __SDSCC__
//		hw_ctr.stop();
//		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
//		#endif

   //////////////////  end opencv reference code//////////

	/////////////////////////////////////// CL ////////////////////////
	int height = in_img.rows;
	int width = in_img.cols;
	
	int *roi = (int*)malloc(NUM_ROI*4*sizeof(int));
	for(int i=0,j=0;i<(NUM_ROI*4);j++,i+=4)
		{
			roi[i]  = x_loc[j];
			roi[i+1]= y_loc[j];
			roi[i+2]= ROI_height[j];
			roi[i+3]= ROI_width[j];

		}

	
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_crop");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"crop_accel");

	
	cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY,(in_img.rows*in_img.cols*INPUT_CH_TYPE));
	cl::Buffer structToDeviceroi(context,CL_MEM_READ_ONLY,(NUM_ROI*4*sizeof(int)));

	cl::Buffer imageFromDeviceroi1(context,CL_MEM_WRITE_ONLY,(ROI_height[0]*ROI_width[0]*OUTPUT_CH_TYPE));
	cl::Buffer imageFromDeviceroi2(context,CL_MEM_WRITE_ONLY,(ROI_height[1]*ROI_width[1]*OUTPUT_CH_TYPE));
	cl::Buffer imageFromDeviceroi3(context,CL_MEM_WRITE_ONLY,(ROI_height[2]*ROI_width[2]*OUTPUT_CH_TYPE));
	
	printf("finished buffer creation task\n");


		/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (in_img.rows*in_img.cols*INPUT_CH_TYPE), in_img.data);/* 0 means from host*/
	q.enqueueWriteBuffer(structToDeviceroi, CL_TRUE, 0, (NUM_ROI*4*sizeof(int)), roi);/* 0 means from host*/
	printf("finished enqueueWriteBuffer task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageFromDeviceroi1);
	krnl.setArg(2, imageFromDeviceroi2);
	krnl.setArg(3, imageFromDeviceroi3);
	krnl.setArg(4, structToDeviceroi);
	krnl.setArg(5, height);
	krnl.setArg(6, width);
	
	printf("finished setting kernel arguments\n");
	
	// Profiling Objects
	cl_ulong start= 0;
	cl_ulong end = 0;
	double diff_prof = 0.0f;
	cl::Event event_sp;
	printf("started kernel execution\n");
	// Launch the kernel 
	q.enqueueTask(krnl,NULL,&event_sp);
	clWaitForEvents(1, (const cl_event*) &event_sp);

	printf("finished kernel execution\n");
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START,&start);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END,&end);
	diff_prof = end-start;
	std::cout<<(diff_prof/1000000)<<"ms"<<std::endl;
	
	q.enqueueReadBuffer(imageFromDeviceroi1, CL_TRUE, 0, (ROI_height[0]*ROI_width[0]*OUTPUT_CH_TYPE), out_img[0].data);
	q.enqueueReadBuffer(imageFromDeviceroi2, CL_TRUE, 0, (ROI_height[1]*ROI_width[1]*OUTPUT_CH_TYPE), out_img[1].data);
	q.enqueueReadBuffer(imageFromDeviceroi3, CL_TRUE, 0, (ROI_height[2]*ROI_width[2]*OUTPUT_CH_TYPE), out_img[2].data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
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
	for (int i = 0; i < ocv_ref[0].rows; i++)
	{
		for(int j = 0; j < ocv_ref[0].cols;j++)
		{
			uchar v = diff[0].at<uchar>(i,j);
			if (v > 1)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval1 < v)
				maxval1 = v;
		}
	}
	float err_per = 100.0*(float)cnt/(ocv_ref[0].rows*ocv_ref[0].cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval1,err_per);


	if(err_per > 0.0f)
	{
		return 1;
	}
	}
	return 0;
}

