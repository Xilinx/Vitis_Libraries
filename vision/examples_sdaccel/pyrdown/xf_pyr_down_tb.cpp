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
#include "xf_pyr_down_config.h"

#include <CL/cl.h>
#include "xcl2.hpp" 


int main(int argc, char *argv[]){

	cv::Mat input_image, output_image, output_xf, output_diff_xf_cv;


	input_image = cv::imread(argv[1],0);

	int input_height = input_image.rows;
	int input_width = input_image.cols;

	int output_height = (input_image.rows + 1) >> 1;
	int output_width = (input_image.cols + 1) >> 1;

	output_xf.create(output_height,output_width,CV_8UC1);
	output_diff_xf_cv.create(output_height,output_width,CV_8UC1);


	std::cout << "Input Height " << input_height << " Input_Width " << input_width << std::endl;
	std::cout << "Output Height " << output_height << " Output_Width " << output_width << std::endl;

	cv::pyrDown(input_image, output_image, cv::Size(output_width,output_height),cv::BORDER_REPLICATE);

	cv::imwrite("opencv_image.png",output_image);
///////////////   End of OpenCV reference     /////////////////

	////////////////////	HLS TOP function call	/////////////////

/////////////////////////////////////// CL ////////////////////////
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>();
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_pyr_down");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"pyr_down_accel");	
	
	std::vector<cl::Memory> inBufVec, outBufVec;
	cl::Buffer imageToDevice(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,(input_height*input_width*CH_TYPE),input_image.data);
	cl::Buffer imageFromDevice(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,(output_height*output_width*CH_TYPE),output_xf.data);

	inBufVec.push_back(imageToDevice);
	outBufVec.push_back(imageFromDevice);
	
	/* Copy input vectors to memory */
	q.enqueueMigrateMemObjects(inBufVec,0/* 0 means from host*/);
	
	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, imageFromDevice);
	krnl.setArg(2, input_height);
	krnl.setArg(3, input_width);
	krnl.setArg(4, output_height);
	krnl.setArg(5, output_width);

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

	q.enqueueMigrateMemObjects(outBufVec,CL_MIGRATE_MEM_OBJECT_HOST);
	q.finish();
/////////////////////////////////////// end of CL ////////////////////////
	

	int num_errors_xf = 0;
	unsigned char max_error = 0;
	unsigned char min_error = 255;
	
	for(int i=0;i<output_height;i++)
	{
		for(int j=0;j<output_width;j++)
		{
			if(output_xf.at<unsigned char>(i,j) == output_image.at<unsigned char>(i,j))
			{
				output_diff_xf_cv.at<unsigned char>(i,j) = 0;
			}
			else
			{
				output_diff_xf_cv.at<unsigned char>(i,j) = 255;
				
				unsigned char temp1 = output_xf.at<unsigned char>(i,j);
				unsigned char temp2 = output_image.at<unsigned char>(i,j);
				unsigned char temp;
				temp = std::abs(temp1-temp2);
				if(temp>max_error)
				{
					std::cout << "Location of error " << i << "," << j << std::endl;
					max_error = temp;
					
				}
				else if(temp < min_error)
				{
					min_error = temp;
				}
				if(temp>0)
				{
					num_errors_xf++;
				}
			}
		}
	}
	std::cout << "number of differences between opencv and xf: " << num_errors_xf << std::endl;
	std::cout << "Max Error between opencv and xf: " << (unsigned int)max_error << std::endl;
	std::cout << "Min Error between opencv and xf: " << (unsigned int)min_error << std::endl;
	cv::imwrite("xf_cv_diff_image.png", output_diff_xf_cv);
	cv::imwrite("xf_image.png", output_xf);
	if(max_error > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
