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

int main(int argc, char** argv)
{

	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img,in_img1,out_img,ocv_ref;
	cv::Mat in_gray,in_gray1,diff;
#if GRAY
	// reading in the color image
	in_gray = cv::imread(argv[1], 0);
#else
	// reading in the color image
	in_gray = cv::imread(argv[1], 1);

#endif


	if (in_gray.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n", argv[1]);
		return 0;
	}

// create memory for output images
#if GRAY
		ocv_ref.create(in_gray.rows,in_gray.cols,CV_8UC1);
		out_img.create(in_gray.rows,in_gray.cols,CV_8UC1);
		diff.create(in_gray.rows,in_gray.cols,CV_8UC1);
#else
		ocv_ref.create(in_gray.rows,in_gray.cols,CV_8UC3);
		out_img.create(in_gray.rows,in_gray.cols,CV_8UC3);
		diff.create(in_gray.rows,in_gray.cols,CV_8UC3);

#endif



		cv::Mat element = cv::getStructuringElement( KERNEL_SHAPE,cv::Size(FILTER_SIZE, FILTER_SIZE), cv::Point(-1, -1));
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
		cv::dilate(in_gray, ocv_ref, element, cv::Point(-1, -1), ITERATIONS, cv::BORDER_CONSTANT);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
		cv::imwrite("out_ocv.jpg", ocv_ref);
	/////////////////////	End of OpenCV reference	 ////////////////

	////////////////////	HLS TOP function call	/////////////////

	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput(in_gray.rows,in_gray.cols);
	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray.rows,in_gray.cols);


#if __SDSCC__
	unsigned char *structure_element=(unsigned char *)sds_alloc_non_cacheable(sizeof(unsigned char)*FILTER_SIZE*FILTER_SIZE);
#else
	unsigned char structure_element[FILTER_SIZE*FILTER_SIZE];
#endif


	for(int i=0;i<(FILTER_SIZE*FILTER_SIZE);i++)
	{
		structure_element[i]=element.data[i];
	}

//	unsigned char iterations=2;

	imgInput.copyTo(in_gray.data);

#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	dilation_accel(imgInput, imgOutput, structure_element);

#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif

	//////////////////  Compute Absolute Difference ////////////////////

	xf::absDiff(ocv_ref, imgOutput, diff);

	cv::imwrite("out_error.jpg", diff);

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0; i<in_gray.rows; i++)
	{
		for(int j=0; j<in_gray.cols; j++)
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
	float err_per = 100.0*(float)cnt/(in_gray.rows*in_gray.cols);
	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	if(err_per > 0.0f)
		return 1;


	return 0;
}
