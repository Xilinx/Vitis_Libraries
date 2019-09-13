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
#include "xf_box_filter_config.h"



int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray, in_conv_img, out_img, ocv_ref, diff;

	in_img = cv::imread(argv[1],0); // reading in the color image

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}


	/*  convert to specific types  */
#if T_8U
	in_img.convertTo(in_conv_img,CV_8U);			//Size conversion
#elif T_16U
	in_img.convertTo(in_conv_img,CV_16U);			//Size conversion
#elif T_16S
	in_img.convertTo(in_conv_img,CV_16S);			//Size conversion
#endif

	ocv_ref.create(in_img.rows,in_img.cols,in_conv_img.depth()); // create memory for output image
	diff.create(in_img.rows,in_img.cols,in_conv_img.depth()); // create memory for output image

	/////////////////    OpenCV reference  /////////////////
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
#if FILTER_SIZE_3
	cv::boxFilter(in_conv_img,ocv_ref,-1,cv::Size(3,3),cv::Point(-1,-1),true,cv::BORDER_CONSTANT);
#elif FILTER_SIZE_5
	cv::boxFilter(in_conv_img,ocv_ref,-1,cv::Size(5,5),cv::Point(-1,-1),true,cv::BORDER_CONSTANT);
#elif FILTER_SIZE_7
	cv::boxFilter(in_conv_img,ocv_ref,-1,cv::Size(7,7),cv::Point(-1,-1),true,cv::BORDER_CONSTANT);
#endif
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	unsigned short height  = in_img.rows;
	unsigned short width  = in_img.cols;
	
	static xf::Mat<IN_T, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<IN_T, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> in_8bit(in_img.rows,in_img.cols);

	in_8bit.copyTo(in_img.data);
#if T_8U
	imgInput = in_8bit;
#elif T_16U
	in_8bit.convertTo(imgInput, XF_CONVERT_8U_TO_16U);
#else
	in_8bit.convertTo(imgInput, XF_CONVERT_8U_TO_16S);
#endif
	#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
	#endif


	boxfilter_accel(imgInput,imgOutput);

	#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif


	xf::imwrite("hls_out.jpg", imgOutput);
	imwrite("ref_img.jpg", ocv_ref);  // reference image
	xf::absDiff(ocv_ref,imgOutput, diff);    // Compute absolute difference image
	imwrite("diff_img.jpg",diff);     // Save the difference image for debugging purpose

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0;i<in_img.rows;i++)
	{
		for(int j=0;j<in_img.cols;j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v>1)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
