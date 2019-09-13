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
#include "xf_cvt_color_config.h"

#include <CL/cl.h>
#include "xcl2.hpp" 

#define ERROR_THRESHOLD 2

int main(int argc, char** argv) {

	uint16_t img_width;
	uint16_t img_height;

	cv::Mat inputimg0, inputimg1, inputimg2, inputimg;
	cv::Mat outputimg0, outputimg1, outputimg2;
	cv::Mat error_img0, error_img1, error_img2;
	cv::Mat refimage, refimg0, refimg1, refimg2;
	cv::Mat refimage0, refimage1, refimage2;

	cv::Mat img;

	
#if __SDSCC__
	perf_counter hw_ctr;
#endif
#if IYUV2NV12

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg1 = cv::imread(argv[2], 0);
	if(!inputimg1.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg2 = cv::imread(argv[3], 0);
	if(!inputimg2.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_uv = inputimg1.cols/2;
	int newheight_uv = inputimg1.rows+inputimg2.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0, CV_8UC1);
	error_img0.create(S0, CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1, CV_16UC1);
	error_img1.create(S1, CV_16UC1);

/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;
	int height_u = inputimg1.rows;
	int width_u = inputimg1.cols;
	int height_v = inputimg2.rows;
	int width_v = inputimg2.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_iyuv2nv12");
printf("started buffer creation task\n");
	
	std::vector<cl::Memory> inBuf_y,inBuf_u,inBuf_v, outBuf_y, outBuf_uv;
	cl::Buffer imageToDeviceY(context, CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceU(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols));
	cl::Buffer imageToDeviceV(context, CL_MEM_READ_ONLY,(inputimg2.rows*inputimg2.cols));
	cl::Buffer imageFromDeviceY(context, CL_MEM_WRITE_ONLY,(newheight_y*newwidth_y));
	cl::Buffer imageFromDeviceUV(context,CL_MEM_WRITE_ONLY,(newheight_uv*newwidth_uv*2));
printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDeviceY, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceU, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);
	q.enqueueWriteBuffer(imageToDeviceV, CL_TRUE, 0, (inputimg2.rows*inputimg2.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg2.data);

printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceY);
	krnl.setArg(1, imageToDeviceU);
	krnl.setArg(2, imageToDeviceV);
	krnl.setArg(3, imageFromDeviceY);
	krnl.setArg(4, imageFromDeviceUV);
	
	krnl.setArg(5, height);
	krnl.setArg(6, width);
	krnl.setArg(7, height_u);
	krnl.setArg(8, width_u);
	krnl.setArg(9, height_v);
	krnl.setArg(10, width_v);
	krnl.setArg(11, newheight_uv);
	krnl.setArg(12, newwidth_uv);

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
	q.enqueueReadBuffer(imageFromDeviceY, CL_TRUE, 0, (newheight_y*newwidth_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceUV, CL_TRUE, 0, (newheight_uv*newwidth_uv*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL ////////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_UV.png",outputimg1);

	refimage0 = cv::imread(argv[4],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[5],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);
	cv::imwrite("y_error.png",error_img0);
	cv::imwrite("UV_error.png",error_img1);
#endif
#if IYUV2RGBA

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg1 = cv::imread(argv[2], 0);
	if(!inputimg1.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg2 = cv::imread(argv[3], 0);
	if(!inputimg2.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}

	cv::Size S0(inputimg0.cols,inputimg0.rows);
	outputimg0.create(S0, CV_8UC4);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows + inputimg2.rows;

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S(newwidth,newheight);
	outputimg1.create(S,CV_8UC3);

	//outputimg_ocv.create(S,CV_8UC4);
	error_img0.create(S,CV_8UC3);

		/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;
	int height_u = inputimg1.rows;
	int width_u = inputimg1.cols;
	int height_v = inputimg2.rows;
	int width_v = inputimg2.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_iyuv2rgba");

	
	
	std::vector<cl::Memory> inBuf_y,inBuf_u,inBuf_v, outBuf_rgba;
	cl::Buffer imageToDeviceY(context, CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceU(context, CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols));
	cl::Buffer imageToDeviceV(context, CL_MEM_READ_ONLY,(inputimg2.rows*inputimg2.cols));
	cl::Buffer imageFromDevicergba(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*4));


	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDeviceY, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceU, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);
	q.enqueueWriteBuffer(imageToDeviceV, CL_TRUE, 0, (inputimg2.rows*inputimg2.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg2.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceY);
	krnl.setArg(1, imageToDeviceU);
	krnl.setArg(2, imageToDeviceV);
	krnl.setArg(3, imageFromDevicergba);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	krnl.setArg(6, height_u);
	krnl.setArg(7, width_u);
	krnl.setArg(8, height_v);
	krnl.setArg(9, width_v);


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
	
	q.enqueueReadBuffer(imageFromDevicergba, CL_TRUE, 0, (newwidth*newheight*4),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL ////////////////////////

	cvtColor(outputimg0,outputimg1,CV_RGBA2BGR);
	cv::imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[4],1);

	absdiff(outputimg1,refimage,error_img0);


#endif
#if IYUV2RGB

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg1 = cv::imread(argv[2], 0);
	if(!inputimg1.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg2 = cv::imread(argv[3], 0);
	if(!inputimg2.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}

	cv::Size S0(inputimg0.cols,inputimg0.rows);
	outputimg0.create(S0, CV_8UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows + inputimg2.rows;

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S(newwidth,newheight);
	outputimg0.create(S,CV_8UC3);

	//outputimg_ocv.create(S,CV_8UC4);
	error_img0.create(S,CV_8UC3);
	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;
	int height_u = inputimg1.rows;
	int width_u = inputimg1.cols;
	int height_v = inputimg2.rows;
	int width_v = inputimg2.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_iyuv2rgb");

	
	
	std::vector<cl::Memory> inBuf_y,inBuf_u,inBuf_v, outBuf_rgb;
	cl::Buffer imageToDeviceY(context, CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceU(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols));
	cl::Buffer imageToDeviceV(context, CL_MEM_READ_ONLY,(inputimg2.rows*inputimg2.cols));
	cl::Buffer imageFromDevicergb(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceY, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceU, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);
	q.enqueueWriteBuffer(imageToDeviceV, CL_TRUE, 0, (inputimg2.rows*inputimg2.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg2.data);

printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceY);
	krnl.setArg(1, imageToDeviceU);
	krnl.setArg(2, imageToDeviceV);
	krnl.setArg(3, imageFromDevicergb);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	krnl.setArg(6, height_u);
	krnl.setArg(7, width_u);
	krnl.setArg(8, height_v);
	krnl.setArg(9, width_v);


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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (newwidth*newheight*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL ////////////////////////
	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[4],1);

	absdiff(outputimg0,refimage,error_img0);
	cv::imwrite("diff.jpg",error_img0);

#endif
#if IYUV2YUV4

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg1 = cv::imread(argv[2], 0);
	if(!inputimg1.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg2 = cv::imread(argv[3], 0);
	if(!inputimg2.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_u = inputimg1.cols;
	int newheight_u = inputimg1.rows<<2;
	int newwidth_v = inputimg2.cols;
	int newheight_v = inputimg2.rows<<2;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u,newheight_u);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);

	cv::Size S2(newwidth_v,newheight_v);
	outputimg2.create(S2,CV_8UC1);
	error_img2.create(S2,CV_8UC1);
	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;
	int height_u = inputimg1.rows;
	int width_u = inputimg1.cols;
	int height_v = inputimg2.rows;
	int width_v = inputimg2.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_iyuv2yuv4");

	
	
	std::vector<cl::Memory> inBuf_y,inBuf_u,inBuf_v, outBuf_y, outBuf_u, outBuf_v;
	cl::Buffer imageToDeviceY(context,CL_MEM_READ_ONLY ,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceU(context,CL_MEM_READ_ONLY ,(inputimg1.rows*inputimg1.cols));
	cl::Buffer imageToDeviceV(context,CL_MEM_READ_ONLY ,(inputimg2.rows*inputimg2.cols));
	cl::Buffer imageFromDeviceY(context,CL_MEM_READ_ONLY ,(newheight_y*newwidth_y));
	cl::Buffer imageFromDeviceU(context,CL_MEM_READ_ONLY ,(newheight_u*newwidth_u));
	cl::Buffer imageFromDevicev(context,CL_MEM_READ_ONLY ,(newheight_v*newwidth_v));

printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceY, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceU, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);
	q.enqueueWriteBuffer(imageToDeviceV, CL_TRUE, 0, (inputimg2.rows*inputimg2.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg2.data);

printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceY);
	krnl.setArg(1, imageToDeviceU);
	krnl.setArg(2, imageToDeviceV);
	krnl.setArg(3, imageFromDeviceY);
	krnl.setArg(4, imageFromDeviceU);
	krnl.setArg(5, imageFromDevicev);
	krnl.setArg(6, height);
	krnl.setArg(7, width);
	krnl.setArg(8, height_u);
	krnl.setArg(9, width_u);
	krnl.setArg(10, height_v);
	krnl.setArg(11, width_v);


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
	
	q.enqueueReadBuffer(imageFromDeviceY, CL_TRUE, 0, (newheight_y*newwidth_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceU, CL_TRUE, 0, (newheight_u*newwidth_u),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newheight_v*newwidth_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL ////////////////////////

	refimage0 = cv::imread(argv[4],0);
	if(!refimage0.data)
	{
		printf("unable to open image \n");
		return(1);
	}
	refimage1 = cv::imread(argv[5],0);
	if(!refimage1.data)
	{
		printf("unable to open image \n");
		return(1);
	}
	refimage2 = cv::imread(argv[6],0);
	if(!refimage2.data)
	{
		printf("unable to open image \n");
		return(1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);

	imwrite("error_u.png",error_img1);
	imwrite("error_V.png",error_img2);

#endif
#if NV122IYUV

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_u_v = inputimg1.cols<<1;
	int newheight_u_v = inputimg1.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);

	outputimg2.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);


/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv122iyuv");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDevicey(context, CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));
	cl::Buffer imageFromDevicev(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));

	printf("finished buffer creation task\n");


	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicey);
	krnl.setArg(3, imageFromDeviceu);
	krnl.setArg(4, imageFromDevicev);
	krnl.setArg(5, height_y);
	krnl.setArg(6, width_y);
	krnl.setArg(7, height_u_y);
	krnl.setArg(8, width_u_y);
	krnl.setArg(9, newheight_y);
	krnl.setArg(10, newwidth_y);
	krnl.setArg(11, newheight_u_v);
	krnl.setArg(12, newwidth_u_v);
	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_U.png",outputimg1);
	cv::imwrite("out_V.png",outputimg2);

	refimg0 = cv::imread(argv[3],0);
	if(!refimg0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimg1 = cv::imread(argv[4],0);
	if(!refimg1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimg2 = cv::imread(argv[5],0);
	if(!refimg2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimg0, outputimg0, error_img0);
	cv::absdiff(refimg1, outputimg1, error_img1);
	cv::absdiff(refimg2, outputimg2, error_img2);


	imwrite("error_Y.png", error_img0);
	imwrite("error_U.png", error_img1);
	imwrite("error_V.png", error_img2);
#endif

#if NV122RGBA

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC4);
	error_img0.create(S0,CV_8UC4);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv122rgba");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgba;
	cl::Buffer imageToDevicey(context, CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicergba(context,CL_MEM_WRITE_ONLY,(newwidth*newheight*4));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicergba);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicergba, CL_TRUE, 0, (newwidth*newheight*4),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////


	cvtColor(outputimg0,outputimg0,CV_RGBA2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg0,refimage,error_img0);

#endif
#if NV122RGB

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);
	error_img0.create(S0,CV_8UC3);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv122rgb");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgb;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicergb(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*3));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicergb);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (newwidth*newheight*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);
#endif

#if NV122BGR

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);
	error_img0.create(S0,CV_8UC3);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv122bgr");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgb;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicergb(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*3));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicergb);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (newwidth*newheight*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);
#endif
#if NV212BGR

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);
	error_img0.create(S0,CV_8UC3);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv212bgr");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgb;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicergb(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*3));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicergb);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (newwidth*newheight*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);
#endif

#if NV122UYVY

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv122uyvy");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgb;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDeviceuyvy(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*2));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDeviceuyvy);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDeviceuyvy, CL_TRUE, 0, (newwidth*newheight*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	imwrite("out_uyvy.png", outputimg0);

	refimage = cv::imread(argv[3],-1);

	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);

#endif

#if NV122NV21

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;
	int newwidth_uv = inputimg1.cols;
	int newheight_uv = inputimg1.rows;

	cv::Size S0(newwidth,newheight);
	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg0.create(S0,CV_8UC1);
	outputimg1.create(S1,CV_16UC1);
	error_img0.create(S0,CV_8UC1);
	error_img1.create(S0,CV_16UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);

	cl::Kernel krnl(program,"cvtcolor_nv122nv21");

	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_y,out_uv;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicey(context, CL_MEM_WRITE_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(inputimg1.rows*inputimg1.cols*2));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicey);
	krnl.setArg(3, imageFromDeviceuv);
	krnl.setArg(4, height_y);
	krnl.setArg(5, width_y);
	krnl.setArg(6, height_u_y);
	krnl.setArg(7, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],-1);
	if(!refimage0.data) {
		printf("Failed to open vu reference image\n");
		return (1);
	}
	absdiff(refimage1, outputimg1, error_img1);
	absdiff(refimage0, outputimg0, error_img0);
	cv::imwrite("error_img0.png",error_img0);
	cv::imwrite("error_img1.png",error_img1);
