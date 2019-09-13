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
#include "xf_channel_combine_config.h"


int main(int argc, char **argv)
{
#if FOUR_INPUT
	if(argc != 5)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image1 path> <input image2 path> <input image3 path> <input image4 path>\n");
		return -1;
	}
#endif
#if THREE_INPUT
	if(argc != 4)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image1 path> <input image2 path> <input image3 path> \n");
		return -1;
	}
#endif
#if TWO_INPUT
	if(argc != 3)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image1 path> <input image2 path> \n");
		return -1;
	}
#endif
	cv::Mat in_gray1, in_gray2;
	cv::Mat in_gray3, in_gray4;
	cv::Mat out_img, ref;
	cv::Mat diff;

	// Read images
	in_gray1 = cv::imread(argv[1], 0);
	in_gray2 = cv::imread(argv[2], 0);

	if ((in_gray1.data == NULL) | (in_gray2.data == NULL))
	{
		fprintf(stderr, "Cannot open input images \n");
		return 0;
	}

#if !TWO_INPUT
	in_gray3 = cv::imread(argv[3], 0);
	if ((in_gray3.data == NULL))
		{
			fprintf(stderr, "Cannot open input images \n");
			return 0;
		}

	static xf::Mat<IN_TYPE, HEIGHT, WIDTH, NPC> imgInput3(in_gray3.rows,in_gray3.cols);

	//creating memory for diff image
	diff.create(in_gray1.rows, in_gray1.cols, CV_TYPE);
#endif

#if FOUR_INPUT

	in_gray4 = cv::imread(argv[4], 0);

	if ((in_gray4.data == NULL))
	{
		fprintf(stderr, "Cannot open image 4\n");
		return 0;
	}
	static xf::Mat<IN_TYPE, HEIGHT, WIDTH, NPC> imgInput4(in_gray4.rows,in_gray4.cols);

#endif


	// image height and width
	uint16_t height = in_gray1.rows;
	uint16_t width = in_gray1.cols;

	//OpenCV Reference
        std::vector<cv::Mat> bgr_planes;
        cv::Mat merged;

#if (!TWO_INPUT)
        bgr_planes.push_back(in_gray3);
#endif
        bgr_planes.push_back(in_gray2);
        bgr_planes.push_back(in_gray1);

#if FOUR_INPUT
        bgr_planes.push_back(in_gray4);
#endif

#if __SDSCC__
        perf_counter hw_ctr1;
        hw_ctr1.start();
#endif
        cv::merge(bgr_planes, merged);
#if __SDSCC__
        hw_ctr1.stop();
        uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif

	static xf::Mat<IN_TYPE, HEIGHT, WIDTH, NPC> imgInput1(in_gray1.rows,in_gray1.cols);
	static xf::Mat<IN_TYPE, HEIGHT, WIDTH, NPC> imgInput2(in_gray2.rows,in_gray2.cols);
	static xf::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC> imgOutput(in_gray1.rows,in_gray1.cols);

	imgInput1.copyTo(in_gray1.data);
	imgInput2.copyTo(in_gray2.data);

#if (!TWO_INPUT)
	imgInput3.copyTo(in_gray3.data);
#endif
#if FOUR_INPUT
	imgInput4.copyTo(in_gray4.data);
#endif


#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif

#if FOUR_INPUT
		channel_combine_accel(imgInput1, imgInput2, imgInput3, imgInput4, imgOutput);
#elif THREE_INPUT
		channel_combine_accel(imgInput1, imgInput2, imgInput3, imgOutput);
#else
		channel_combine_accel(imgInput1, imgInput2, imgOutput);
#endif

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	// Write output image
#if !TWO_INPUT
	xf::imwrite("hls_out.jpg",imgOutput);
	cv::imwrite("out_ocv.jpg", merged);

	// compute the absolute difference
	xf::absDiff(merged, imgOutput, diff);

	// write the difference image
	cv::imwrite("diff.jpg", diff);

	// Find minimum and maximum differences.
	double minval = 256, maxval = 0;
	float err_per = 0;
	xf::analyzeDiff(diff, 0, err_per);

	if(err_per>0)
		return 1;
#endif

	return 0;
}

