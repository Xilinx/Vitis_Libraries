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
#include "xf_magnitude_config.h"


////////////    Reference for L1NORM    //////////
int ComputeMagnitude(cv::Mat gradx, cv::Mat grady, cv::Mat &dst)
{

	int row, col;
	int16_t gx, gy, tmp_res;
	int16_t tmp1, tmp2;
	int16_t res;
	for( row = 0; row < gradx.rows; row++ )
	{
		for ( col = 0; col < gradx.cols; col++ )
		{
			gx = gradx.at<int16_t>(row, col);
			gy = grady.at<int16_t>(row, col);
			tmp1 = abs(gx);
			tmp2 = abs(gy);
			tmp_res = tmp1 + tmp2;
			res = (int16_t)tmp_res;
			dst.at<int16_t>(row, col) = res;
		}
	}
	return 0;
}



int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray, c_grad_x, c_grad_y, c_grad_x1,
	c_grad_y1, ocv_ref1, ocv_ref2, ocv_res, out_img, diff;

	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	int filter_size = 3;

	/*  reading in the color image  */
	in_img = cv::imread(argv[1],0);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	/*  convert to gray  */
//	cvtColor(in_img,in_gray,CV_BGR2GRAY);

	cv::Sobel( in_img, c_grad_x, CV_16S, 1, 0, filter_size, scale,delta, cv::BORDER_CONSTANT );
	cv::Sobel( in_img, c_grad_y, CV_16S, 0, 1, filter_size, scale,delta, cv::BORDER_CONSTANT );

	ocv_ref1.create(in_img.rows,in_img.cols,CV_16S);
	out_img.create(in_img.rows,in_img.cols,CV_16S);
	diff.create(in_img.rows,in_img.cols,CV_16S);
	
	
	/////////////////    OpenCV reference  /////////////////
#if L1NORM
	#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	ComputeMagnitude(c_grad_x, c_grad_y, ocv_ref1);

	#if __SDSCC__
		hw_ctr1.stop();
	 uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif
#elif L2NORM
	cv::Sobel( in_img, c_grad_x1, CV_32FC1, 1, 0, filter_size,scale, delta, cv::BORDER_CONSTANT );
	Sobel( in_img, c_grad_y1, CV_32FC1, 0, 1, filter_size,scale, delta, cv::BORDER_CONSTANT );

	#if __SDSCC__
		perf_counter hw_ctr2;
	hw_ctr2.start();
	#endif

	magnitude(c_grad_x1, c_grad_y1, ocv_ref2);

	#if __SDSCC__
		hw_ctr2.stop();
		uint64_t hw_cycles2 = hw_ctr2.avg_cpu_cycles();
	#endif
#endif


	uint16_t width = in_img.cols;
	uint16_t height = in_img.rows;




	static xf::Mat<XF_16SC1,HEIGHT,WIDTH,NPC1> imgInputx(in_img.rows,in_img.cols);
	static xf::Mat<XF_16SC1,HEIGHT,WIDTH,NPC1> imgInputy(in_img.rows,in_img.cols);
	static xf::Mat<XF_16SC1,HEIGHT,WIDTH,NPC1> imgOutput(in_img.rows,in_img.cols);

	imgInputx.copyTo(( short int*)c_grad_x.data);
	imgInputy.copyTo(( short int*)c_grad_y.data);

	

	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif
	

	magnitude_accel(imgInputx, imgInputy,imgOutput);
	
	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif


	out_img.data=imgOutput.copyFrom();

	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);

	




#if L1NORM
	imwrite("ref_img.jpg",ocv_ref1);          // save the reference image
	xf::absDiff(ocv_ref1,imgOutput,diff);    // Compute absolute difference image
	imwrite("diff_img.jpg",diff);            // Save the difference image for debugging purpose
#elif L2NORM
	ocv_ref2.convertTo(ocv_res,CV_16S);  //  convert from 32F type to 16S type for finding the AbsDiff
	imwrite("ref_img.jpg",ocv_res);          // save the reference image
	absdiff(ocv_res,out_img,diff);    // Compute absolute difference image
	imwrite("diff_img.jpg",diff);            // Save the difference image for debugging purpose
#endif

	// Find minimum and maximum differences

	double minval=256,maxval=0;
	int cnt = 0;
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j = 0; j < in_img.cols; j++)
		{
			uchar v = diff.at<short int>(i,j);

			if (v > 1)
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


