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
#include "xf_canny_config.h"

typedef unsigned char NMSTYPE;

NMSTYPE Filter3x3(NMSTYPE t0, NMSTYPE t1, NMSTYPE t2, NMSTYPE m0, NMSTYPE m1, NMSTYPE m2, NMSTYPE b0, NMSTYPE b1, NMSTYPE b2){
	NMSTYPE value = false;
	int g0 = t0+t2+b0+b2;
	int g1 = (t1+b1+m0+m2)<<1;
	int g2 = m1<<2;

	value = ((int)(g0+g1+g2) >> 4);
	return value;
}
void AverageGaussian(cv::Mat &src, cv::Mat &dst){
	int i, j;
	NMSTYPE t0, t1, t2;
	NMSTYPE m0, m1, m2;
	NMSTYPE b0, b1, b2;
	NMSTYPE result;

	/*			First row			*/
	i = 0;
	for(j = 0; j < src.cols; j++){
		if(j==0){
			t0 = 0;
			t1 = 0;
			t2 = 0;
			m0 = 0;
			m1 = src.at<NMSTYPE>(i, j);
			m2 = src.at<NMSTYPE>(i, j+1);
			b0 = 0;
			b1 = src.at<NMSTYPE>(i+1, j);
			b2 = src.at<NMSTYPE>(i+1, j+1);
		}
		else if((j > 0) && (j < src.cols-1)){
			t0 = 0;
			t1 = 0;
			t2 = 0;
			m0 = src.at<NMSTYPE>(i, j-1);
			m1 = src.at<NMSTYPE>(i, j);
			m2 = src.at<NMSTYPE>(i, j+1);
			b0 = src.at<NMSTYPE>(i+1, j-1);
			b1 = src.at<NMSTYPE>(i+1, j);
			b2 = src.at<NMSTYPE>(i+1, j+1);
		}else if(j == src.cols-1){
			t0 = 0;
			t1 = 0;
			t2 = 0;
			m0 = src.at<NMSTYPE>(i, j-1);
			m1 = src.at<NMSTYPE>(i, j);
			m2 = 0;
			b0 = src.at<NMSTYPE>(i+1, j-1);
			b1 = src.at<NMSTYPE>(i+1, j);
			b2 = 0;
		}
		result = Filter3x3(t0, t1, t2, m0, m1, m2, b0, b1, b2);
		dst.at<uchar>(i, j) = result;
	}
	for(i = 1; i < src.rows-1; i++){
		for(j = 0; j < src.cols; j++){
			if(j==0){
				t0 = 0;
				t1 = src.at<NMSTYPE>(i-1, j);
				t2 = src.at<NMSTYPE>(i-1, j+1);
				m0 = 0;
				m1 = src.at<NMSTYPE>(i, j);
				m2 = src.at<NMSTYPE>(i, j+1);
				b0 = 0;
				b1 = src.at<NMSTYPE>(i+1, j);
				b2 = src.at<NMSTYPE>(i+1, j+1);
			}else if((j > 0) && (j < src.cols-1)){
				t0 = src.at<NMSTYPE>(i-1, j-1);
				t1 = src.at<NMSTYPE>(i-1, j);
				t2 = src.at<NMSTYPE>(i-1, j+1);
				m0 = src.at<NMSTYPE>(i, j-1);
				m1 = src.at<NMSTYPE>(i, j);
				m2 = src.at<NMSTYPE>(i, j+1);
				b0 = src.at<NMSTYPE>(i+1, j-1);
				b1 = src.at<NMSTYPE>(i+1, j);
				b2 = src.at<NMSTYPE>(i+1, j+1);
			}else if(j == src.cols-1){
				t0 = src.at<NMSTYPE>(i-1, j-1);
				t1 = src.at<NMSTYPE>(i-1, j);
				t2 = 0;
				m0 = src.at<NMSTYPE>(i, j-1);
				m1 = src.at<NMSTYPE>(i, j);
				m2 = 0;
				b0 = src.at<NMSTYPE>(i+1, j-1);
				b1 = src.at<NMSTYPE>(i+1, j);
				b2 = 0;
			}
			result = Filter3x3(t0, t1, t2, m0, m1, m2, b0, b1, b2);
			dst.at<uchar>(i, j) = result;
		}
	}
	/*			Last row			*/
	i = src.rows-1;
	for(j = 0; j < src.cols; j++){
		if(j==0){
			t0 = 0;
			t1 = src.at<NMSTYPE>(i-1, j);
			t2 = src.at<NMSTYPE>(i-1, j+1);
			m0 = 0;
			m1 = src.at<NMSTYPE>(i, j);
			m2 = src.at<NMSTYPE>(i, j+1);
			b0 = 0;
			b1 = 0;//src.at<NMSTYPE>(i+1, j);
			b2 = 0;//src.at<NMSTYPE>(i+1, j+1);
		}else if((j > 0) && (j < src.cols-1)){
			t0 = src.at<NMSTYPE>(i-1, j-1);
			t1 = src.at<NMSTYPE>(i-1, j);
			t2 = src.at<NMSTYPE>(i-1, j+1);
			m0 = src.at<NMSTYPE>(i, j-1);
			m1 = src.at<NMSTYPE>(i, j);
			m2 = src.at<NMSTYPE>(i, j+1);
			b0 = 0;
			b1 = 0;
			b2 = 0;
		}
		else if(j == src.cols-1){
			t0 = src.at<NMSTYPE>(i-1, j-1);
			t1 = src.at<NMSTYPE>(i-1, j);
			t2 = 0;
			m0 = src.at<NMSTYPE>(i, j-1);
			m1 = src.at<NMSTYPE>(i, j);
			m2 = 0;
			b0 = 0;
			b1 = 0;
			b2 = 0;
		}
		result = Filter3x3(t0, t1, t2, m0, m1, m2, b0, b1, b2);
		dst.at<uchar>(i, j) = result;
	}

}