#endif
#if NV212NV12

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;
	int newwidth_uv = inputimg1.cols;
	int newheight_uv = inputimg1.rows;

	cv::Size S0(newwidth,newheight);
	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg0.create(S0,CV_8UC1);
	outputimg1.create(S1,CV_16UC1);
	error_img0.create(S0,CV_8UC1);
	error_img1.create(S0,CV_16UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);

	cl::Kernel krnl(program,"cvtcolor_nv212nv12");

	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_y,out_uv;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicey(context, CL_MEM_WRITE_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(inputimg1.rows*inputimg1.cols*2));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicey);
	krnl.setArg(3, imageFromDeviceuv);
	krnl.setArg(4, height_y);
	krnl.setArg(5, width_y);
	krnl.setArg(6, height_u_y);
	krnl.setArg(7, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],-1);
	if(!refimage0.data) {
		printf("Failed to open vu reference image\n");
		return (1);
	}
	absdiff(refimage1, outputimg1, error_img1);
	absdiff(refimage0, outputimg0, error_img0);
	cv::imwrite("error_img0.png",error_img0);
	cv::imwrite("error_img1.png",error_img1);
#endif
#if NV122YUYV

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv122yuyv");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgb;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDeviceyuyv(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*2));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDeviceyuyv);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDeviceyuyv, CL_TRUE, 0, (newwidth*newheight*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	imwrite("out_yuyv.png", outputimg0);

	refimage = cv::imread(argv[3],-1);

	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);

