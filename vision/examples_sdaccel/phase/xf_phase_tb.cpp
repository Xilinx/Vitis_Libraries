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
#include "xf_phase_config.h"

#include <CL/cl.h>
#include "xcl2.hpp" 

int main( int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray, c_grad_x, c_grad_y, c_grad_x1,
	c_grad_y1, ocv_ref, out_img, diff;

	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	int filter_size = 3;

	/*  reading in the color image  */
	in_img = cv::imread(argv[1],1);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	/*  convert to gray  */
	cvtColor(in_img,in_gray,CV_BGR2GRAY);

	/////////	OpenCV Phase computation API    ///////
	cv::Sobel(in_gray, c_grad_x1, CV_32FC1, 1, 0, filter_size, scale, delta, cv::BORDER_CONSTANT );
	cv::Sobel(in_gray, c_grad_y1, CV_32FC1, 0, 1, filter_size, scale, delta, cv::BORDER_CONSTANT );

#if DEGREES
	phase(c_grad_x1, c_grad_y1, ocv_ref, true);
#elif RADIANS
	phase(c_grad_x1, c_grad_y1, ocv_ref, false);
#endif
	/////////   End Opencv Phase computation API  ///////

	cv::Sobel( in_gray, c_grad_x, ddepth, 1, 0, filter_size,
			scale, delta, cv::BORDER_CONSTANT );
	cv::Sobel( in_gray, c_grad_y, ddepth, 0, 1, filter_size,
			scale, delta, cv::BORDER_CONSTANT );

	out_img.create(in_gray.rows,in_gray.cols,CV_16S);

	
/////////////////////////////////////// CL ////////////////////////

	int height = in_img.rows;
	int width = in_img.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_phase");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"phase_accel");
	
		

	std::vector<cl::Memory> inBufVec, inBufVec1,outBufVec;
	cl::Buffer imageToDevice(context, CL_MEM_READ_ONLY,(height*width*2));//,(ap_uint<INPUT_PTR_WIDTH>*)c_grad_x.data);
	cl::Buffer imageToDevice1(context, CL_MEM_READ_ONLY,(height*width*2));//,(ap_uint<INPUT_PTR_WIDTH>*)c_grad_y.data);
	cl::Buffer imageFromDevice(context, CL_MEM_WRITE_ONLY,(height*width*2));//,(ap_uint<OUTPUT_PTR_WIDTH>*)out_img.data);
		
	q.enqueueWriteBuffer(imageToDevice, CL_TRUE, 0, (height*width*2), (ap_uint<INPUT_PTR_WIDTH>*)c_grad_x.data);
	q.enqueueWriteBuffer(imageToDevice1, CL_TRUE, 0, (height*width*2), (ap_uint<INPUT_PTR_WIDTH>*)c_grad_y.data);
	
	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageToDevice1);
	krnl.setArg(2, imageFromDevice);
	krnl.setArg(3, height);
	krnl.setArg(4, width);
	
	
	// Profiling Objects
	cl_ulong start= 0;
	cl_ulong end = 0;
	double diff_prof = 0.0f;
	cl::Event event_sp;

	printf("before kernel .... !!!\n");
	// Launch the kernel 
	q.enqueueTask(krnl,NULL,&event_sp);
	clWaitForEvents(1, (const cl_event*) &event_sp);
	printf("after kernel .... !!!\n");
	

	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START,&start);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END,&end);
	diff_prof = end-start;
	std::cout<<(diff_prof/1000000)<<"ms"<<std::endl;

	q.enqueueReadBuffer(imageFromDevice, CL_TRUE, 0, (height*width*2), (ap_uint<INPUT_PTR_WIDTH>*)out_img.data);
	
	q.finish();
/////////////////////////////////////// end of CL ////////////////////////




#if DEGREES
	/////   writing the difference between the OpenCV and the Kernel output into a text file /////
	FILE *fp;
	fp = fopen("diff.txt", "w");
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j = 0; j < in_img.cols; j++)
		{
			short int v = out_img.at<short int>(i,j);
			float v1 = ocv_ref.at<float>(i,j);
			float v2 = v/pow(2.0,6);
			fprintf(fp,"%f ", v1-v2); // converting the output fixed point format from Q4.12 format to float
		}
		fprintf(fp, "\n");
	}
	fclose(fp);

	// Find minimum and maximum differences
	float ocvminvalue, ocvmaxvalue;
	float hlsminvalue, hlsmaxvalue;
	double minval=65535,maxval=0;
	int cnt = 0;
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j=0; j < in_img.cols; j++)
		{
			short int v3 = out_img.at<short int>(i,j);
			float v2 = ocv_ref.at<float>(i,j);
			float v1 ;

			if(DEGREES)
			{
				v1 = v3/(pow(2.0,6));  // converting the output fixed point format from Q4.12 format to float
			}

			float v = (v2-v1);

			if (v > 1)
				cnt++;
			if (minval > v )
			{
				minval = v;
				ocvminvalue = v2;
				hlsminvalue = v1;
			}
			if (maxval < v)
			{
				maxval = v;
				ocvmaxvalue = v2;
				hlsmaxvalue = v1;
			}
		}
	}
	printf("Minimum value ocv = %f Minimum value hls = %f\n", ocvminvalue, hlsminvalue);
	printf("Maximum value ocv = %f Maximum value hls = %f\n", ocvmaxvalue, hlsmaxvalue);

#elif RADIANS

	/////   writing the difference between the OpenCV and the Kernel output into a text file /////
	FILE *fp;
	fp = fopen("diff.txt", "w");
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j = 0; j < in_img.cols; j++)
		{
			short int v = out_img.at<short int>(i,j);
			float v1 = ocv_ref.at<float>(i,j);
			float v2 = v/pow(2.0,12);
			fprintf(fp,"%f ", v1-v2); // converting the output fixed point format from Q4.12 format to float
		}
		fprintf(fp, "\n");
	}
	fclose(fp);

	// Find minimum and maximum differences
	float ocvminvalue, ocvmaxvalue;
	float hlsminvalue, hlsmaxvalue;
	double minval=65535,maxval=0;
	int cnt = 0;
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j=0; j < in_img.cols; j++)
		{
			short int v3 = out_img.at<short int>(i,j);
			float v2 = ocv_ref.at<float>(i,j);
			float v1 ;

			if(RADIANS)
			{
				v1 = v3/(pow(2.0,12));  // converting the output fixed point format from Q4.12 format to float
			}

			float v = (v2-v1);

			if (v > 1)
				cnt++;
			if (minval > v )
			{
				minval = v;
				ocvminvalue = v2;
				hlsminvalue = v1;
			}
			if (maxval < v)
			{
				maxval = v;
				ocvmaxvalue = v2;
				hlsmaxvalue = v1;
			}
		}
	}
	printf("Minimum value ocv = %f Minimum value hls = %f\n", ocvminvalue, hlsminvalue);
	printf("Maximum value ocv = %f Maximum value hls = %f\n", ocvmaxvalue, hlsmaxvalue);

#endif

	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	in_img.~Mat();
	in_gray.~Mat();
	c_grad_x.~Mat();
	c_grad_y.~Mat();
	c_grad_x1.~Mat();
	c_grad_y1.~Mat();
	ocv_ref.~Mat();
	out_img.~Mat();
	diff.~Mat();

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}


