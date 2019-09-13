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
#include "xf_threshold_config.h"

#include <CL/cl.h>
#include "xcl2.hpp" 

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, out_img, ocv_ref, in_gray, diff;

	unsigned short in_width,in_height;

	/*  reading in the color image  */
	in_img = cv::imread(argv[1],0);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	in_width = in_img.cols;
	in_height = in_img.rows;

	ocv_ref.create(in_img.rows,in_img.cols,in_img.depth());
	out_img.create(in_img.rows,in_img.cols,in_img.depth());
	diff.create(in_img.rows,in_img.cols,in_img.depth());





	////////////////  reference code  ////////////////

	 unsigned char maxval=50;
	 unsigned char thresh=100;

	cv::threshold(in_img,ocv_ref,thresh,maxval,THRESH_TYPE);
   //////////////////  end opencv reference code//////////

	
/////////////////////////////////////// CL ////////////////////////

	int height = in_img.rows;
	int width = in_img.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_threshold");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"threshold_accel");
	
		

	std::vector<cl::Memory> inBufVec, outBufVec;
	cl::Buffer imageToDevice(context,CL_MEM_READ_ONLY,(height*width));
	cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY,(height*width));
	
	
	q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (height*width), (ap_uint<INPUT_PTR_WIDTH>*)in_img.data);
	
	
	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageFromDevice);
	krnl.setArg(2, thresh);
	krnl.setArg(3, maxval);
	krnl.setArg(4, height);
	krnl.setArg(5, width);
	
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
	q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, (height*width), (ap_uint<INPUT_PTR_WIDTH>*)out_img.data);

	q.finish();
	
	/////////////////////////////////////// end of CL ////////////////////////

	// Write output image
	imwrite("hls_out.jpg",out_img);

	// Compute absolute difference image
	absdiff(ocv_ref, out_img, diff);

	// Save the difference image 
	imwrite("diff.png", diff); 

	float err_per;
	xf::analyzeDiff(diff, 0, err_per);

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
