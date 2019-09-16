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
#include "xf_accumulate_weighted_config.h"

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path1> <input image path2> \n");
		return -1;
	}

	cv::Mat in_img, in_img1, out_img;
	cv::Mat in_gray, in_gray1;
#if GRAY
	in_gray  = cv::imread(argv[1], 0); // read image
	in_gray1 = cv::imread(argv[2], 0); // read image
#else
	in_gray  = cv::imread(argv[1], 1); // read image
	in_gray1 = cv::imread(argv[2], 1); // read image
#endif
	if (in_gray.data == NULL)
	{
		fprintf(stderr, "Cannot open image %s\n", argv[1]);
		return -1;
	}
	if (in_gray1.data == NULL)
	{
		fprintf(stderr, "Cannot open image %s\n", argv[2]);
		return -1;
	}

#if GRAY

	cv::Mat ocv_in(in_gray.rows, in_gray.cols, CV_32FC1, 1);
	cv::Mat ocv_inout(in_gray.rows, in_gray.cols, CV_32FC1, 1);
	cv::Mat ocv_out_16bit(in_gray.rows, in_gray.cols, CV_16UC1, 1);
	cv::Mat diff(in_gray.rows, in_gray.cols, CV_16UC1, 1);

	in_gray.convertTo(ocv_in, CV_32FC1);
	in_gray1.convertTo(ocv_inout, CV_32FC1);
#else

	cv::Mat ocv_in(in_gray.rows, in_gray.cols, CV_32FC3);
	cv::Mat ocv_inout(in_gray.rows, in_gray.cols, CV_32FC3);
	cv::Mat ocv_out_16bit(in_gray.rows, in_gray.cols, CV_16UC3);
	cv::Mat diff(in_gray.rows, in_gray.cols, CV_16UC3, 1);
	in_gray.convertTo(ocv_in, CV_32FC3);
	in_gray1.convertTo(ocv_inout, CV_32FC3);
#endif
	// Weight ( 0 to 1 )
	float alpha = 0.76;	

	// OpenCV function
	cv::accumulateWeighted(ocv_in, ocv_inout, alpha, cv::noArray());
#if GRAY
	ocv_inout.convertTo(ocv_out_16bit, CV_16UC1);
#else
	ocv_inout.convertTo(ocv_out_16bit, CV_16UC3);
#endif
	// Write OpenCV reference image
	cv::imwrite("out_ocv.jpg", ocv_out_16bit);


	xf::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray1.rows,in_gray1.cols);
	xf::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput2(in_gray.rows,in_gray.cols);
	xf::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray.rows,in_gray.cols);

	imgInput1.copyTo(in_gray.data);
	imgInput2.copyTo(in_gray1.data);

#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	accumulate_weighted_accel(imgInput1,imgInput2,imgOutput,alpha);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	xf::imwrite("out_hls.jpg", imgOutput);

	xf::absDiff(ocv_out_16bit, imgOutput, diff);
	// Save the difference image
	cv::imwrite("diff.jpg", diff);
	int err_thresh;float err_per;
	xf::analyzeDiff(diff, err_thresh, err_per);

	if(err_per > 0.0f)
		return -1;
	return 0;
}