#endif
#if NV212UYVY

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv212uyvy");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgb;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDeviceuyvy(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*2));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDeviceuyvy);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDeviceuyvy, CL_TRUE, 0, (newwidth*newheight*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	imwrite("out_uyvy.png", outputimg0);

	refimage = cv::imread(argv[3],-1);

	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);

#endif
#if NV212YUYV

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv212yuyv");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgb;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDeviceyuyv(context, CL_MEM_WRITE_ONLY,(newwidth*newheight*2));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDeviceyuyv);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDeviceyuyv, CL_TRUE, 0, (newwidth*newheight*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	imwrite("out_yuyv.png", outputimg0);

	refimage = cv::imread(argv[3],-1);

	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);

#endif
#if NV122YUV4

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_u_v = inputimg1.cols<<1;
	int newheight_u_v = inputimg1.rows<<1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);

	outputimg2.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows;

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv122yuv4");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDevicey(context, CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicey(context, CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context, CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));
	cl::Buffer imageFromDevicev(context, CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicey);
	krnl.setArg(3, imageFromDeviceu);
	krnl.setArg(4, imageFromDevicev);
	krnl.setArg(5, height_y);
	krnl.setArg(6, width_y);
	krnl.setArg(7, height_u_y);
	krnl.setArg(8, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_U.png",outputimg1);
	cv::imwrite("out_V.png",outputimg2);

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[5],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}


	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);

#endif
#if NV212IYUV

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_u_v = inputimg1.cols<<1;
	int newheight_u_v = inputimg1.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);

	outputimg2.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv212iyuv");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));
	cl::Buffer imageFromDevicev(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicey);
	krnl.setArg(3, imageFromDeviceu);
	krnl.setArg(4, imageFromDevicev);
	krnl.setArg(5, height_y);
	krnl.setArg(6, width_y);
	krnl.setArg(7, height_u_y);
	krnl.setArg(8, width_u_y);
	krnl.setArg(9, newheight_y);
	krnl.setArg(10, newwidth_y);
	krnl.setArg(11, newheight_u_v);
	krnl.setArg(12, newwidth_u_v);
	
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
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_U.png",outputimg1);
	cv::imwrite("out_V.png",outputimg2);

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[5],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);

#endif

#if NV212RGBA

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC4);
	outputimg1.create(S0,CV_8UC3);

/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv212rgba");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgba;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicergba(context,CL_MEM_WRITE_ONLY,(newwidth*newheight*4));


	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicergba);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicergba, CL_TRUE, 0, (newwidth*newheight*4),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cvtColor(outputimg0,outputimg1,CV_RGBA2BGR);
	imwrite("out.png", outputimg1);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg1,refimage,error_img0);

#endif
#if NV212RGB

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);
/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv212rgb");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_rgb;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicergb(context,CL_MEM_WRITE_ONLY,(newwidth*newheight*OUTPUT_CH_TYPE));


	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicergb);
	krnl.setArg(3, height_y);
	krnl.setArg(4, width_y);
	krnl.setArg(5, height_u_y);
	krnl.setArg(6, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (newwidth*newheight*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg0,refimage,error_img0);

#endif

#if NV212YUV4

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_u_v = inputimg1.cols<<1;
	int newheight_u_v = inputimg1.rows<<1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	/////////////////////////////////////// CL ////////////////////////
	int height_y = inputimg0.rows;
	int width_y = inputimg0.cols;
	int height_u_y = inputimg1.rows;
	int width_u_y = inputimg1.cols;
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_nv212yuv4");

	
	std::vector<cl::Memory> inBuf_y,inBuf_uv,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDevicey(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols));
	cl::Buffer imageToDeviceuv(context,CL_MEM_READ_ONLY,(inputimg1.rows*inputimg1.cols*2));

	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));
	cl::Buffer imageFromDevicev(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicey, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);
	q.enqueueWriteBuffer(imageToDeviceuv, CL_TRUE, 0, (inputimg1.rows*inputimg1.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg1.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicey);
	krnl.setArg(1, imageToDeviceuv);
	krnl.setArg(2, imageFromDevicey);
	krnl.setArg(3, imageFromDeviceu);
	krnl.setArg(4, imageFromDevicev);
	krnl.setArg(5, height_y);
	krnl.setArg(6, width_y);
	krnl.setArg(7, height_u_y);
	krnl.setArg(8, width_u_y);

	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	
	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_U.png",outputimg1);
	cv::imwrite("out_V.png",outputimg2);

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[5],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);

#endif

