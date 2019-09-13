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
#include "xf_hist_equalize_config.h"

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

	cv::Mat in_img, in_img_copy, out_img, ocv_ref, diff;

	// reading in the color image
	in_img = cv::imread(argv[1], 0);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}

	int height = in_img.rows;
	int width = in_img.cols;

	// create memory for output images
	in_img.copyTo(in_img_copy);
	out_img.create(height, width, XF_8UC1);
	ocv_ref.create(height, width, XF_8UC1);
	diff.create(height, width, XF_8UC1);

	///////////////// 	Opencv  Reference  ////////////////////////
	cv::equalizeHist(in_img, ocv_ref);

/////////////////////////////////////// CL ////////////////////////

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_hist_equalize");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"equalizeHist_accel");

	cl::Buffer imageToDevice1(context, CL_MEM_READ_ONLY,height*width);
	cl::Buffer imageToDevice2(context, CL_MEM_READ_ONLY,height*width);
	cl::Buffer imageFromDevice(context,CL_MEM_WRITE_ONLY,height*width);

	q.enqueueWriteBuffer(imageToDevice1, CL_TRUE, 0, height*width, in_img.data);
	q.enqueueWriteBuffer(imageToDevice2, CL_TRUE, 0, height*width, in_img_copy.data);

	// Set the kernel arguments
	krnl.setArg(0, imageToDevice1);
	krnl.setArg(1, imageToDevice2);
	krnl.setArg(2, imageFromDevice);
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

	q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, height*width, out_img.data);
	q.finish();
/////////////////////////////////////// end of CL ////////////////////////

	//////////////////  Compute Absolute Difference ////////////////////
	cv::absdiff(ocv_ref,out_img,diff);

	cv::imwrite("input.jpg", in_img);
	cv::imwrite("out_ocv.jpg", ocv_ref);
	cv::imwrite("out_hls.jpg", out_img);
	cv::imwrite("out_error.jpg", diff);

	// Find minimum and maximum differences.
	float err_per;
	xf::analyzeDiff(diff, 1, err_per);

	return 0;
}
