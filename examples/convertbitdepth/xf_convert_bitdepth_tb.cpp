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
#include "xf_convert_bitdepth_config.h"

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img;
	cv::Mat in_gray, input_img;//, ocv_ref;

	// reading in the color image
	in_img = cv::imread(argv[1], 0);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n", argv[1]);
		return 0;
	}

	//cvtColor(in_img,in_gray,CV_BGR2GRAY);

#if !(XF_CONVERT8UTO16S || XF_CONVERT8UTO16U || XF_CONVERT8UTO32S)
	in_img.convertTo(input_img, OCV_INTYPE);
#endif

	// create memory for output image
	cv::Mat ocv_ref(in_img.rows,in_img.cols, OCV_OUTTYPE);
	cv::Mat diff(in_img.rows,in_img.cols, OCV_OUTTYPE);
	unsigned short int height=in_img.rows;
	unsigned short int width=in_img.cols;

	///////////////// 	Opencv  Reference  ////////////////////////

#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif

#if !(XF_CONVERT8UTO16S || XF_CONVERT8UTO16U || XF_CONVERT8UTO32S)
	input_img.convertTo(ocv_ref,OCV_OUTTYPE);
#else
	in_img.convertTo(ocv_ref,OCV_OUTTYPE);
#endif

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	cv::imwrite("out_ocv.jpg", ocv_ref);
	//////////////////////////////////////////////////////////////

	ap_int<4> _convert_type = CONVERT_TYPE;
	int shift = 0;

	static xf::Mat<_SRC_T, HEIGHT, WIDTH, _NPC> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<_DST_T, HEIGHT, WIDTH, _NPC> imgOutput(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, _NPC> in_8bit(in_img.rows,in_img.cols);

	//in_8bit = xf::imread<XF_8UC1, HEIGHT, WIDTH, _NPC>(argv[1], 0);
	in_8bit.copyTo(in_img.data);


#if (XF_CONVERT16STO8U)
	//imgInput.copyTo((IN_TYPE *) input_img.data);
	in_8bit.convertTo(imgInput,XF_CONVERT_8U_TO_16S);
#elif(XF_CONVERT16UTO8U || XF_CONVERT16UTO32S || XF_CONVERT16STO32S)
	in_8bit.convertTo(imgInput,XF_CONVERT_8U_TO_16U);
#elif (XF_CONVERT32STO8U || XF_CONVERT32STO16U || XF_CONVERT32STO16S)
	in_8bit.convertTo(imgInput,XF_CONVERT_8U_TO_32S);
#else
	//imgInput.copyTo((IN_TYPE *)in_img.data);
	imgInput=in_8bit;
#endif

#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif
	convert_bitdepth_accel(imgInput, imgOutput, _convert_type, shift);
#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif

	xf::imwrite("hls_out.png",imgOutput);

	//////////////////  Compute Absolute Difference ////////////////////
	xf::absDiff(ocv_ref,imgOutput,diff);
	imwrite("out_err.png", diff);

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
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	if(err_per > 0.0f){
		printf("\nTest Failed\n");
		return -1;
	}

	return 0;
}
