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
#include "xf_dilation_config.h"

int main(int argc, char** argv)
{

	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat out_img,ocv_ref;
	cv::Mat in_img,in_img1,diff;

	// reading in the color image
	in_img = cv::imread(argv[1], 0);
	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n", argv[1]);
		return 0;
	}
	// create memory for output images
	ocv_ref.create(in_img.rows,in_img.cols,CV_8UC1);
	diff.create(in_img.rows,in_img.cols,CV_8UC1);
	in_img1.create(in_img.rows,in_img.cols,CV_8UC1);

	uint16_t height = in_img.rows;
	uint16_t width = in_img.cols;



	///////////////// 	Opencv  Reference  ////////////////////////
	cv::Mat element = cv::getStructuringElement( 0,cv::Size(3, 3), cv::Point(-1, -1));
	cv::dilate(in_img, ocv_ref, element, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT);
	cv::imwrite("out_ocv.jpg", ocv_ref);

	#if __SDSCC__
	unsigned char *structure_element=(unsigned char *)sds_alloc_non_cacheable(sizeof(unsigned char)*FILTER_SIZE*FILTER_SIZE);
	#else
	unsigned char structure_element[FILTER_SIZE*FILTER_SIZE];
	#endif


	for(int i=0;i<(FILTER_SIZE*FILTER_SIZE);i++)
	{
		structure_element[i]=element.data[i];
	}

	hls::stream< ap_axiu<8,1,1,1> > _src,_dst;

	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	cvMat2AXIvideoxf<NPC1>(in_img, _src);

	ip_accel_app(_src, _dst,height,width, structure_element);

	AXIvideo2cvMatxf<NPC1>(_dst, in_img1);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	cv::imwrite("hls.jpg", in_img1);
	//////////////////  Compute Absolute Difference ////////////////////

	cv::absdiff(ocv_ref, in_img1, diff);
	cv::imwrite("out_error.jpg", diff);

	// Find minimum and maximum differences.
		double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0; i<in_img.rows; i++)
	{
		for(int j=0; j<in_img.cols; j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v>0)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}

	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	if(err_per > 0.0f)
		return 1;


	return 0;

}