#if RGBA2YUV4

	
	inputimg = cv::imread(argv[1], 1);

	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGBA);

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgba2yuv4");

	
	std::vector<cl::Memory> inBuf_rgba,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDevicergba(context ,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*4));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));
	cl::Buffer imageFromDevicev(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergba, CL_TRUE, 0, (inputimg.rows*inputimg.cols*4),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergba);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceu);
	krnl.setArg(3, imageFromDevicev);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	krnl.setArg(6, newheight_y);
	krnl.setArg(7, newwidth_y);
	krnl.setArg(8, newheight_u_v);
	krnl.setArg(9, newwidth_u_v);
	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);

#endif

#if RGBA2IYUV

	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows>>2;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGBA);

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgba2iyuv");

	
	std::vector<cl::Memory> inBuf_rgba,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDevicergba(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*4));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));
	cl::Buffer imageFromDevicev(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergba, CL_TRUE, 0, (inputimg.rows*inputimg.cols*4),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergba);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceu);
	krnl.setArg(3, imageFromDevicev);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	krnl.setArg(6, newheight_y);
	krnl.setArg(7, newwidth_y);
	krnl.setArg(8, newheight_u_v);
	krnl.setArg(9, newwidth_u_v);
	
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
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_U.png",outputimg1);
	cv::imwrite("out_V.png",outputimg2);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);

	imwrite("out_Y_error.png", error_img0);
	imwrite("out_U_error.png", error_img1);
	imwrite("out_V_error.png", error_img2);

#endif

#if RGBA2NV12

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGBA);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;

		/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgba2nv12");

	
	std::vector<cl::Memory> inBuf_rgba,outBuf_y,outBuf_uv;
	cl::Buffer imageToDevicergba(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols*4));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(newwidth_uv*newheight_uv*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergba, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols*4),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergba);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_uv);
	krnl.setArg(8, newwidth_uv);
	
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
		
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newwidth_uv*newheight_uv*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);


	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_UV.png",outputimg1);
	



	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif

#if RGBA2NV21

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGBA);

		/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgba2nv21");

	
	std::vector<cl::Memory> inBuf_rgba,outBuf_y,outBuf_uv;
	cl::Buffer imageToDevicergba(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols*4));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(newwidth_uv*newheight_uv*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergba, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols*4),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergba);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_uv);
	krnl.setArg(8, newwidth_uv);
	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newwidth_uv*newheight_uv*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_VU.png",outputimg1);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif

#if RGB2IYUV

	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows>>2;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGB);


	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2iyuv");

	
	std::vector<cl::Memory> inBuf_rgb,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));
	cl::Buffer imageFromDevicev(context, CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));

	printf("finished buffer creation task\n");
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceu);
	krnl.setArg(3, imageFromDevicev);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	krnl.setArg(6, newheight_y);
	krnl.setArg(7, newwidth_y);
	krnl.setArg(8, newheight_u_v);
	krnl.setArg(9, newwidth_u_v);
	
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
	
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_U.png",outputimg1);
	cv::imwrite("out_V.png",outputimg2);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);

	imwrite("out_Y_error.png", error_img0);
	imwrite("out_U_error.png", error_img1);
	imwrite("out_V_error.png", error_img2);

#endif
#if RGB2NV12

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGB);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2nv12");

	
	std::vector<cl::Memory> inBuf_rgb,outBuf_y,outBuf_uv;
	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(newwidth_uv*newheight_uv*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_uv);
	krnl.setArg(8, newwidth_uv);
	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newwidth_uv*newheight_uv*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_UV.png",outputimg1);
	cv::imwrite("out_Y.png",outputimg0);



	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif
#if BGR2NV12

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2nv12");

	
	std::vector<cl::Memory> inBuf_rgb,outBuf_y,outBuf_uv;
	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(newwidth_uv*newheight_uv*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_uv);
	krnl.setArg(8, newwidth_uv);
	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newwidth_uv*newheight_uv*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_UV.png",outputimg1);
	cv::imwrite("out_Y.png",outputimg0);



	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif
#if RGB2NV21

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGB);

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2nv21");
	
	std::vector<cl::Memory> inBuf_rgb,outBuf_y,outBuf_uv;
	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(newwidth_uv*newheight_uv*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);

	

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_uv);
	krnl.setArg(8, newwidth_uv);
	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newwidth_uv*newheight_uv*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_VU.png",outputimg1);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif

#if BGR2NV21

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);
	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg0.rows;
	int width = inputimg0.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2nv21");
	
	std::vector<cl::Memory> inBuf_rgb,outBuf_y,outBuf_uv;
	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg0.rows*inputimg0.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(newwidth_uv*newheight_uv*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg0.rows*inputimg0.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg0.data);

	

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_uv);
	krnl.setArg(8, newwidth_uv);
	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newwidth_uv*newheight_uv*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_VU.png",outputimg1);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif

#if RGB2YUV4


	inputimg = cv::imread(argv[1], 1);

	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGB);

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2yuv4");

	
	std::vector<cl::Memory> inBuf_rgb,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));
	cl::Buffer imageFromDevicev(context, CL_MEM_WRITE_ONLY,(newwidth_u_v*newheight_u_v));

	printf("finished buffer creation task\n");
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceu);
	krnl.setArg(3, imageFromDevicev);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	krnl.setArg(6, newheight_y);
	krnl.setArg(7, newwidth_y);
	krnl.setArg(8, newheight_u_v);
	krnl.setArg(9, newwidth_u_v);
	
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_U.png",outputimg1);
	cv::imwrite("out_V.png",outputimg2);




	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);

#endif

