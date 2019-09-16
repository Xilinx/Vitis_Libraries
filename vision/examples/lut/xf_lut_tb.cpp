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
#include "xf_lut_config.h"


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	/* gamma correction LUT for example */
	uchar_t lut[256]={0,16,23,28,32,36,39,42,45,48,50,53,55,58,60,62,64,66,68,70,71,73,75,77,78,80,81,83,84,86,87,89,90,92,93,94,96,97,98,100,101,102,103,105,106,107,108,109,111,112,113,114,115,116,117,118,119,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,135,136,137,138,139,140,141,142,143,144,145,145,146,147,148,149,150,151,151,152,153,154,155,156,156,157,158,159,160,160,161,162,163,164,164,165,166,167,167,168,169,170,170,171,172,173,173,174,175,176,176,177,178,179,179,180,181,181,182,183,183,184,185,186,186,187,188,188,189,190,190,191,192,192,193,194,194,195,196,196,197,198,198,199,199,200,201,201,202,203,203,204,204,205,206,206,207,208,208,209,209,210,211,211,212,212,213,214,214,215,215,216,217,217,218,218,219,220,220,221,221,222,222,223,224,224,225,225,226,226,227,228,228,229,229,230,230,231,231,232,233,233,234,234,235,235,236,236,237,237,238,238,239,240,240,241,241,242,242,243,243,244,244,245,245,246,246,247,247,248,248,249,249,250,250,251,251,252,252,253,253,254,254,255};

	cv::Mat in_img, in_gray, out_img, ocv_ref, diff, lut_mat;

	/*  reading in the color image  */
#if GRAY
	in_img = cv::imread(argv[1],0);
#else
	in_img = cv::imread(argv[1],1);
#endif

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}
#if __SDSCC__
	uchar_t*lut_ptr=(uchar_t*)sds_alloc_non_cacheable(256 * sizeof(uchar_t));
#else
	uchar_t*lut_ptr=(uchar_t*)malloc(256*sizeof(uchar_t));
#endif

	for(int i=0;i<256;i++)
	{
		lut_ptr[i]=lut[i];
	}

#if GRAY
	out_img.create(in_img.rows,in_img.cols,CV_8UC1);
	ocv_ref.create(in_img.rows,in_img.cols,CV_8UC1);
	diff.create(in_img.rows,in_img.cols,CV_8UC1);
#else
	out_img.create(in_img.rows,in_img.cols,CV_8UC3);
	ocv_ref.create(in_img.rows,in_img.cols,CV_8UC3);
	diff.create(in_img.rows,in_img.cols,CV_8UC3);
#endif

	///////////// OpenCV reference  /////////////
	lut_mat = cv::Mat(1,256,CV_8UC1,lut);

	#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	cv::LUT(in_img,lut_mat,ocv_ref);

	#if __SDSCC__
		hw_ctr1.stop();
	 uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif

	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);

	#if __SDSCC__
		perf_counter hw_ctr; hw_ctr.start();
	#endif

	lut_accel(imgInput,imgOutput,lut_ptr);

	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);


	

	imwrite("ref_img.jpg",ocv_ref);     // save the reference image
	//////////////// Comparison /////////////
	xf::absDiff(ocv_ref,imgOutput, diff);
	imwrite("diff_img.jpg",diff);            // Save the difference image for debugging purpose

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0;i<in_img.rows;i++)
	{
		for(int j=0;j<in_img.cols;j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v>0)
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
