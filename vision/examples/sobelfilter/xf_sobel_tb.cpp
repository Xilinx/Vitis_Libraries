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

#include <stdio.h>
#include <stdlib.h>

#include "xf_headers.h"
#include "xf_sobel_config.h"


int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}
	char in[100], in1[100], out_hlsx[100], out_ocvx[100];
	char out_errorx[100], out_hlsy[100], out_ocvy[100], out_errory[100];

	cv::Mat in_img, in_gray, diff;
	cv::Mat c_grad_x_1, c_grad_y_1;
	cv::Mat c_grad_x, c_grad_y;
	cv::Mat hls_grad_x, hls_grad_y;
	cv::Mat diff_grad_x, diff_grad_y;

	// reading in the color image
#if GRAY
	in_img = cv::imread(argv[1], 0);
#else
	in_img = cv::imread(argv[1], 1);
#endif

	if(in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n", in);
		return 0;
	}
//	cvtColor(in_img, in_gray, CV_BGR2GRAY);

	///////////////// 	Opencv  Reference  ////////////////////////
	int scale = 1;
	int delta = 0;



#if (FILTER_WIDTH != 7)

	int ddepth = CV_8U;
	typedef unsigned char TYPE; // Should be short int when ddepth is CV_16S
#if GRAY
	#define PTYPE CV_8UC1			// Should be CV_16S when ddepth is CV_16S
#else
	#define PTYPE CV_8UC3		// Should be CV_16S when ddepth is CV_16S
#endif
#endif
#if (FILTER_WIDTH == 7)

	int ddepth = -1;//CV_32F;	//Should be CV_32F if the output pixel type is XF_32UC1
	typedef unsigned char TYPE; // Should be int when ddepth is CV_32F
#if GRAY
	#define PTYPE CV_8UC1			// Should be CV_16S when ddepth is CV_16S
#else
	#define PTYPE CV_8UC3		// Should be CV_16S when ddepth is CV_16S
#endif

#endif

	// create memory for output images
	hls_grad_x.create(in_img.rows,in_img.cols,PTYPE);
	hls_grad_y.create(in_img.rows,in_img.cols,PTYPE);
	diff_grad_x.create(in_img.rows,in_img.cols,PTYPE);
	diff_grad_y.create(in_img.rows,in_img.cols,PTYPE);



	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif


	cv::Sobel( in_img, c_grad_x_1, ddepth, 1, 0, FILTER_WIDTH, scale, delta, cv::BORDER_CONSTANT );
	cv::Sobel( in_img, c_grad_y_1, ddepth, 0, 1, FILTER_WIDTH, scale, delta, cv::BORDER_CONSTANT );

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	imwrite("out_ocvx.jpg", c_grad_x_1);
	imwrite("out_ocvy.jpg", c_grad_y_1);


	unsigned short height = in_img.rows;
	unsigned short width = in_img.cols;


	static xf::Mat<IN_TYPE,HEIGHT,WIDTH,NPC1> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<OUT_TYPE,HEIGHT,WIDTH,NPC1> imgOutputx(in_img.rows,in_img.cols);
	static xf::Mat<OUT_TYPE,HEIGHT,WIDTH,NPC1> imgOutputy(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);

	#if __SDSCC__
		perf_counter hw_ctr1;
hw_ctr1.start();
	#endif

	sobel_accel(imgInput, imgOutputx,imgOutputy);

	#if __SDSCC__
		hw_ctr1.stop();
uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif

	// Write output image
	xf::imwrite("hls_out_x.jpg",imgOutputx);
	xf::imwrite("hls_out_y.jpg",imgOutputy);

/*	hls_grad_x.data = (unsigned char *)imgOutputx.copyFrom();
	hls_grad_y.data = (unsigned char *)imgOutputy.copyFrom();

	imwrite("out_hlsx.jpg", hls_grad_x);
	imwrite("out_hlsy.jpg", hls_grad_y);
*/

	//////////////////  Compute Absolute Difference ////////////////////
#if (FILTER_WIDTH == 3 | FILTER_WIDTH == 5)
	xf::absDiff(c_grad_x_1, imgOutputx, diff_grad_x);
	xf::absDiff(c_grad_y_1, imgOutputy, diff_grad_y);
#endif

#if (FILTER_WIDTH == 7)
	if(OUT_TYPE == XF_8UC1){
//	absdiff(c_grad_x_1, hls_grad_x, diff_grad_x);
//	absdiff(c_grad_y_1, hls_grad_y, diff_grad_y);
	xf::absDiff(c_grad_x_1, imgOutputx, diff_grad_x);
	xf::absDiff(c_grad_y_1, imgOutputy, diff_grad_y);
	}
	else if (OUT_TYPE == XF_32UC1){
	c_grad_x_1.convertTo(c_grad_x, CV_32S);
	c_grad_y_1.convertTo(c_grad_y, CV_32S);
	xf::absDiff(c_grad_x, imgOutputx, diff_grad_x);
	xf::absDiff(c_grad_y, imgOutputy, diff_grad_y);
//	absdiff(c_grad_x, hls_grad_x, diff_grad_x);
//	absdiff(c_grad_y, hls_grad_y, diff_grad_y);
	}
#endif

	imwrite("out_errorx.jpg", diff_grad_x);
	imwrite("out_errory.jpg", diff_grad_y);

	float err_per,err_per1;
	int ret;

	xf::analyzeDiff(diff_grad_x, 0, err_per);
	xf::analyzeDiff(diff_grad_y, 0, err_per1);


	if(err_per > 0.0f)
	{
		printf("Test failed .... !!!\n");
		ret = 1;
	}else
	{
		printf("Test Passed .... !!!\n");
		ret = 0;
	}

	return ret;
}

