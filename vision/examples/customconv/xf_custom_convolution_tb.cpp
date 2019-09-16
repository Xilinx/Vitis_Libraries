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
#include "xf_custom_convolution_config.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, out_img, ocv_ref, diff, filter;


#if GRAY
	in_img = cv::imread(argv[1],0); // reading in the gray image
#else
	in_img = cv::imread(argv[1],1); // reading in the gray image
#endif


	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}


	unsigned char shift = SHIFT;

	//////////////////  Creating the kernel ////////////////
	filter.create(FILTER_HEIGHT,FILTER_WIDTH,CV_32F);

	/////////	Filling the Filter coefficients  /////////
	for(int i = 0; i < FILTER_HEIGHT; i++)
	{
		for(int j = 0; j < FILTER_WIDTH; j++)
		{
			filter.at<float>(i,j) = (float)0.1111;
		}
	}


	/////////////////    OpenCV reference   /////////////////
#if GRAY
#if   OUT_8U
	out_img.create(in_img.rows,in_img.cols,CV_8UC1); // create memory for output image
	diff.create(in_img.rows,in_img.cols,CV_8UC1);    // create memory for difference image
#elif OUT_16S
	out_img.create(in_img.rows,in_img.cols,CV_16SC1); // create memory for output image
	diff.create(in_img.rows,in_img.cols,CV_16SC1);	  // create memory for difference image
#endif
#else
#if   OUT_8U
	out_img.create(in_img.rows,in_img.cols,CV_8UC3); // create memory for output image
	diff.create(in_img.rows,in_img.cols,CV_8UC3);    // create memory for difference image
#elif OUT_16S
	out_img.create(in_img.rows,in_img.cols,CV_16SC3); // create memory for output image
	diff.create(in_img.rows,in_img.cols,CV_16SC3);	  // create memory for difference image
#endif
#endif

	cv::Point anchor = cv::Point( -1, -1 );

#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
#if OUT_8U
	cv::filter2D(in_img, ocv_ref, CV_8U , filter, anchor, 0, cv::BORDER_CONSTANT);
#elif OUT_16S
	cv::filter2D(in_img, ocv_ref, CV_16S ,filter, anchor, 0, cv::BORDER_CONSTANT );
#endif
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	imwrite("ref_img.jpg", ocv_ref);  // reference image

#if __SDSCC__
	short int *filter_ptr=(short int*)sds_alloc_non_cacheable(FILTER_WIDTH*FILTER_HEIGHT*sizeof(short int));
#else
	short int *filter_ptr=(short int*)malloc(FILTER_WIDTH*FILTER_HEIGHT*sizeof(short int));
#endif
	for(int i = 0; i < FILTER_HEIGHT; i++)
	{
		for(int j = 0; j < FILTER_WIDTH; j++)
		{
			filter_ptr[i*FILTER_WIDTH+j] = 3640;
		}
	}


	static xf::Mat<INTYPE, HEIGHT, WIDTH, NPC_T> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<OUTTYPE,HEIGHT,WIDTH,NPC_T> imgOutput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);


#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	Filter2d_accel(imgInput,imgOutput,filter_ptr,shift);

#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif

	xf::imwrite("out_img.jpg", imgOutput);
	xf::absDiff(ocv_ref,imgOutput,diff);    // Compute absolute difference image

	float err_per;
	xf::analyzeDiff(diff,2,err_per);
	cv::imwrite("diff_img.jpg",diff);

	ocv_ref.~Mat();
	diff.~Mat();
	in_img.~Mat();
	out_img.~Mat();

	if(err_per > 0.0f)
		return 1;

	return 0;
}

