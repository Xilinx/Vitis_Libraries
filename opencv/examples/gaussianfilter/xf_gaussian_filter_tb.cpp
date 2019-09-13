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
#include "xf_gaussian_filter_config.h"


using namespace std;


int main(int argc, char **argv) {

	if (argc != 2)
	{
		printf("Usage: <executable> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, out_img, ocv_ref, in_img_gau;
	cv::Mat in_gray, in_gray1, diff;


#if GRAY
	in_img = cv::imread(argv[1], 0); // reading in the color image
#else
	in_img = cv::imread(argv[1], 1); // reading in the color image
#endif
	if (!in_img.data) {
		printf("Failed to load the image ... !!!");
		return -1;
	}
	//extractChannel(in_img, in_img, 1);
#if GRAY

	out_img.create(in_img.rows, in_img.cols, CV_8UC1); // create memory for output image
	diff.create(in_img.rows, in_img.cols, CV_8UC1); // create memory for OCV-ref image
	ocv_ref.create(in_img.rows, in_img.cols, CV_8UC1); // create memory for OCV-ref image
	
#else
	out_img.create(in_img.rows, in_img.cols, CV_8UC3); // create memory for output image
	diff.create(in_img.rows, in_img.cols, CV_8UC3); // create memory for OCV-ref image
	ocv_ref.create(in_img.rows, in_img.cols, CV_8UC3); // create memory for OCV-ref image
#endif

#if FILTER_WIDTH==3
	float sigma = 0.5f;
#endif
#if FILTER_WIDTH==7
	float sigma=1.16666f;
#endif
#if FILTER_WIDTH==5
	float sigma = 0.8333f;
#endif


	// OpenCV Gaussian filter function
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	cv::GaussianBlur(in_img, ocv_ref, cvSize(FILTER_WIDTH, FILTER_WIDTH),FILTER_WIDTH / 6.0, FILTER_WIDTH / 6.0, cv::BORDER_CONSTANT);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	imwrite("output_ocv.png", ocv_ref);


	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);
#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	gaussian_filter_accel(imgInput,imgOutput,sigma);

#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif
	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);

	// Compute absolute difference image
	xf::absDiff(ocv_ref, imgOutput, diff);

	imwrite("error.png", diff); // Save the difference image for debugging purpose

	float err_per;
	xf::analyzeDiff(diff, 0, err_per);

	
	return 0;

}
