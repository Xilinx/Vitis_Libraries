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
#include "xf_bilateral_filter_config.h"

int main(int argc, char **argv)
{	
	cv::Mat in_img, out_img, ocv_ref, in_img_gau;
	cv::Mat in_gray, in_gray1, diff;

	cv::RNG rng;


	if(argc != 2)
	{
		printf("Usage: <executable> <input image path> \n");
		return -1;
	}
#if GRAY
	in_img = cv::imread(argv[1], 0); // reading in the color image
#else
	in_img = cv::imread(argv[1], 1); // reading in the color image
#endif

	if(!in_img.data)
	{
		printf("Failed to load the image ... !!!");
		return -1;
	}

	//cv::extractChannel(in_img, in_img,1);
	// create memory for output image
#if GRAY
	ocv_ref.create(in_img.rows,in_img.cols,CV_8UC1);
	out_img.create(in_img.rows,in_img.cols,CV_8UC1); // create memory for output image
	diff.create(in_img.rows,in_img.cols,CV_8UC1);
#else
	ocv_ref.create(in_img.rows,in_img.cols,CV_8UC3);
	out_img.create(in_img.rows,in_img.cols,CV_8UC3); // create memory for output image
	diff.create(in_img.rows,in_img.cols,CV_8UC3);
#endif

	float sigma_color = rng.uniform(0.0,1.0)*255;
	float sigma_space = rng.uniform(0.0,1.0);

	std::cout << " sigma_color: " <<  sigma_color << " sigma_space: " << sigma_space << std::endl;

	// OpenCV bilateral filter function
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	cv::bilateralFilter(in_img, ocv_ref, FILTER_WIDTH, sigma_color, sigma_space, cv::BORDER_REPLICATE);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	cv::imwrite("output_ocv.png", ocv_ref);


	uint16_t width = in_img.cols;
	uint16_t height = in_img.rows;

	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> _src(height,width);
	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> _dst(height,width);


	_src.copyTo(in_img.data);

#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	bilateral_filter_accel(_src,_dst, sigma_color, sigma_space);

#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif

	// Write output image
	//xf::imwrite("hls_out.jpg",_dst);
	out_img.data = _dst.copyFrom();



	// Compute absolute difference image
	cv::absdiff(ocv_ref, out_img, diff);

	cv::imwrite("diff.jpg",diff);


	// 	Find minimum and maximum differences.

	double minval=256,maxval=0;
	int cnt = 0;
	float err=0;

	xf::analyzeDiff(diff,0,err);
	

	return 0;

}
