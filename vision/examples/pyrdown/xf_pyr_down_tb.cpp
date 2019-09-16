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
#include "xf_pyr_down_config.h"
int main(int argc, char *argv[]){

	cv::Mat input_image, output_image, output_xf, output_diff_xf_cv;

#if GRAY
	input_image = cv::imread(argv[1],0);
#else
	input_image = cv::imread(argv[1],1);
#endif

	unsigned short input_height = input_image.rows;
	unsigned short input_width = input_image.cols;

	unsigned short output_height = (input_image.rows + 1) >> 1;
	unsigned short output_width = (input_image.cols + 1) >> 1;
#if GRAY
	output_xf.create(output_height,output_width,CV_8UC1);
	output_diff_xf_cv.create(output_height,output_width,CV_8UC1);
	
#else
	output_xf.create(output_height,output_width,CV_8UC3);
	output_diff_xf_cv.create(output_height,output_width,CV_8UC3);
#endif

	std::cout << "Input Height " << input_height << " Input_Width " << input_width << std::endl;
	std::cout << "Output Height " << output_height << " Output_Width " << output_width << std::endl;
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif

	cv::pyrDown(input_image, output_image, cv::Size(output_width,output_height),cv::BORDER_REPLICATE);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	cv::imwrite("opencv_image.png",output_image);


	static xf::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> imgInput(input_image.rows,input_image.cols);
	static xf::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> imgOutput(output_height,output_width);

	imgInput.copyTo(input_image.data);


#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif

	pyr_down_accel(imgInput, imgOutput);

#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif


	xf::imwrite("hls_out.jpg",imgOutput);

	xf::absDiff(output_image, imgOutput, output_diff_xf_cv);

	imwrite("error.png", output_diff_xf_cv); // Save the difference image for debugging purpose

	float err_per;
	xf::analyzeDiff(output_diff_xf_cv, 0, err_per);

	if(err_per > 1){
		printf("\nTest failed\n");
		return -1;
	}
	else{
		printf("\nTest Pass\n");
		return 0;
	}


}
