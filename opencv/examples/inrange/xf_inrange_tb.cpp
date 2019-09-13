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
#include "xf_inrange_config.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, out_img, ocv_ref, in_gray, diff;

	unsigned short in_width,in_height;
#if GRAY
	/*  reading in the color image  */
	in_img = cv::imread(argv[1],0);
#else
	/*  reading in the color image  */
	in_img = cv::imread(argv[1],1);
#endif
	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	in_width = in_img.cols;
	in_height = in_img.rows;
#if GRAY
	ocv_ref.create(in_img.rows,in_img.cols,in_img.depth());
	out_img.create(in_img.rows,in_img.cols,in_img.depth());
	diff.create(in_img.rows,in_img.cols,in_img.depth());
#else
	ocv_ref.create(in_img.rows,in_img.cols,CV_8UC1);
	out_img.create(in_img.rows,in_img.cols,CV_8UC3);
	diff.create(in_img.rows,in_img.cols,CV_8UC3);
#endif




	////////////////  reference code  ////////////////
#if __SDSCC___
	unsigned char *lower_thresh=(unsigned char *)sds_alloc_non_cacheable(XF_CHANNELS(IN_TYPE,NPIX)*sizeof(unsigned char));
	unsigned char *upper_thresh=(unsigned char *)sds_alloc_non_cacheable(XF_CHANNELS(IN_TYPE,NPIX)*sizeof(unsigned char));
#else
	unsigned char *lower_thresh=(unsigned char *)malloc(XF_CHANNELS(IN_TYPE,NPIX)*sizeof(unsigned char));
	unsigned char *upper_thresh=(unsigned char *)malloc(XF_CHANNELS(IN_TYPE,NPIX)*sizeof(unsigned char));
#endif
#if GRAY
	 lower_thresh[0]=50;
	 upper_thresh[0]=100;
#else
	 lower_thresh[0]=50;
	 upper_thresh[0]=100;
	 lower_thresh[1]=0;
	 upper_thresh[1]=150;
	 lower_thresh[2]=50;
	 upper_thresh[2]=150;
#endif
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
#if GRAY
	cv::inRange(in_img,lower_thresh[0],upper_thresh[0], ocv_ref);
#else
	cv::inRange(in_img,cv::Scalar(lower_thresh[0],lower_thresh[1],lower_thresh[2]),cv::Scalar( upper_thresh[0], upper_thresh[1], upper_thresh[2]), ocv_ref);
#endif
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
   //////////////////  end opencv reference code//////////

	////////////////////// HLS TOP function call ////////////////////////////

	static xf::Mat<IN_TYPE, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<OUT_TYPE, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows,in_img.cols);
	
	imgInput.copyTo(in_img.data);

	#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	inrange_accel(imgInput,lower_thresh,upper_thresh, imgOutput);

	#if __SDSCC__
	hw_ctr1.stop();
	 uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif



	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);
	cv::imwrite("ref_img.jpg", ocv_ref);  // reference image


	xf::absDiff(ocv_ref, imgOutput, diff);
	imwrite("diff_img.jpg",diff); // Save the difference image for debugging purpose

	// Find minimum and maximum differences.
	double minval = 256, maxval1 = 0;
	int cnt = 0;
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j = 0; j < in_img.cols;j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v > 1)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval1 < v)
				maxval1 = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval1,err_per);


	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
