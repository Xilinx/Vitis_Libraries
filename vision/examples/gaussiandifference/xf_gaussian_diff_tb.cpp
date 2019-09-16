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
#include "xf_gaussian_diff_config.h"


using namespace std;


int main(int argc, char **argv) {

	if (argc != 2)
	{
		printf("Usage: <executable> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, out_img, ocv_ref, in_img_gau;
	cv::Mat in_gray, in_gray1, diff;

	in_img = cv::imread(argv[1], 1); // reading in the color image
	if (!in_img.data) {
		printf("Failed to load the image ... !!!");
		return -1;
	}

	extractChannel(in_img, in_gray, 1);

	ocv_ref.create(in_gray.rows, in_gray.cols, in_gray.depth()); // create memory for output image
#if FILTER_WIDTH==3
	float sigma = 0.5f;
#endif
#if FILTER_WIDTH==7
	float sigma=1.16666f;
#endif
#if FILTER_WIDTH==5
	float sigma = 0.8333f;
#endif

	out_img.create(in_gray.rows, in_gray.cols, in_gray.depth()); // create memory for output image


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput(in_gray.rows,in_gray.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat1(in_gray.rows,in_gray.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat2(in_gray.rows,in_gray.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat3(in_gray.rows,in_gray.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat4(in_gray.rows,in_gray.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat5(in_gray.rows,in_gray.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput(in_gray.rows,in_gray.cols);

	imgInput.copyTo(in_gray.data);
	

	#if __SDSCC__
	perf_counter hw_ctr;
	 hw_ctr.start();
	#endif

	gaussian_diff_accel(imgInput,imgmat1,imgmat2,imgmat3,imgmat4,imgmat5,imgOutput,sigma);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif


	FILE *fp = fopen("gaussdiff.txt","w");
	out_img.data = imgOutput.copyFrom();

	for (int i = 0; i < out_img.rows; i++) {
			for (int j = 0; j < out_img.cols; j++) {
				uchar v = out_img.at<uchar>(i, j);
				fprintf(fp,"%d ",v);
			}
			fprintf(fp,"\n");
	}
	fclose(fp);



	imwrite("output_hls.png", out_img);


}
