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
#include "xf_channel_extract_config.h"



int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <Input image>\n");
		return -1;
	}

	cv::Mat in_src, in_rgba, out_img;
	// read image
	in_src = cv::imread(argv[1], 1);

	if (in_src.data == NULL)
	{
		fprintf(stderr,"Cannot open image \n");
		return 0;
	}

	out_img.create(in_src.rows, in_src.cols, CV_8U);
	cv::cvtColor(in_src, in_rgba, CV_BGR2RGBA);

	// image height and width
	unsigned short int height = in_rgba.rows;
	unsigned short int width = in_rgba.cols;

	// Channel to be extracted < see au_channel_extract_e >
	uint16_t channel = XF_EXTRACT_CH_R;

	std::vector<cv::Mat> bgr_planes;
        // call OpenCV function
        #if __SDSCC__
                 perf_counter hw_ctr1;
                 hw_ctr1.start();
        #endif
                 cv::split( in_src, bgr_planes );
        #if __SDSCC__
                 hw_ctr1.stop();
                 uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
        #endif
                                                 

	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> imgInput(in_rgba.rows,in_rgba.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgOutput(in_rgba.rows,in_rgba.cols);

	imgInput.copyTo(in_rgba.data);

	 #if __SDSCC__
	perf_counter hw_ctr;   
	hw_ctr.start();
    	#endif

		channel_extract_accel(imgInput, imgOutput, channel);		

   	 #if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	 #endif

	xf::imwrite("hls_out.png",imgOutput);


	// write output and OpenCV reference image
	cv::imwrite("out_ocv.png",bgr_planes[2]);

	cv::Mat diff;
	diff.create(in_src.rows, in_src.cols, CV_8U);

	// Check with the correct channel. Keep 2 for R, 1 for G and 0 for B in index of bgr_planes
	xf::absDiff(bgr_planes[2], imgOutput, diff);
	cv::imwrite("diff.jpg", diff);

	// Find minimum and maximum differences.
	double minval = 256, maxval = 0;
	int cnt = 0;
	for (int i = 0; i < diff.rows; i++)
	{
		for(int j = 0; j < diff.cols; j++)
		{
			unsigned char v = diff.at<unsigned char>(i,j);
			if (v > 0) cnt++;
			if (minval > v ) minval = v;
			if (maxval < v)  maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_src.rows * in_src.cols);

	fprintf(stderr,"Minimum error in intensity = %f\n"
			"Maximum error in intensity = %f\n"
			"Percentage of pixels above error threshold = %f\n",
			minval, maxval, err_per);

	if(err_per > 0.0f)
		return -1;

	return 0;
}
