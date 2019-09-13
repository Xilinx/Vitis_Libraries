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
#include "xf_integral_image_config.h"


int main(int argc, char** argv)
{
	cv::Mat in_img, in_img1, out_img, ocv_ref, ocv_ref1;
	cv::Mat in_gray, in_gray1, diff;

	if(argc != 2)
	{
		fprintf(stderr, "Usage: <executable> <input image>\n");
		return -1;
	}	

	// Read input image
	in_img = cv::imread(argv[1], 0);
	if (in_img.data == NULL)
	{
		//cout << "Can't open image !!" << endl;
		return -1;
	}


	// create memory for output images
	ocv_ref.create(in_img.rows,in_img.cols,CV_32S);
	ocv_ref1.create(in_img.rows,in_img.cols,CV_32S);
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	cv::integral(in_img, ocv_ref, -1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	for(int i = 0; i < in_img.rows; i++)
	{
		for(int j=0; j<in_img.cols; j++)
		{
			ocv_ref1.at<unsigned int>(i,j) =  ocv_ref.at<unsigned int>(i+1, j+1);
		}
	}

	imwrite("out_ocv.png",ocv_ref1);


	// create memory for output image
	diff.create(in_img.rows,in_img.cols,CV_32S);
	out_img.create(in_img.rows,in_img.cols,CV_32S);

	// image height and width
	uint16_t height = in_img.rows;
	uint16_t width = in_img.cols;

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<XF_32UC1, HEIGHT, WIDTH, XF_NPPC1> imgOutput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);
	

	#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	integral_accel(imgInput, imgOutput);

	#if __SDSCC__
	hw_ctr1.stop();
	 uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif

	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);

	// Compute absolute difference image
	xf::absDiff(ocv_ref1, imgOutput, diff);

	// Save the difference image 
	imwrite("diff.png", diff); 

	float err_per;
		double minval=256,maxval=0;
	int cnt = 0;
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j = 0; j < in_img.cols; j++)
		{
			unsigned int v = diff.at<unsigned int>(i,j);

			if (v > 0)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}

	}

	err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;

}