#if RGB2YUYV


	inputimg = cv::imread(argv[1], 1);

	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGB);

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2yuyv");

	

	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*3));
	cl::Buffer imageFromDeviceyuyv(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y*2));

	printf("finished buffer creation task\n");
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*3),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDeviceyuyv);
	krnl.setArg(2, height);
	krnl.setArg(3, width);

	
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
	
	q.enqueueReadBuffer(imageFromDeviceyuyv, CL_TRUE, 0, (newwidth_y*newheight_y*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_YUYV.png",outputimg0);




	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data) {
		printf("Failed to open YUYV reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
#endif

#if BGR2YUYV


	inputimg = cv::imread(argv[1], 1);

	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);
	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2yuyv");

	

	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*3));
	cl::Buffer imageFromDeviceyuyv(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y*2));

	printf("finished buffer creation task\n");
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*3),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDeviceyuyv);
	krnl.setArg(2, height);
	krnl.setArg(3, width);

	
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
	
	q.enqueueReadBuffer(imageFromDeviceyuyv, CL_TRUE, 0, (newwidth_y*newheight_y*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_YUYV.png",outputimg0);




	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data) {
		printf("Failed to open YUYV reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
#endif



#if RGB2UYVY


	inputimg = cv::imread(argv[1], 1);

	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGB);

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2uyvy");

	

	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*3));
	cl::Buffer imageFromDeviceyuyv(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y*2));

	printf("finished buffer creation task\n");
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*3),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDeviceyuyv);
	krnl.setArg(2, height);
	krnl.setArg(3, width);

	
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
	
	q.enqueueReadBuffer(imageFromDeviceyuyv, CL_TRUE, 0, (newwidth_y*newheight_y*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_YUYV.png",outputimg0);
	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data) {
		printf("Failed to open YUYV reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
#endif

#if BGR2UYVY
	inputimg = cv::imread(argv[1], 1);

	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);
	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2uyvy");

	

	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*3));
	cl::Buffer imageFromDeviceyuyv(context,CL_MEM_WRITE_ONLY,(newwidth_y*newheight_y*2));

	printf("finished buffer creation task\n");
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*3),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);


	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDeviceyuyv);
	krnl.setArg(2, height);
	krnl.setArg(3, width);

	
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
	
	q.enqueueReadBuffer(imageFromDeviceyuyv, CL_TRUE, 0, (newwidth_y*newheight_y*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cv::imwrite("out_YUYV.png",outputimg0);
	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data) {
		printf("Failed to open YUYV reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
#endif


#if UYVY2IYUV
	inputimg =cv::imread(argv[1],-1);
	if (!inputimg.data) {
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows >> 2;

	cv::Size S0(newwidth_y, newheight_y);
	outputimg0.create(S0, CV_8UC1);

	cv::Size S1(newwidth_u_v, newheight_u_v);
	outputimg1.create(S1, CV_8UC1);
	outputimg2.create(S1, CV_8UC1);
	error_img0.create(S0, CV_8UC1);
	error_img1.create(S1, CV_8UC1);
	error_img2.create(S1, CV_8UC1);


					/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_uyvy2iyuv");

	
	std::vector<cl::Memory> inBuf_uyvy,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDeviceuyvy(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicey(context, CL_MEM_WRITE_ONLY,(newheight_y*newwidth_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY,(newheight_u_v*newwidth_u_v));
	cl::Buffer imageFromDevicev(context,CL_MEM_WRITE_ONLY,(newheight_u_v*newwidth_u_v));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDeviceuyvy, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceuyvy);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceu);
	krnl.setArg(3, imageFromDevicev);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	krnl.setArg(6, newheight_y);
	krnl.setArg(7, newwidth_y);
	krnl.setArg(8, newheight_u_v);
	krnl.setArg(9, newwidth_u_v);
	krnl.setArg(10, newheight_u_v);
	krnl.setArg(11, newwidth_u_v);
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);


	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_U.png",outputimg1);
	cv::imwrite("out_V.png",outputimg2);

	refimage0 =cv::imread(argv[2],0);
	if (!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 =cv::imread(argv[3],0);
	if (!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 =cv::imread(argv[4],0);
	if (!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	imwrite("out_Y_error.png", error_img0);
	cv::absdiff(refimage1, outputimg1, error_img1);
	imwrite("out_U_error.png", error_img1);
	cv::absdiff(refimage2, outputimg2, error_img2);
	imwrite("out_V_error.png", error_img2);
#endif


#if UYVY2NV12

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols>>1;
	int newheight_u_v = inputimg.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;


/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_uyvy2nv12");

	
	std::vector<cl::Memory> inBuf_uyvy,outBuf_y,outBuf_uv;
	cl::Buffer imageToDeviceuyvy(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicey(context, CL_MEM_WRITE_ONLY,(newheight_y*newwidth_y));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(newheight_u_v*newwidth_u_v*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceuyvy, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceuyvy);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_u_v);
	krnl.setArg(8, newwidth_u_v);

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
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newheight_u_v*newwidth_u_v*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_UV.png",outputimg1);


	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);
#endif
#if UYVY2NV21

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols>>1;
	int newheight_u_v = inputimg.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;


/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_uyvy2nv21");

	
	std::vector<cl::Memory> inBuf_uyvy,outBuf_y,outBuf_uv;
	cl::Buffer imageToDeviceuyvy(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicey(context, CL_MEM_WRITE_ONLY,(newheight_y*newwidth_y));
	cl::Buffer imageFromDeviceuv(context, CL_MEM_WRITE_ONLY,(newheight_u_v*newwidth_u_v*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceuyvy, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceuyvy);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_u_v);
	krnl.setArg(8, newwidth_u_v);

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
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newheight_u_v*newwidth_u_v*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_UV.png",outputimg1);


	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);
#endif
#if UYVY2YUYV

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;


/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_uyvy2yuyv");

	
	std::vector<cl::Memory> inBuf_uyvy,outBuf_yuyv;
	cl::Buffer imageToDeviceuyvy(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDeviceyuyv(context, CL_MEM_WRITE_ONLY,(inputimg.rows*inputimg.cols*2));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceuyvy, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceuyvy);
	krnl.setArg(1, imageFromDeviceyuyv);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	

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
	q.enqueueReadBuffer(imageFromDeviceyuyv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_YUYV.png",outputimg0);

	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data)
	{
		std::cout << "Can't open YUYV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
#endif
#if UYVY2RGBA

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}

	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;
	cv::Mat outputimgrgba;
	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC4);
	error_img0.create(S0,CV_8UC4);
	
	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_uyvy2rgba");

	
	std::vector<cl::Memory> inBuf_uyvy,outBuf_rgba;
	cl::Buffer imageToDeviceuyvy(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicergba(context,CL_MEM_WRITE_ONLY,(inputimg.rows*inputimg.cols*4));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceuyvy, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceuyvy);
	krnl.setArg(1, imageFromDevicergba);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicergba, CL_TRUE, 0, (inputimg.rows*inputimg.cols*4),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cvtColor(outputimg0,outputimg0,CV_RGBA2BGR);
	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to open reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);
#endif



#if UYVY2RGB

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}

	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;
	
	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);


		/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_uyvy2rgb");

	
	std::vector<cl::Memory> inBuf_uyvy,outBuf_rgb;
	cl::Buffer imageToDeviceuyvy(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicergb(context,CL_MEM_WRITE_ONLY,(inputimg.rows*inputimg.cols*3));

	
	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceuyvy, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);
	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceuyvy);
	krnl.setArg(1, imageFromDevicergb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*3),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	

	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);

	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to open reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);

