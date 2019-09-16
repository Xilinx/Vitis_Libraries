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
#include "xf_gaussian_filter_config.h"
#include <iostream>

#include <CL/cl.h>
#include "xcl2.hpp" 


using namespace std;


int main(int argc, char **argv) {

	if (argc != 2)
	{
		printf("Usage: <executable> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, out_img, ocv_ref, in_img_gau;
	cv::Mat in_gray, in_gray1, diff;


#if GRAY
	in_img = cv::imread(argv[1], 0); // reading in the color image
#else
	in_img = cv::imread(argv[1], 1); // reading in the color image
#endif
	if (!in_img.data) {
		printf("Failed to load the image ... !!!");
		return -1;
	}
	//extractChannel(in_img, in_img, 1);
#if GRAY

	out_img.create(in_img.rows, in_img.cols, CV_8UC1); // create memory for output image
	diff.create(in_img.rows, in_img.cols, CV_8UC1); // create memory for OCV-ref image
	ocv_ref.create(in_img.rows, in_img.cols, CV_8UC1); // create memory for OCV-ref image
	
#else
	out_img.create(in_img.rows, in_img.cols, CV_8UC3); // create memory for output image
	diff.create(in_img.rows, in_img.cols, CV_8UC3); // create memory for OCV-ref image
	ocv_ref.create(in_img.rows, in_img.cols, CV_8UC3); // create memory for OCV-ref image
#endif

#if FILTER_WIDTH==3
	float sigma = 0.5f;
#endif
#if FILTER_WIDTH==7
	float sigma=1.16666f;
#endif
#if FILTER_WIDTH==5
	float sigma = 0.8333f;
#endif


	// OpenCV Gaussian filter function
	cv::GaussianBlur(in_img, ocv_ref, cvSize(FILTER_WIDTH, FILTER_WIDTH),FILTER_WIDTH / 6.0, FILTER_WIDTH / 6.0, cv::BORDER_CONSTANT);

	imwrite("output_ocv.png", ocv_ref);



/////////////////////////////////////// CL ////////////////////////

	int height = in_img.rows;
	int width = in_img.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_gaussian_filter");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"gaussian_filter_accel");
	
		

	std::vector<cl::Memory> inBufVec, outBufVec;
	cl::Buffer imageToDevice(context,CL_MEM_READ_ONLY,(height*width*CH_TYPE));//,(ap_uint<INPUT_PTR_WIDTH>*)in_img.data);
	cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY,(height*width*CH_TYPE));//,(ap_uint<OUTPUT_PTR_WIDTH>*)out_img.data);
	
	q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (height*width*CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)in_img.data);
	
	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageFromDevice);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	krnl.setArg(4, sigma);
	
	// Profiling Objects
	cl_ulong start= 0;
	cl_ulong end = 0;
	double diff_prof = 0.0f;
	cl::Event event_sp;

	
	// Launch the kernel 
	q.enqueueTask(krnl,NULL,&event_sp);
	clWaitForEvents(1, (const cl_event*) &event_sp);

	

	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START,&start);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END,&end);
	diff_prof = end-start;
	std::cout<<(diff_prof/1000000)<<"ms"<<std::endl;

	//Copying Device result data to Host memory
	q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, (height*width*CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)out_img.data);
	q.finish();
/////////////////////////////////////// end of CL ////////////////////////

	

	cv::imwrite("hw_out.jpg",out_img);

	//////////////////  Compute Absolute Difference ////////////////////
	cv::absdiff(ocv_ref,out_img,diff);
	cv::imwrite("out_error.jpg",diff);



	float err_per;

	xf::analyzeDiff(diff, 0, err_per);

	
	if(err_per > 1){
		printf("\nTest failed\n");
		return -1;
	}
	else{
		printf("\nTest Pass\n");
	return 0;
	}

}
