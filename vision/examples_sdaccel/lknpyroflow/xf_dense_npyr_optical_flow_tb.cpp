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

#include "xf_dense_npyr_optical_flow_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "ap_int.h"
#include "hls_stream.h"



#define HLS 0

#if !HLS
#include <CL/cl.h>
#include "xcl2.hpp"
#endif

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
//#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <iostream>
#include <string>

void dense_non_pyr_of_accel(ap_uint<INPUT_PTR_WIDTH> *img_curr, ap_uint<INPUT_PTR_WIDTH> *img_prev, float *img_outx, float *img_outy, int cols, int rows);




int main(int argc, char** argv)
{
	cv::Mat frame0, frame1;
	#if !HLS
	cv::Mat flowx, flowy;
	#endif
	cv::Mat frame_out;

	if (argc != 3) {
		std::cout << "Usage incorrect. Correct usage: ./exe <current frame> <next frame>" << std::endl;
		return -1;
	}

	frame0 = cv::imread(argv[1],0);
	frame1 = cv::imread(argv[2],0);
	
	if (frame0.empty() || frame1.empty()) {
		std::cout << "input files not found!" << std::endl;
		return -1;
	}
	
	frame_out.create(frame0.rows, frame0.cols, CV_8UC4);
	#if !HLS
	flowx.create(frame0.rows, frame0.cols, CV_32FC1);
	flowy.create(frame0.rows, frame0.cols, CV_32FC1);
	#endif

	int cnt = 0;
	unsigned char p1, p2, p3, p4;
	unsigned int pix =0;

	char out_string[200];
	
	#if HLS
	static xf::Mat<XF_32FC1,MAX_HEIGHT, MAX_WIDTH, OF_PIX_PER_CLOCK> flowx(frame0.rows,frame0.cols);
	static xf::Mat<XF_32FC1,MAX_HEIGHT, MAX_WIDTH, OF_PIX_PER_CLOCK> flowy(frame0.rows,frame0.cols);
	
#endif
/////////////////////////////////////// CL ////////////////////////

	int height = frame0.rows;
	int width = frame0.cols;
	#if HLS
	dense_non_pyr_of_accel((ap_uint<INPUT_PTR_WIDTH>*)frame0.data,(ap_uint<INPUT_PTR_WIDTH>*)frame1.data,(float *)flowx.data,(float *)flowy.data,height,width);
	#endif
	#if !HLS
	
	std::cout<<"Starting xrt programmingms"<<std::endl;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);

	std::cout<<"device context created"<<std::endl;

	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::cout<<"command queue created"<<std::endl;

	//Create Program and Kernel
	std::string device_name = device.getInfo<CL_DEVICE_NAME>();
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_lknpyrof");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"dense_non_pyr_of_accel");

	std::cout<<"kernel loaded"<<std::endl;
	


	//Allocate Buffer in Global Memory
	cl::Buffer currImageToDevice(context, CL_MEM_READ_ONLY,(height*width));
	cl::Buffer prevImageToDevice(context, CL_MEM_READ_ONLY,(height*width));

	std::cout<<"input buffer created"<<std::endl;

	cl::Buffer outxImageFromDevice(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,(height*width*OUT_BYTES_PER_CHANNEL),flowx.data);
	cl::Buffer outyImageFromDevice(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,(height*width*OUT_BYTES_PER_CHANNEL),flowy.data);
	
	std::vector<cl::Memory> outBufVec0, outBufVec1;
	outBufVec0.push_back(outxImageFromDevice);
	outBufVec1.push_back(outyImageFromDevice);

	std::cout<<"output buffer created"<<std::endl;

	//Copying input data to Device buffer from host memory
	q.enqueueWriteBuffer(currImageToDevice, CL_TRUE, 0, (height*width), frame0.data);
	q.enqueueWriteBuffer(prevImageToDevice, CL_TRUE, 0, (height*width), frame1.data);

	std::cout<<"input buffer copied"<<std::endl;

	krnl.setArg(0, currImageToDevice);
	krnl.setArg(1, prevImageToDevice);
	krnl.setArg(2, outxImageFromDevice);
	krnl.setArg(3, outyImageFromDevice);
	krnl.setArg(4, height);
	krnl.setArg(5, width);

	std::cout<<"arguments copied"<<std::endl;

	// Profiling Objects
	cl_ulong start= 0;
	cl_ulong end = 0;
	double diff_prof = 0.0f;
	cl::Event event_sp;

	// Launch the kernel
	q.enqueueTask(krnl,NULL,&event_sp);
	clWaitForEvents(1, (const cl_event*) &event_sp);

	// Profiling
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START,&start);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END,&end);
	diff_prof = end-start;
	std::cout<<(diff_prof/1000000)<<"ms"<<std::endl;

	std::cout<<"ip returned"<<std::endl;
	//Copying Device result data to Host memory
	q.enqueueMigrateMemObjects(outBufVec0,CL_MIGRATE_MEM_OBJECT_HOST);
	std::cout<<"output x  buffer read"<<std::endl;
	q.enqueueMigrateMemObjects(outBufVec1,CL_MIGRATE_MEM_OBJECT_HOST);
	std::cout<<"output y  buffer read"<<std::endl;

	std::cout<<"done"<<std::endl;
	q.finish();
	
	#endif
/////////////////////////////////////// end of CL ////////////////////////
	

	float *flowx_copy ;
	float *flowy_copy ;
#if 1
	flowx_copy = (float *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(float)));
	if(flowx_copy==NULL){
		fprintf(stderr,"\nFailed to allocate memory for flowx_copy\n");
	}
	flowy_copy = (float *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(float)));
	if(flowy_copy==NULL){
		fprintf(stderr,"\nFailed to allocate memory for flowy_copy\n");
	}

#if !HLS
	int size = height*width; 
	for (int f = 0; f < height; f++) {
		for ( int i=0; i<width; i++) {
		flowx_copy[f*width+i] = flowx.at<float>(f,i);
		flowy_copy[f*width+i] = flowy.at<float>(f,i);
	}
	}
	#endif
#endif

	unsigned int *outputBuffer;
	outputBuffer = (unsigned int *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(unsigned int)));
	if(outputBuffer==NULL){
		fprintf(stderr,"\nFailed to allocate memory for outputBuffer\n");
	}

		fprintf(stderr,"\ncopied to clow_copy\n");

	hls::stream <rgba_t> out_pix("Color pixel");
	
	xf::getOutPix<MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(flowx_copy,flowy_copy,frame1.data,out_pix,frame0.rows,frame0.cols,frame0.cols*frame0.rows);
		fprintf(stderr,"\ncopied to clow_copy\n");
	xf::writeMatRowsRGBA<MAX_HEIGHT, MAX_WIDTH, XF_NPPC1, KMED>(out_pix, outputBuffer,frame0.rows,frame0.cols,frame0.cols*frame0.rows);


	rgba_t *outbuf_copy;
	for(int i=0;i<frame0.rows;i++){
		for(int j=0;j<frame0.cols;j++){
			outbuf_copy = (rgba_t *) (outputBuffer + i*(frame0.cols) + j);
			p1 = outbuf_copy->r;
			p2 = outbuf_copy->g;
			p3 = outbuf_copy->b;
			p4 = outbuf_copy->a;
			pix = ((unsigned int)p4 << 24) | ((unsigned int)p3 << 16) | ((unsigned int)p2 << 8) | (unsigned int)p1 ;
			frame_out.at<unsigned int>(i,j) = pix;
		}
	}

	sprintf(out_string,"out_%d.png", cnt);
	cv::imwrite(out_string,frame_out);
	return 0;
}