#endif


#if YUYV2IYUV
	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows>>2;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);

	error_img0.create(S0,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;

/////////////////////////////////////// CL /////////////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_yuyv2iyuv");
	
	std::vector<cl::Memory> inBuf_yuyv,outBuf_y,outBuf_u,outBuf_v;
	cl::Buffer imageToDeviceyuyv(context,CL_MEM_READ_ONLY ,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY ,(newwidth_y*newheight_y));
	cl::Buffer imageFromDeviceu(context,CL_MEM_WRITE_ONLY ,(newheight_u_v*newwidth_u_v));
	cl::Buffer imageFromDevicev(context,CL_MEM_WRITE_ONLY ,(newheight_u_v*newwidth_u_v));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceyuyv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceyuyv);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceu);
	krnl.setArg(3, imageFromDevicev);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	krnl.setArg(6, newheight_y);
	krnl.setArg(7, newwidth_y);
	krnl.setArg(8, newheight_u_v);
	krnl.setArg(9, newwidth_u_v);
	krnl.setArg(10, newheight_u_v);
	krnl.setArg(11, newwidth_u_v);
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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceu, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);
	q.enqueueReadBuffer(imageFromDevicev, CL_TRUE, 0, (newwidth_u_v*newheight_u_v),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg2.data);

	q.finish();
	printf("write output buffer\n");

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	cv::absdiff(refimage0,outputimg0, error_img0);
	cv::absdiff(refimage1,outputimg1, error_img1);
	cv::absdiff(refimage2,outputimg2, error_img2);

#endif

#if YUYV2NV12

	inputimg =cv::imread(argv[1],-1);
	if (!inputimg.data) {
		return -1;
	}
	int newwidth_y = inputimg.cols; //>>1;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols >> 1;
	int newheight_u_v = inputimg.rows >> 1;

	cv::Size S0(newwidth_y, newheight_y);
	outputimg0.create(S0, CV_8UC1);
	error_img0.create(S0, CV_8UC1);

	cv::Size S1(newwidth_u_v, newheight_u_v);
	outputimg1.create(S1, CV_16UC1);
	error_img1.create(S1, CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;

/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_yuyv2nv12");

	
	std::vector<cl::Memory> inBuf_yuyv,outBuf_y,outBuf_uv;
	cl::Buffer imageToDeviceyuyv(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newheight_y*newwidth_y));
	cl::Buffer imageFromDeviceuv(context,CL_MEM_WRITE_ONLY,(newheight_u_v*newwidth_u_v*2));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceyuyv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceyuyv);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_u_v);
	krnl.setArg(8, newwidth_u_v);

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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newwidth_u_v*newheight_u_v*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_UV.png",outputimg1);

	printf("\n Written output images\n");
	refimage0 =cv::imread(argv[2],0);
	if (!refimage0.data) {
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 =cv::imread(argv[3],-1);
	if (!refimage1.data) {
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0, refimage0, error_img0);
	absdiff(outputimg1, refimage1, error_img1);

	imwrite("error_Y.png", error_img0);
	imwrite("error_UV.png", error_img1);
#endif
#if YUYV2NV21

	inputimg =cv::imread(argv[1],-1);
	if (!inputimg.data) {
		return -1;
	}
	int newwidth_y = inputimg.cols; //>>1;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols >> 1;
	int newheight_u_v = inputimg.rows >> 1;

	cv::Size S0(newwidth_y, newheight_y);
	outputimg0.create(S0, CV_8UC1);
	error_img0.create(S0, CV_8UC1);

	cv::Size S1(newwidth_u_v, newheight_u_v);
	outputimg1.create(S1, CV_16UC1);
	error_img1.create(S1, CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;

/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_yuyv2nv21");

	
	std::vector<cl::Memory> inBuf_yuyv,outBuf_y,outBuf_uv;
	cl::Buffer imageToDeviceyuyv(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicey(context,CL_MEM_WRITE_ONLY,(newheight_y*newwidth_y));
	cl::Buffer imageFromDeviceuv(context,CL_MEM_WRITE_ONLY,(newheight_u_v*newwidth_u_v*2));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceyuyv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceyuyv);
	krnl.setArg(1, imageFromDevicey);
	krnl.setArg(2, imageFromDeviceuv);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	krnl.setArg(5, newheight_y);
	krnl.setArg(6, newwidth_y);
	krnl.setArg(7, newheight_u_v);
	krnl.setArg(8, newwidth_u_v);

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
	
	q.enqueueReadBuffer(imageFromDevicey, CL_TRUE, 0, (newwidth_y*newheight_y),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.enqueueReadBuffer(imageFromDeviceuv, CL_TRUE, 0, (newwidth_u_v*newheight_u_v*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg1.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cv::imwrite("out_Y.png",outputimg0);
	cv::imwrite("out_UV.png",outputimg1);

	printf("\n Written output images\n");
	refimage0 =cv::imread(argv[2],0);
	if (!refimage0.data) {
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 =cv::imread(argv[3],-1);
	if (!refimage1.data) {
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0, refimage0, error_img0);
	absdiff(outputimg1, refimage1, error_img1);

	imwrite("error_Y.png", error_img0);
	imwrite("error_UV.png", error_img1);
#endif

#if YUYV2RGBA
	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC4);


/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_yuyv2rgba");

	
	std::vector<cl::Memory> inBuf_yuyv,outBuf_rgba;
	cl::Buffer imageToDeviceyuyv(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicergba(context,CL_MEM_WRITE_ONLY,(inputimg.rows*inputimg.cols*4));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceyuyv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceyuyv);
	krnl.setArg(1, imageFromDevicergba);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicergba, CL_TRUE, 0, (inputimg.rows*inputimg.cols*4),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	cvtColor(outputimg0,outputimg0,CV_RGBA2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to read reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);
		imwrite("error_img0.png", error_img0);
#endif
#if YUYV2UYVY
	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_16UC1);


/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_yuyv2uyvy");


	
	std::vector<cl::Memory> inBuf_yuyv,outBuf_rgba;
	cl::Buffer imageToDeviceyuyv(context,CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDeviceuyvy(context,CL_MEM_WRITE_ONLY,(inputimg.rows*inputimg.cols*2));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceyuyv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceyuyv);
	krnl.setArg(1, imageFromDeviceuyvy);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDeviceuyvy, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	imwrite("out.png", outputimg0);
	refimage = cv::imread(argv[2],-1);
	if(!refimage.data)
	{
		printf("\nFailed to read reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);
#endif
#if YUYV2RGB
	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);

/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_yuyv2rgb");

	
	std::vector<cl::Memory> inBuf_yuyv,outBuf_rgb;
	cl::Buffer imageToDeviceyuyv(context,CL_MEM_READ_ONLY ,(inputimg.rows*inputimg.cols*2));
	cl::Buffer imageFromDevicergb(context,CL_MEM_WRITE_ONLY,(inputimg.rows*inputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceyuyv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*2),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceyuyv);
	krnl.setArg(1, imageFromDevicergb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg0.data);
	

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to read reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);

#endif
#if RGB2GRAY
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC1);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC1);
/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2gray");

	
	
	std::vector<cl::Memory> inBuf_rgb,outBuf_gray;
	cl::Buffer imageToDevicergb(context,CL_MEM_READ_ONLY ,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicegray(context,CL_MEM_WRITE_ONLY ,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */

	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicegray);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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

	q.enqueueReadBuffer(imageFromDevicegray, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2GRAY,1);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2GRAY
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC1);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC1);
	/////////////////////////////////////// CL //////////////////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2gray");

	std::vector<cl::Memory> inBuf_bgr,outBuf_gray;
	cl::Buffer imageToDevicebgr(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicegray(context,CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicebgr, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicebgr);
	krnl.setArg(1, imageFromDevicegray);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicegray, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2GRAY,1);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if GRAY2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 0);
	if(!inputimg.data)
	{
		return -1;
	}
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_gray2rgb");

	
	
	std::vector<cl::Memory> inBuf_gray,outBuf_rgb;
	cl::Buffer imageToDevicegray(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicergb(context,CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicegray, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicegray);
	krnl.setArg(1, imageFromDevicergb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_GRAY2RGB,3);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	cv::absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if GRAY2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 0);
	if(!inputimg.data)
	{
		return -1;
	}
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_gray2bgr");

	
	
	std::vector<cl::Memory> inBuf_gray,outBuf_bgr;
	cl::Buffer imageToDevicegray(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicebgr(context,CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicegray, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);
	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicegray);
	krnl.setArg(1, imageFromDevicebgr);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicebgr, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_GRAY2BGR,3);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if RGB2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	////////////////////////////////////////// CL ///////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2bgr");

	std::vector<cl::Memory> inBuf_rgb,outBuf_xyz;
	cl::Buffer imageToDevicergb(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicexyz(context,CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicexyz);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicexyz, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	//OpenCV reference
	cv::imwrite("ocv_out.jpg",inputimg1);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,inputimg1,error_img0);

