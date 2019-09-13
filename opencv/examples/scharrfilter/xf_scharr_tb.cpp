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
#include "xf_scharr_config.h"


int main(int argc, char** argv)
{

	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray;
	cv::Mat c_grad_x, c_grad_y;
	cv::Mat hls_grad_x, hls_grad_y;
	cv::Mat diff_grad_x, diff_grad_y;

	// reading in the gray image
#if GRAY
	in_img = cv::imread(argv[1],0);
#else
	in_img = cv::imread(argv[1],1);
#endif

#if GRAY
	#define PTYPE CV_8UC1			// Should be CV_16S when ddepth is CV_16S
#else
	#define PTYPE CV_8UC3		// Should be CV_16S when ddepth is CV_16S
#endif


	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}



	typedef unsigned char TYPE; //short int TYPE; //

	// create memory for output images
	c_grad_x.create(in_img.rows,in_img.cols,PTYPE);
	c_grad_y.create(in_img.rows,in_img.cols,PTYPE);
	hls_grad_x.create(in_img.rows,in_img.cols,PTYPE);
	hls_grad_y.create(in_img.rows,in_img.cols,PTYPE);
	diff_grad_x.create(in_img.rows,in_img.cols,PTYPE);
	diff_grad_y.create(in_img.rows,in_img.cols,PTYPE);

	////////////    Opencv Reference    //////////////////////
	int scale = 1;
	int delta = 0;
	int ddepth =-1;//CV_16S;//

	

	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif
		Scharr(in_img, c_grad_x, ddepth, 1, 0, scale, delta, cv::BORDER_CONSTANT );
		Scharr(in_img, c_grad_y, ddepth, 0, 1, scale, delta, cv::BORDER_CONSTANT );

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	imwrite("out_ocvx.jpg", c_grad_x);
	imwrite("out_ocvy.jpg", c_grad_y);

	//////////////////	HLS TOP Function Call  ////////////////////////


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

	scharr_accel(imgInput, imgOutputx,imgOutputy);

	#if __SDSCC__
		hw_ctr1.stop();
uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif



	xf::imwrite("hls_out_x.jpg",imgOutputx);
	xf::imwrite("hls_out_y.jpg",imgOutputy);


	imwrite("out_hlsx.jpg", hls_grad_x);
	imwrite("out_hlsy.jpg", hls_grad_y);


	//////////////////  Compute Absolute Difference ////////////////////

	xf::absDiff(c_grad_x, imgOutputx, diff_grad_x);
	xf::absDiff(c_grad_y, imgOutputy, diff_grad_y);

	imwrite("out_errorx.jpg", diff_grad_x);
	imwrite("out_errory.jpg", diff_grad_y);

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	double minval1=256,maxval1=0;
	int cnt = 0, cnt1 =0;
	float err_per,err_per1;
	xf::analyzeDiff(diff_grad_x, 0, err_per);
	xf::analyzeDiff(diff_grad_y, 0, err_per1);

/*	for(int i=0;i<in_img.rows;i++)
	{
		for(int j=0;j<in_img.cols;j++)
		{
			TYPE v = diff_grad_y.at<TYPE>(i,j);
			TYPE v1 = diff_grad_x.at<TYPE>(i,j);
			if (v>0)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
			if (v1>0)
				cnt1++;
			if (minval1 > v1 )
				minval1 = v1;
			if (maxval1 < v1)
				maxval1 = v1;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	float err_per1 = 100.0*(float)cnt1/(in_img.rows*in_img.cols);

	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval,maxval,err_per);
	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval1,maxval1,err_per1);
*/
	int ret  = 0;
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


