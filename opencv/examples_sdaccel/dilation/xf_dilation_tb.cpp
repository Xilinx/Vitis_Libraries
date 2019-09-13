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
#include "xf_dilation_config.h"

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

	cv::Mat in_img,out_img,ocv_ref;
	cv::Mat diff;

	// reading in the image
#if GRAY
	in_img = cv::imread(argv[1], 0);
#else
	in_img = cv::imread(argv[1], 1);
#endif


	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n", argv[1]);
		return 0;
	}

	// create memory for output images
#if GRAY
	ocv_ref.create(in_img.rows,in_img.cols,CV_8UC1);
	out_img.create(in_img.rows,in_img.cols,CV_8UC1);
	diff.create(in_img.rows,in_img.cols,CV_8UC1);

#else
	ocv_ref.create(in_img.rows,in_img.cols,CV_8UC3);
	out_img.create(in_img.rows,in_img.cols,CV_8UC3);
	diff.create(in_img.rows,in_img.cols,CV_8UC3);
#endif

#if 1
		cv::Mat element = cv::getStructuringElement( KERNEL_SHAPE,cv::Size(FILTER_SIZE, FILTER_SIZE), cv::Point(-1, -1));
		cv::dilate(in_img, ocv_ref, element, cv::Point(-1, -1), ITERATIONS, cv::BORDER_CONSTANT);
		cv::imwrite("out_ocv.jpg", ocv_ref);
	/////////////////////	End of OpenCV reference	 ////////////////
	////////////////////	HLS TOP function call	/////////////////

	unsigned char structure_element[FILTER_SIZE*FILTER_SIZE];

	for(int i=0;i<(FILTER_SIZE*FILTER_SIZE);i++)
	{
		structure_element[i]=element.data[i];
	}

/////////////////////////////////////// CL ////////////////////////

	int height = in_img.rows;
	int width = in_img.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);

	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	//Create Program and Kernel
	std::string device_name = device.getInfo<CL_DEVICE_NAME>();
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_dilation");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"dilation_accel");

	//Allocate Buffer in Global Memory
	cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY,(height*width*CH_TYPE));
	cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY,(height*width*CH_TYPE));
	cl::Buffer kernelFilterToDevice(context, CL_MEM_READ_ONLY,(FILTER_SIZE*FILTER_SIZE));

	//Copying input data to Device buffer from host memory
	q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (height*width*CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)in_img.data);
	q.enqueueWriteBuffer(kernelFilterToDevice, CL_TRUE, 0, (FILTER_SIZE*FILTER_SIZE), (unsigned char*)structure_element);

	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageFromDevice);
	krnl.setArg(2, kernelFilterToDevice);
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

	// Profiling
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START,&start);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END,&end);
	diff_prof = end-start;
	std::cout<<(diff_prof/1000000)<<"ms"<<std::endl;

	//Copying Device result data to Host memory
	q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, (height*width*CH_TYPE), (ap_uint<INPUT_PTR_WIDTH>*)out_img.data);

	q.finish();
/////////////////////////////////////// end of CL ////////////////////////

	// Write output image
	cv::imwrite("hw_out.jpg",out_img);

	//////////////////  Compute Absolute Difference ////////////////////
	cv::absdiff(ocv_ref,out_img,diff);
	cv::imwrite("out_error.jpg",diff);

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0; i<in_img.rows; i++)
	{
		for(int j=0; j<in_img.cols; j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v>0)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	if(err_per > 0.0f)
		return 1;
#endif

	return 0;
}
