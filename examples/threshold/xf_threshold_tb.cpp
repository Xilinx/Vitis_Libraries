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
#include "xf_threshold_config.h"

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

	/*  reading in the color image  */
	in_img = cv::imread(argv[1],0);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	in_width = in_img.cols;
	in_height = in_img.rows;

	ocv_ref.create(in_img.rows,in_img.cols,in_img.depth());
	out_img.create(in_img.rows,in_img.cols,in_img.depth());
	diff.create(in_img.rows,in_img.cols,in_img.depth());





	////////////////  reference code  ////////////////

	 short int maxval=50;
	 short int thresh=100;




	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	cv::threshold(in_img,ocv_ref,thresh,maxval,THRESH_TYPE);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

   //////////////////  end opencv reference code//////////

	////////////////////// HLS TOP function call ////////////////////////////

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows,in_img.cols);
	imgInput.copyTo(in_img.data);

	#if __SDSCC__
		perf_counter hw_ctr1;
hw_ctr1.start();
	#endif

	threshold_accel(imgInput, imgOutput, thresh, maxval);

	#if __SDSCC__
		hw_ctr1.stop();
uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif



	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);
	cv::imwrite("ref_img.jpg", ocv_ref);  // reference image


	xf::absDiff(ocv_ref, imgOutput, diff);
	imwrite("diff_img.jpg",diff); // Save the difference image for debugging purpose

	float err_per;
	xf::analyzeDiff(diff, 0, err_per);

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
