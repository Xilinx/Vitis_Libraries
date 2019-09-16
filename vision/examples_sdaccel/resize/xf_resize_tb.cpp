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
#include "xf_resize_config.h"
	
#include <CL/cl.h>
#include "xcl2.hpp" 

int main(int argc,char **argv)
{
	if(argc != 2){
		printf("Usage : <executable> <input image> \n");
		return -1;
	}
	cv::Mat img,result_hls,result_ocv,error;

#if GRAY
	// reading in the color image
	img = cv::imread(argv[1], 0);
#else
	img = cv::imread(argv[1], 1);
#endif

	if(!img.data){
		return -1;
	}

	cv::imwrite("input.png",img);

	int in_width,in_height;
	int out_width,out_height;

	in_width = img.cols;
	in_height = img.rows;
	out_height = NEWHEIGHT;
	out_width = NEWWIDTH;

	result_hls.create(cv::Size(NEWWIDTH, NEWHEIGHT),img.depth());
	result_ocv.create(cv::Size(NEWWIDTH, NEWHEIGHT),img.depth());
	error.create(cv::Size(NEWWIDTH, NEWHEIGHT),img.depth());

/////////////////////////////////////// CL ///////////////////////////////////////

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_resize");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"resize_accel");

	std::vector<cl::Memory> inBufVec, outBufVec;
	cl::Buffer imageToDevice(context,CL_MEM_READ_ONLY, in_height*in_width);
	cl::Buffer imageFromDevice(context,CL_MEM_WRITE_ONLY,out_height*out_width);

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevice,CL_TRUE, 0,  in_height*in_width, (ap_uint<INPUT_PTR_WIDTH>*)img.data);
	
// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageFromDevice);
	krnl.setArg(2, in_height);
	krnl.setArg(3, in_width);
	krnl.setArg(4, out_height);
	krnl.setArg(5, out_width);
	
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
	q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, out_height*out_width, (ap_uint<OUTPUT_PTR_WIDTH>*)result_hls.data);

	q.finish();
/////////////////////////////////////// end of CL ///////////////////////////////////////
		
	/*OpenCV resize function*/
#if INTERPOLATION==0
	cv::resize(img,result_ocv,cv::Size(NEWWIDTH,NEWHEIGHT),0,0,CV_INTER_NN);
#endif
#if INTERPOLATION==1
	cv::resize(img,result_ocv,cv::Size(NEWWIDTH,NEWHEIGHT),0,0,CV_INTER_LINEAR);
#endif
#if INTERPOLATION==2
	cv::resize(img,result_ocv,cv::Size(NEWWIDTH,NEWHEIGHT),0,0,CV_INTER_AREA);
#endif

	cv::absdiff(result_hls,result_ocv,error);
	float err_per;
	xf::analyzeDiff(error, 10, err_per);

	return 0;
}