#endif
#if BGR2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	////////////////////////////////////////// CL ///////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2rgb");

	std::vector<cl::Memory> inBuf_rgb,outBuf_xyz;
	cl::Buffer imageToDevicergb(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicexyz(context,CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicexyz);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicexyz, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	//OpenCV reference
	cv::cvtColor(inputimg,inputimg,CV_BGR2RGB);

	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,inputimg,error_img0);

#endif
#if RGB2XYZ
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	////////////////////////////////////////// CL ///////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2xyz");

	std::vector<cl::Memory> inBuf_rgb,outBuf_xyz;
	cl::Buffer imageToDevicergb(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicexyz(context,CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicexyz);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicexyz, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2XYZ);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2XYZ
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

/////////////////////////////////////// CL /////////////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2xyz");

	
	
	std::vector<cl::Memory> inBuf_bgr,outBuf_xyz;
	cl::Buffer imageToDevicebgr(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicexyz(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicebgr, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicebgr);
	krnl.setArg(1, imageFromDevicexyz);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicexyz, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2XYZ);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if XYZ2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);
	cv::cvtColor(inputimg,inputimg,CV_RGB2XYZ);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

			/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_xyz2rgb");

	
	
	std::vector<cl::Memory> inBuf_xyz,outBuf_rgb;
	cl::Buffer imageToDevicexyz(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicergb(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicexyz, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicexyz);
	krnl.setArg(1, imageFromDevicergb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2XYZ);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);


		//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_XYZ2RGB);

	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if XYZ2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2XYZ);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_xyz2bgr");
	std::vector<cl::Memory> inBuf_xyz,outBuf_bgr;
	cl::Buffer imageToDevicexyz(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicebgr(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicexyz, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicexyz);
	krnl.setArg(1, imageFromDevicebgr);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicebgr, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
		//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_XYZ2BGR);

	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if RGB2YCrCb
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2ycrcb");

	std::vector<cl::Memory> inBuf_rgb,outBuf_ycrcb;
	cl::Buffer imageToDevicergb(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDeviceycrcb(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");
	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDeviceycrcb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDeviceycrcb, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2YCrCb);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2YCrCb
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2ycrcb");

	
	
	std::vector<cl::Memory> inBuf_bgr,outBuf_ycrcb;
	cl::Buffer imageToDevicebgr(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDeviceycrcb(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicebgr, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicebgr);
	krnl.setArg(1, imageFromDeviceycrcb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDeviceycrcb, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);

	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////


	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2YCrCb);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if YCrCb2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2YCrCb);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_ycrcb2rgb");

	
	
	std::vector<cl::Memory> inBuf_ycrcb,outBuf_rgb;
	cl::Buffer imageToDeviceycrcb(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicergb(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceycrcb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceycrcb);
	krnl.setArg(1, imageFromDevicergb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////
		//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_YCrCb2RGB);

	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if YCrCb2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2YCrCb);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

		/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_ycrcb2bgr");	
	
	std::vector<cl::Memory> inBuf_ycrcb,outBuf_bgr;
	cl::Buffer imageToDeviceycrcb(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicebgr(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDeviceycrcb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceycrcb);
	krnl.setArg(1, imageFromDevicebgr);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicebgr, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

		//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_YCrCb2BGR);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);
	cv::imwrite("error_img0.jpg",error_img0);
#endif
#if RGB2HLS
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2hls");

	
	
	std::vector<cl::Memory> inBuf_rgb,outBuf_hls;
	cl::Buffer imageToDevicergb(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicehls(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");


	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);
	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicehls);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicehls, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL ////////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2HLS);
	
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2HLS
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

			/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2hls");

	
	
	std::vector<cl::Memory> inBuf_bgr,outBuf_hls;
	cl::Buffer imageToDevicebgr(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicehls(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicebgr, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicebgr);
	krnl.setArg(1, imageFromDevicehls);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicehls, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL ////////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2HLS);
	//c_Ref((float*)inputimg.data,(float*)ocv_outputimg.data,3);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if HLS2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg,inputimg,CV_BGR2HLS);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

				/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_hls2rgb");

	
	
	std::vector<cl::Memory> inBuf_hls,outBuf_rgb;
	cl::Buffer imageToDevicehls(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicergb(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicehls, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicehls);
	krnl.setArg(1, imageFromDevicergb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL ////////////////////////
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_HLS2RGB);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);


#endif
#if HLS2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg,inputimg,CV_BGR2HLS);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
			/////////////////////////////////////// CL ////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_hls2bgr");

	
	
	std::vector<cl::Memory> inBuf_hls,outBuf_bgr;
	cl::Buffer imageToDevicehls(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicebgr(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicehls, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);
	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicehls);
	krnl.setArg(1, imageFromDevicebgr);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicebgr, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL ////////////////////////
	
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_HLS2BGR);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);


#endif
#if RGB2HSV
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1> imgOutput(outputimg.rows,outputimg.cols);


	//////////////////////////////////////////// CL ///////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_rgb2hsv");

	std::vector<cl::Memory> inBuf_rgb,outBuf_hsv;
	cl::Buffer imageToDevicergb(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols));
	cl::Buffer imageFromDevicehsv(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicergb, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);

	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicergb);
	krnl.setArg(1, imageFromDevicehsv);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicehsv, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2HSV);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2HSV
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	
	//////////////////////////////////////////// CL ///////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_bgr2hsv");

	std::vector<cl::Memory> inBuf_bgr,outBuf_hsv;
	cl::Buffer imageToDevicebgr(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols));
	cl::Buffer imageFromDevicehsv(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicebgr, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicebgr);
	krnl.setArg(1, imageFromDevicehsv);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	
	q.enqueueReadBuffer(imageFromDevicehsv, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2HSV);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if HSV2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg,inputimg,CV_BGR2HSV);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

