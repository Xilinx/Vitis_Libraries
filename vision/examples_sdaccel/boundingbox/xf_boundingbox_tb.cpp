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

#include <sys/time.h>
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

#include <CL/cl.h>
#include "xcl2.hpp" 
using namespace std;


int main(int argc, char** argv)
{
	cv::Mat in_img,in_img1, out_img,diff;

	struct timespec start_time;
	struct timespec end_time;

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
	cvtColor(in_img,in_img,cv::COLOR_BGR2RGBA);
	in_img1=in_img.clone();
	int num_box=atoi(argv[2]);
#endif

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}
	unsigned int x_loc[MAX_BOXES],y_loc[MAX_BOXES],ROI_height[MAX_BOXES],ROI_width[MAX_BOXES];

/////////////////////////////////////Feeding ROI/////////////////////////////////////////

		x_loc[0]=0;											  // only 3-ROI are feeded, should be modified according to NUM_BOX
		y_loc[0]=0;
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

	
//////////////////////////////////end of Feeding ROI///////////////////////////////////////
#if GRAY
	int color_info[MAX_BOXES][4]={{255,0,0,0},{110,0,0,0},{0,0,0,0},{150,0,0,0},{56,0,0,0}};
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
		clock_gettime(CLOCK_MONOTONIC, &start_time);

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


		clock_gettime(CLOCK_MONOTONIC, &end_time);
		float diff_latency = (end_time.tv_nsec - start_time.tv_nsec)/1e9 + end_time.tv_sec - start_time.tv_sec;
		printf("\latency: %f ", diff_latency);

		cv::imwrite( "ocv_ref.jpg",in_img1);  // reference image

   //////////////////  end opencv reference code//////////
   /////////////////////////////////////// CL //////////////////////////////////////

	  int *roi=( int*)malloc(MAX_BOXES*4*sizeof(int));
//	ap_uint<32> *color=(ap_uint<32>*)malloc(MAX_BOXES*sizeof(ap_uint<32>));

		for(int i=0,j=0;i<(MAX_BOXES*4);j++,i+=4)
			{
				roi[i]  = x_loc[j];
				roi[i+1]= y_loc[j];
				roi[i+2]= ROI_height[j];
				roi[i+3]= ROI_width[j];
			}

/*		for(int i=0;i<(MAX_BOXES);i++)
		{

			for(int j=0,k=0;j<XF_CHANNELS(TYPE,NPIX);j++,k+=XF_DTPIXELDEPTH(TYPE,NPIX))
			{
				color[i].range(k+(XF_DTPIXELDEPTH(TYPE,NPIX)-1),k)  = color_info[i][j];
			}
		}*/
	int height = in_img.rows;
	int width = in_img.cols;
	
	
	
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_boundingbox");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"boundingbox_accel");

	std::vector<cl::Memory> inBuf,inbuf_roi,inbuf_color,outBuf;

	cl::Buffer imageToDevice(context,CL_MEM_READ_WRITE,(in_img.rows*in_img.cols*INPUT_CH_TYPE));
	cl::Buffer structToDeviceroi(context, CL_MEM_READ_ONLY,(MAX_BOXES*4*sizeof(int)));
	cl::Buffer structToDevicecolor(context, CL_MEM_READ_ONLY,(MAX_BOXES*4*sizeof(int)));
	
	
	printf("finished buffer creation task\n");

	inBuf.push_back(imageToDevice);
	inbuf_roi.push_back(structToDeviceroi);
//	outBuf.push_back(imageFromDevice);

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (in_img.rows*in_img.cols*INPUT_CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)in_img.data);/* 0 means from host*/
	q.enqueueWriteBuffer(structToDeviceroi, CL_TRUE, 0, MAX_BOXES*4*sizeof(int), roi);/* 0 means from host*/
	q.enqueueWriteBuffer(structToDevicecolor, CL_TRUE, 0, (MAX_BOXES*4*sizeof(int)), color_info);/* 0 means from host*/
	

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, structToDeviceroi);
	krnl.setArg(2, structToDevicecolor);
	krnl.setArg(3, height);

	krnl.setArg(4, width);
	krnl.setArg(5, num_box);
	
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
	
	q.enqueueReadBuffer(imageToDevice, CL_TRUE, 0, (in_img.rows*in_img.cols*OUTPUT_CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)in_img.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////



	cv::imwrite("hls_out.jpg",in_img);

	cv::absdiff(in_img, in_img1, diff);
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

