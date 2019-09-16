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
#include "xf_mean_shift_config.h"


#include <CL/cl.h>
#include "xcl2.hpp"

int main(int argc, char* argv[])
{

	if( argc != 3)
	{
		printf("Missed input arguments. Usage: <executable> <path to input video file or image path> <Number of objects to be tracked> \n");
		return -1;
	}

	char *path = argv[1];

#if VIDEO_INPUT
	cv::VideoCapture cap(path);
	if(!cap.isOpened()) // check if we succeeded
	{
		std::cout << "ERROR: Cannot open the video file" << std::endl;
		return -1;
	}
#endif
	uint8_t no_objects = atoi(argv[2]);


	uint16_t *c_x = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *c_y = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *h_x = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *h_y = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *tlx = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *tly = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *brx = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *bry = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *track = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *obj_width = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *obj_height = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *dx = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));
	uint16_t *dy = (uint16_t*)malloc(XF_MAX_OBJECTS*sizeof(uint16_t));

	for(int i = 0; i < no_objects; i++)
	{
		dx[i] = 0;
		dy[i] = 0;
	}

	// object loop, for reading input to the object
	for( uint16_t i = 0; i < no_objects; i++)
	{
		h_x[i] = WIDTH_MST[i]/2;
		h_y[i] = HEIGHT_MST[i]/2;
		c_x[i] = X1[i] + h_x[i];
		c_y[i] = Y1[i] + h_y[i];

		obj_width[i] = WIDTH_MST[i];
		obj_height[i] = HEIGHT_MST[i];

		tlx[i] = X1[i];
		tly[i] = Y1[i];
		brx[i] = c_x[i] + h_x[i];
		bry[i] = c_y[i] + h_y[i];
		track[i] = 1;
	}

	cv::Mat frame, image;
	int no_of_frames = TOTAL_FRAMES;
	char nm[1000];

#if VIDEO_INPUT
		cap >> frame;
#else
		sprintf(nm,"%s/img%d.png",path,1);
		frame = cv::imread(nm,1);
#endif

	int height = frame.rows;
	int width = frame.cols;

	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);

	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>();
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_mean_shift");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"mean_shift_accel");

	for(int f_no = 1; f_no <= no_of_frames; f_no++)
	{
		if (f_no > 1) {
#if VIDEO_INPUT
		cap >> frame;
#else
		sprintf(nm,"%s/img%d.png",path,f_no);
		frame = cv::imread(nm,1);
#endif
		}


		if( frame.empty() ) {
			printf("no image!\n");
			break;
		}
		frame.copyTo(image);

		// convert to four channels with a dummy alpha channel for 32-bit data transfer
		cvtColor(image,image,cv::COLOR_BGR2RGBA);

		// set the status of the frame, set as '0' for the first frame
		uint8_t frame_status = 1;
		if (f_no-1 == 0)
			frame_status = 0;

		uint8_t no_of_iterations = 4;

/////////////////////////////////////// CL ////////////////////////

	cl::Buffer imageToDevice(context,CL_MEM_READ_ONLY,(height*width*4));
	cl::Buffer tlxToDevice(context,CL_MEM_READ_ONLY,no_objects*sizeof(unsigned short));
	cl::Buffer tlyToDevice(context,CL_MEM_READ_ONLY,no_objects*sizeof(unsigned short));
	cl::Buffer objHeightToDevice(context,CL_MEM_READ_ONLY,no_objects*sizeof(unsigned short));
	cl::Buffer objWidthToDevice(context,CL_MEM_READ_ONLY,no_objects*sizeof(unsigned short));
	cl::Buffer dxFromDevice(context,CL_MEM_WRITE_ONLY,no_objects*sizeof(unsigned short));
	cl::Buffer dyFromDevice(context,CL_MEM_WRITE_ONLY,no_objects*sizeof(unsigned short));
	cl::Buffer trackFromDevice(context,CL_MEM_READ_WRITE,no_objects*sizeof(unsigned short));

	// Set the kernel arguments
	krnl.setArg(0, imageToDevice);
	krnl.setArg(1, tlxToDevice);
	krnl.setArg(2, tlyToDevice);
	krnl.setArg(3, objHeightToDevice);
	krnl.setArg(4, objWidthToDevice);
	krnl.setArg(5, dxFromDevice);
	krnl.setArg(6, dyFromDevice);
	krnl.setArg(7, trackFromDevice);
	krnl.setArg(8, frame_status);
	krnl.setArg(9, no_objects);
	krnl.setArg(10, no_of_iterations);
	krnl.setArg(11, height);
	krnl.setArg(12, width);

	// initiate write to clBuffer
	q.enqueueWriteBuffer(imageToDevice,CL_TRUE,0,(height*width*4),image.data);
	q.enqueueWriteBuffer(tlxToDevice,CL_TRUE,0,no_objects*sizeof(unsigned short),tlx);
	q.enqueueWriteBuffer(tlyToDevice,CL_TRUE,0,no_objects*sizeof(unsigned short),tly);
	q.enqueueWriteBuffer(objHeightToDevice,CL_TRUE,0,no_objects*sizeof(unsigned short),obj_height);
	q.enqueueWriteBuffer(objWidthToDevice,CL_TRUE,0,no_objects*sizeof(unsigned short),obj_width);
	q.enqueueWriteBuffer(trackFromDevice,CL_TRUE,0,no_objects*sizeof(unsigned short),track);

	// Profiling Objects
	cl_ulong start= 0;
	cl_ulong end = 0;
	double diff_prof = 0.0f;
	cl::Event event_sp;

	// Launch the kernel
	q.enqueueTask(krnl,NULL,&event_sp);

	// profiling
	clWaitForEvents(1, (const cl_event*) &event_sp);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START,&start);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END,&end);
	diff_prof = end-start;
	std::cout<<(diff_prof/1000000)<<"ms"<<std::endl;

	q.enqueueReadBuffer(dxFromDevice, CL_TRUE, 0, no_objects*sizeof(unsigned short), dx);
	q.enqueueReadBuffer(dyFromDevice, CL_TRUE, 0, no_objects*sizeof(unsigned short), dy);
	q.enqueueReadBuffer(trackFromDevice, CL_TRUE, 0, no_objects*sizeof(unsigned short), track);
	q.finish();
/////////////////////////////////////// end of CL ////////////////////////
		
		std::cout<<"frame "<<f_no<<":"<<std::endl;
		for (int k = 0; k < no_objects; k++)
		{
			c_x[k] = dx[k];
			c_y[k] = dy[k];
			tlx[k] = c_x[k] - h_x[k];
			tly[k] = c_y[k] - h_y[k];
			brx[k] = c_x[k] + h_x[k];
			bry[k] = c_y[k] + h_y[k];

			std::cout<<" "<<c_x[k]<<" "<<c_y[k]<<std::endl;
		}
		std::cout<<std::endl;
	}

	return 0;
}
