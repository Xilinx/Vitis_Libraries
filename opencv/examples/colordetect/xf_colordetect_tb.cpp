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
#include "xf_colordetect_config.h"

// colordetect
// Description:
// Will detect the colors from the thresholds provided
// Inputs:
//	- in_img
//  - nLowThresh
//  - nHighThresh
// Output:
//  - out_img
void colordetect(cv::Mat &_src,
		cv::Mat &_dst,
		unsigned char nLowThresh[3][3],
		unsigned char nHighThresh[3][3]) {

	// Temporary matrices for processing
	cv::Mat mask1, mask2, mask3, _imgrange, _imghsv;

	// Convert the input to the HSV colorspace. Using BGR here since it is the default of OpenCV.
	// Using RGB yields different results, requiring a change of the threshold ranges
	cv::cvtColor(_src, _imghsv, cv::COLOR_BGR2HSV);

	// Get the color of Yellow from the HSV image and store it as a mask
	cv::inRange(_imghsv, cv::Scalar(nLowThresh[0][0], nLowThresh[0][1], nLowThresh[0][2]), cv::Scalar(nHighThresh[0][0], nHighThresh[0][1], nHighThresh[0][2]), mask1);

	// Get the color of Green from the HSV image and store it as a mask
	cv::inRange(_imghsv, cv::Scalar(nLowThresh[1][0], nLowThresh[1][1], nLowThresh[1][2]), cv::Scalar(nHighThresh[1][0], nHighThresh[1][1], nHighThresh[1][2]), mask2);

	// Get the color of Red from the HSV image and store it as a mask
	cv::inRange(_imghsv, cv::Scalar(nLowThresh[2][0], nLowThresh[2][1], nLowThresh[2][2]), cv::Scalar(nHighThresh[2][0], nHighThresh[2][1], nHighThresh[2][2]), mask3);

	// Bitwise OR the masks together (adding them) to the range
	_imgrange = mask1 | mask2 | mask3;


	cv::Mat element = cv::getStructuringElement( KERNEL_SHAPE,cv::Size(FILTER_SIZE, FILTER_SIZE), cv::Point(-1, -1));
	cv::erode(_imgrange, _dst, element, cv::Point(-1, -1), ITERATIONS, cv::BORDER_CONSTANT);

	cv::dilate(_dst, _dst, element, cv::Point(-1, -1), ITERATIONS, cv::BORDER_CONSTANT);

	cv::dilate(_dst, _dst, element, cv::Point(-1, -1), ITERATIONS, cv::BORDER_CONSTANT);

	cv::erode(_dst, _dst, element, cv::Point(-1, -1), ITERATIONS, cv::BORDER_CONSTANT);
}

int main(int argc, char **argv)
{
	cv::Mat in_img, img_rgba,out_img,out_img1;



	in_img = cv::imread(argv[1],1);
	if(!in_img.data){
		return -1;
	}

	uint16_t height = in_img.rows;
	uint16_t width = in_img.cols;

	out_img.create(height,width,CV_8U);

	out_img1.create(height,width,CV_8U);

	//cv::cvtColor(in_img, img_rgba, CV_BGR2RGBA);
	
	cv::Mat element = cv::getStructuringElement( KERNEL_SHAPE,cv::Size(FILTER_SIZE, FILTER_SIZE), cv::Point(-1, -1));

#if __SDSCC__
	unsigned char * high_thresh = (unsigned char *)sds_alloc_non_cacheable(9* sizeof(unsigned char));
	unsigned char * low_thresh = (unsigned char *)sds_alloc_non_cacheable(9* sizeof(unsigned char));
	unsigned char *structure_element=(unsigned char *)sds_alloc_non_cacheable(sizeof(unsigned char)*FILTER_SIZE*FILTER_SIZE);
#else
	unsigned char * high_thresh = (unsigned char *)malloc(9* sizeof(unsigned char));
	unsigned char * low_thresh = (unsigned char *)malloc(9* sizeof(unsigned char));
	unsigned char structure_element[FILTER_SIZE*FILTER_SIZE];
#endif

	for(int i=0;i<(FILTER_SIZE*FILTER_SIZE);i++)
	{
		structure_element[i]=element.data[i];
	}



	low_thresh[0] = 22;
	low_thresh[1] = 150;
	low_thresh[2] = 60;

	high_thresh[0] = 38;
	high_thresh[1] = 255;
	high_thresh[2] = 255;

	low_thresh[3] = 38;
	low_thresh[4] = 150;
	low_thresh[5] = 60;

	high_thresh[3] = 75;
	high_thresh[4] = 255;
	high_thresh[5] = 255;

	low_thresh[6] = 160;
	low_thresh[7] = 150;
	low_thresh[8] = 60;

	high_thresh[6] = 179;
	high_thresh[7] = 255;
	high_thresh[8] = 255;

	printf("thresholds loaded");
	
	
	// Define the low and high thresholds
	// Want to grab 3 colors (Yellow, Green, Red) for teh input image
	unsigned char nLowThresh[3][3] = { { 22, 150, 60 }, // Lower boundary for Yellow
			{ 38, 150, 60 }, // Lower boundary for Green
			{ 160, 150, 60 } }; // Lower boundary for Red
	unsigned char nHighThresh[3][3] = { { 38, 255, 255 }, // Upper boundary for Yellow
			{ 75, 255, 255 }, // Upper boundary for Green
			{ 179, 255, 255 } }; // Upper boundary for Red


#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif
	colordetect(in_img, out_img1, nLowThresh, nHighThresh);
#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif

	static xf::Mat<XF_8UC3,HEIGHT,WIDTH,NPIX> imgInput(in_img.rows,in_img.cols);

	static xf::Mat<XF_8UC3,HEIGHT,WIDTH,NPIX> hsvimage(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgrange(in_img.rows,in_img.cols);

	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgerode1(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgdilate1(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgdilate2(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgOutput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);


	printf("image loaded");

#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif

	colordetect_accel(imgInput,hsvimage,imgrange,imgerode1,imgdilate1,imgdilate2,imgOutput, low_thresh, high_thresh,structure_element);

#if __SDSCC__

	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	out_img.data = imgOutput.copyFrom();

	


	imwrite("outputref.png", out_img1);
	imwrite("output.png", out_img); 
	imwrite("input.png", in_img); 

	return 0;
}