//////////////////////////////////////////// CL ///////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_hsv2rgb");

	std::vector<cl::Memory> inBuf_hsv,outBuf_rgb;
	cl::Buffer imageToDevicehsv(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicergb(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");

	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicehsv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);
	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicehsv);
	krnl.setArg(1, imageFromDevicergb);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicergb, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_HSV2RGB);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if HSV2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg,inputimg,CV_BGR2HSV);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	//////////////////////////////////////////// CL ///////////////////////////////////
	int height = inputimg.rows;
	int width = inputimg.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_cvt_color");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"cvtcolor_hsv2bgr");

	std::vector<cl::Memory> inBuf_hsv,outBuf_bgr;
	cl::Buffer imageToDevicehsv(context, CL_MEM_READ_ONLY,(inputimg.rows*inputimg.cols*INPUT_CH_TYPE));
	cl::Buffer imageFromDevicebgr(context, CL_MEM_WRITE_ONLY,(outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE));

	printf("finished buffer creation task\n");


	/* Copy input vectors to memory */
	q.enqueueWriteBuffer(imageToDevicehsv, CL_TRUE, 0, (inputimg.rows*inputimg.cols*INPUT_CH_TYPE),(ap_uint<INPUT_PTR_WIDTH>*)inputimg.data);
	printf("finished enqueueing task\n");

	// Set the kernel arguments
	krnl.setArg(0, imageToDevicehsv);
	krnl.setArg(1, imageFromDevicebgr);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
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
	q.enqueueReadBuffer(imageFromDevicebgr, CL_TRUE, 0, (outputimg.rows*outputimg.cols*OUTPUT_CH_TYPE),(ap_uint<OUTPUT_PTR_WIDTH>*)outputimg.data);
	q.finish();
	printf("write output buffer\n");
/////////////////////////////////////// end of CL /////////////////////

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_HSV2BGR);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
	double minval, maxval;
	float err_per;
	int cnt;

	minval = 255;
	maxval = 0;
	cnt = 0;
	for (int i = 0; i < error_img0.rows; i++) {
		for (int j = 0; j < error_img0.cols; j++) {
			uchar v = error_img0.at<uchar>(i, j);

			if (v > ERROR_THRESHOLD)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	err_per = 100.0 * (float) cnt / (error_img0.rows * error_img0.cols);
	fprintf(stderr,
			"Minimum error in intensity = %f\n\
									Maximum error in intensity = %f\n\
									Percentage of pixels above error threshold = %f\n",
			minval, maxval, err_per);

	if (err_per > 3.0f) 
	{
		printf("\n1st Image Test Failed\n");
		return 1;
	}

#if (IYUV2NV12 || RGBA2NV12 ||RGB2NV12 ||BGR2NV12 ||BGR2NV21||RGB2NV21|| RGBA2NV21 || UYVY2NV12 || YUYV2NV12 || NV122IYUV || NV212IYUV || IYUV2YUV4 || NV122YUV4 || NV212YUV4 || RGBA2IYUV || RGBA2YUV4 || UYVY2IYUV || YUYV2IYUV ||RGB2IYUV||RGB2YUV4 ||NV122NV21 ||NV212NV12)
	minval = 255;
	maxval = 0;
	cnt = 0;
	for (int i = 0; i < error_img1.rows; i++) {
		for (int j = 0; j < error_img1.cols; j++) {
			uchar v = error_img1.at<uchar>(i, j);

			if (v > ERROR_THRESHOLD)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	err_per = 100.0 * (float) cnt / (error_img1.rows * error_img1.cols);
	fprintf(stderr,
			"Minimum error in intensity = %f\n\
							Maximum error in intensity = %f\n\
							Percentage of pixels above error threshold = %f\n",
			minval, maxval, err_per);
	if (err_per > 3.0f) 
	{
		printf("\n2nd Image Test Failed\n");
		return 1;
	}

#endif
#if (IYUV2YUV4 || NV122IYUV || NV122YUV4 || NV212IYUV || NV212YUV4 || RGBA2IYUV ||RGB2IYUV || RGBA2YUV4 || UYVY2IYUV || YUYV2IYUV ||RGB2YUV4 )
	minval = 255;
	maxval = 0;
	cnt = 0;
	for (int i = 0; i < error_img2.rows; i++) {
		for (int j = 0; j < error_img2.cols; j++) {
			uchar v = error_img2.at<uchar>(i, j);

			if (v > ERROR_THRESHOLD)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	err_per = 100.0 * (float) cnt / (error_img2.rows * error_img2.cols);
	fprintf(stderr,
			"Minimum error in intensity = %f\n\
							Maximum error in intensity = %f\n\
							Percentage of pixels above error threshold = %f\n",
			minval, maxval, err_per);
	if (err_per > 3.0f) 
	{
		printf("\n3rd Image Test Failed\n");
		return 1;
	}
#endif
	/* ## *************************************************************** ##*/
return 0;

}

