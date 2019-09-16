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
#include "xf_scharr_config.h"

#include <CL/cl.h>
#include "xcl2.hpp" 

int main(int argc, char** argv)
{

	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray;
	cv::Mat c_grad_x, c_grad_y;
	cv::Mat hls_grad_x, hls_grad_y;
	cv::Mat diff_grad_x, diff_grad_y;

	// reading in the gray image
#if GRAY
	in_img = cv::imread(argv[1],0);
#else
	in_img = cv::imread(argv[1],1);
#endif

#if GRAY
	#define PTYPE CV_8UC1			// Should be CV_16S when ddepth is CV_16S
#else
	#define PTYPE CV_8UC3		// Should be CV_16S when ddepth is CV_16S
#endif

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}



	typedef unsigned char TYPE; //short int TYPE; //

	// create memory for output images
	c_grad_x.create(in_img.rows,in_img.cols,PTYPE);
	c_grad_y.create(in_img.rows,in_img.cols,PTYPE);
	hls_grad_x.create(in_img.rows,in_img.cols,PTYPE);
	hls_grad_y.create(in_img.rows,in_img.cols,PTYPE);
	diff_grad_x.create(in_img.rows,in_img.cols,PTYPE);
	diff_grad_y.create(in_img.rows,in_img.cols,PTYPE);

	////////////    Opencv Reference    //////////////////////
	int scale = 1;
	int delta = 0;
	int ddepth =-1;//CV_16S;//

	Scharr(in_img, c_grad_x, ddepth, 1, 0, scale, delta, cv::BORDER_CONSTANT );
	Scharr(in_img, c_grad_y, ddepth, 0, 1, scale, delta, cv::BORDER_CONSTANT );

	imwrite("out_ocvx.jpg", c_grad_x);
	imwrite("out_ocvy.jpg", c_grad_y);

	
/////////////////////////////////////// CL ////////////////////////

	int height = in_img.rows;
	int width = in_img.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_scharr");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"scharr_accel");
	
		

	std::vector<cl::Memory> inBufVec, outBufVec1,outBufVec2;
	cl::Buffer imageToDevice(context,CL_MEM_READ_ONLY,(height*width*CH_TYPE));//,(ap_uint<INPUT_PTR_WIDTH>*)in_img.data);
	cl::Buffer imageFromDevice1(context,CL_MEM_WRITE_ONLY,(height*width*CH_TYPE));//,(ap_uint<OUTPUT_PTR_WIDTH>*)hls_grad_x.data);
	cl::Buffer imageFromDevice2(context, CL_MEM_WRITE_ONLY,(height*width*CH_TYPE));//,(ap_uint<OUTPUT_PTR_WIDTH>*)hls_grad_y.data);
	
	
	//inBufVec.push_back(imageToDevice);
	//outBufVec1.push_back(imageFromDevice1);
	//outBufVec2.push_back(imageFromDevice2);
	
	q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (height*width*CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)in_img.data);

	
	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageFromDevice1);
	krnl.setArg(2, imageFromDevice2);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	
	
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

	//q.enqueueMigrateMemObjects(outBufVec1,CL_MIGRATE_MEM_OBJECT_HOST);
	//q.enqueueMigrateMemObjects(outBufVec2,CL_MIGRATE_MEM_OBJECT_HOST);
	q.enqueueReadBuffer(imageFromDevice1, CL_TRUE, 0, (height*width*CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)hls_grad_x.data);
	q.enqueueReadBuffer(imageFromDevice2, CL_TRUE, 0, (height*width*CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)hls_grad_y.data);
	q.finish();
/////////////////////////////////////// end of CL ////////////////////////

	imwrite("out_hlsx.jpg", hls_grad_x);
	imwrite("out_hlsy.jpg", hls_grad_y);


	//////////////////  Compute Absolute Difference ////////////////////

	
	absdiff(c_grad_x, hls_grad_x, diff_grad_x);
	absdiff(c_grad_y, hls_grad_y, diff_grad_y);

	imwrite("out_errorx.jpg", diff_grad_x);
	imwrite("out_errory.jpg", diff_grad_y);

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	double minval1=256,maxval1=0;
	int cnt = 0, cnt1 =0;
	float err_per,err_per1;
	xf::analyzeDiff(diff_grad_x, 0, err_per);
	xf::analyzeDiff(diff_grad_y, 0, err_per1);

	int ret  = 0;
	if(err_per > 0.0f)
	{
		printf("Test failed .... !!!\n");
		ret = 1;
	}else
	{
		printf("Test Passed .... !!!\n");
		ret = 0;
	}

	return ret;

}