int main(int argc, char **argv)
{
	//# Images
	cv::Mat in_img;
	cv::Mat img_gray, hls_img, ocv_img,out_img;
	cv::Mat diff;

	if(argc != 2){
		printf("Usage : <executable> <input image> \n");
		return -1;
	}

	char img_path[1000];


	in_img = cv::imread(argv[1],1); 									// reading in the color image
	if(!in_img.data)
	{
		printf("Failed to load the image ... %s\n!", argv[1]);
		return -1;
	}

	extractChannel(in_img,img_gray,1);								// Extract gray scale image

	hls_img.create(img_gray.rows, img_gray.cols, img_gray.depth());	// HLS image creation
	out_img.create(img_gray.rows, img_gray.cols, img_gray.depth());	// HLS image creation
	
	/*				Apply Gaussian mask and call opencv canny function					*/
	cv::Mat img_gray1;
	img_gray1.create(img_gray.rows, img_gray.cols, img_gray.depth());
	AverageGaussian(img_gray, img_gray1);							// Gaussian filter

#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif
#if L1NORM
	cv::Canny(img_gray1, ocv_img, 30.0, 64.0, FILTER_WIDTH, false);		// Opencv canny function

#else
	cv::Canny(img_gray1, ocv_img, 30.0, 64.0, FILTER_WIDTH, true);		// Opencv canny function
#endif
#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif


	uint16_t img_height, img_width;
	uchar low_threshold, high_threshold;
	img_height = img_gray.rows;
	img_width  = img_gray.cols;
	low_threshold = 30;
	high_threshold = 64;


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, INTYPE> imgInput(img_gray.rows,img_gray.cols); //XF_NPPC1,XF_NPPC4
	static xf::Mat<XF_2UC1, HEIGHT, WIDTH, XF_NPPC32> nms_output(img_gray.rows,img_gray.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC8> edge_output(img_gray.rows,img_gray.cols);


	imgInput.copyTo(img_gray.data);
	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	canny_accel(imgInput,nms_output,edge_output,low_threshold,high_threshold);


	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	out_img.data = edge_output.copyFrom();


	
	absdiff(ocv_img,out_img,diff);									// Absolute difference between opencv and hls result
	imwrite("hls.png", out_img);									// Save HLS result
	imwrite("ocv.png", ocv_img);									// Save Opencv result
	imwrite("diff.png", diff);
	// Save difference image
	// Find minimum and maximum differences.
	double minval=256,maxval=0;

	FILE *fp = fopen("nmsvalues.txt","w");
	int cnt = 0;
	for (int i=0; i<diff.rows-0; i++)
	{
		for(int j=0; j<diff.cols-0; j++)
		{
			uchar v = diff.at<uchar>(i,j);
			fprintf(fp,"%d ",v);
			if (v>0)//(v>0 && v!=127 && v!=128)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
		fprintf(fp,"\n");
	}

	fclose(fp);
	float err_per = 100.0 * (float)cnt / (diff.rows * diff.cols);

	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\nNo of Pixels with Error = %d\n",minval,maxval,err_per, cnt);

	if(err_per > 2.0f)
		return 1;
	/*			Destructors			*/
	in_img.~Mat();
	img_gray.~Mat();
	img_gray1.~Mat();
	hls_img.~Mat();
	ocv_img.~Mat();
	diff.~Mat();

	return 0;
}
