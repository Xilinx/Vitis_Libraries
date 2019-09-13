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
#include "xf_integral_image_config.h"

#include <CL/cl.h>
#include "xcl2.hpp" 

int main(int argc, char** argv)
{
	cv::Mat in_img, in_img1, out_img, ocv_ref, ocv_ref1;
	cv::Mat in_gray, in_gray1, diff;

	if(argc != 2)
	{
		fprintf(stderr, "Usage: <executable> <input image>\n");
		return -1;
	}	

	// Read input image
	in_img = cv::imread(argv[1], 0);
	if (in_img.data == NULL)
	{
		//cout << "Can't open image !!" << endl;
		return -1;
	}


	// create memory for output images
	ocv_ref.create(in_img.rows,in_img.cols,CV_32S);
	ocv_ref1.create(in_img.rows,in_img.cols,CV_32S);

	cv::integral(in_img, ocv_ref, -1);

	for(int i = 0; i < in_img.rows; i++)
	{
		for(int j=0; j<in_img.cols; j++)
		{
			ocv_ref1.at<unsigned int>(i,j) =  ocv_ref.at<unsigned int>(i+1, j+1);
		}
	}

	imwrite("out_ocv.png",ocv_ref1);


	// create memory for output image
	diff.create(in_img.rows,in_img.cols,CV_32S);
	out_img.create(in_img.rows,in_img.cols,CV_32S);

	
/////////////////////////////////////// CL ////////////////////////

	int height = in_img.rows;
	int width = in_img.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_integral_image");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"integral_accel");
	
		

	std::vector<cl::Memory> inBufVec, outBufVec;
	cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY,(height*width));//,in_img.data);
	cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY,(height*width*4));//,out_img.data);
	
	//fprintf(stderr,"before enqueue write buffer");
	
	q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (height*width), in_img.data);
	
	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageFromDevice);
	krnl.setArg(2, height);
	krnl.setArg(3, width);
	
	// Profiling Objects
	cl_ulong start= 0;
	cl_ulong end = 0;
	double diff_prof = 0.0f;
	cl::Event event_sp;

	//fprintf(stderr,"before kernel");
	// Launch the kernel 
	q.enqueueTask(krnl,NULL,&event_sp);
	clWaitForEvents(1, (const cl_event*) &event_sp);

	//fprintf(stderr,"after kernel");

	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START,&start);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END,&end);
	diff_prof = end-start;
	std::cout<<(diff_prof/1000000)<<"ms"<<std::endl;

	//Copying Device result data to Host memory
	q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, (height*width*4), out_img.data);
	q.finish();
/////////////////////////////////////// end of CL ////////////////////////

	// Write output image
	imwrite("hls_out.jpg",out_img);

	// Compute absolute difference image
	absdiff(ocv_ref1, out_img, diff);

	// Save the difference image 
	imwrite("diff.png", diff); 

	float err_per;
	//xf::analyzeDiff(diff, 1, err_per);
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j = 0; j < in_img.cols; j++)
		{
			unsigned int v = diff.at<unsigned int>(i,j);

			if (v > 0)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}

	}

	err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
