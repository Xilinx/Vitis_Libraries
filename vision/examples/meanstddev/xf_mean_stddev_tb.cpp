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
#include "xf_mean_stddev_config.h"

template<int CHNLS>
float* xmean(cv::Mat& img)
{
	unsigned long Sum[CHNLS] ={0};
	int i, j,c, k;
	float* val=(float*)malloc(CHNLS*sizeof(float));

	/* Sum of All Pixels */
	for(i = 0; i < img.rows; ++i)
	{
		for(j = 0; j < img.cols; ++j)
		{
			for(k=0; k< CHNLS;++k){
if(CHNLS==1)
				Sum[k] += img.at<uchar>(i,j); //imag.data[i]}
else
				Sum[k] += img.at< cv::Vec3b >(i,j)[k]; //imag.data[i]}


			}
		}
	}
	for(int ch = 0; ch<CHNLS;++ch){
		val[ch]=(float)Sum[ch] /(float)(img.rows * img.cols);
	}
	return val;
}
template <int CHNLS>
void variance(cv::Mat& Img, float* mean, double *var)
{
	double sum[CHNLS] ,b_sum=0.0,g_sum=0.0,r_sum=0.0;
	int k;
	double x[CHNLS];
	for(int i = 0; i < Img.rows; i++)
	{
		for(int j = 0; j < Img.cols; j++)
		{
			for(k=0; k< CHNLS;++k){

				if(CHNLS==1)
					x[k] = (double)mean[k] - ((double)Img.at<uint8_t>(i,j));
				else
					x[k] = (double)mean[k] - ((double)Img.at<cv::Vec3b>(i,j)[k]);

				sum[k] = sum[k] + pow(x[k], (double)2.0);
			}

		}
	}
	for(int ch=0; ch< CHNLS;++ch){
		var[ch] = (sum[ch] / (double)(Img.rows * Img.cols));
	}
}


int main(int argc, char** argv)
{

	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray;

	// reading in the color image
	in_img = cv::imread(argv[1], 1);
	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}


#if GRAY
	cvtColor(in_img, in_img, CV_BGR2GRAY);
#endif

	int channels=in_img.channels();
	printf("Channels - %d\n", channels);
	const int xfcv_channels = XF_CHANNELS(TYPE, _NPPC);

		//////////////// 	Opencv  Reference  ////////////////////////
	float *mean_c=(float*)malloc(channels*sizeof(float));
	float *stddev_c=(float*)malloc(channels*sizeof(float));
	double *var_c=(double*)malloc(channels*sizeof(double));
	float mean_hls[channels],stddev_hls[channels];
	float diff_mean[channels],diff_stddev[channels];
	
	
#if __SDSCC__
perf_counter hw_ctr1;
hw_ctr1.start();
#endif
	mean_c = xmean<xfcv_channels>(in_img);
	variance<xfcv_channels>(in_img, mean_c, var_c);
#if __SDSCC__
hw_ctr1.stop();uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif
	
	
#if __SDSCC__
	unsigned short *mean=(unsigned short*)sds_alloc_non_cacheable(channels*sizeof(unsigned short));
	unsigned short *stddev=(unsigned short*)sds_alloc_non_cacheable(channels*sizeof(unsigned short));
#else
	unsigned short *mean=(unsigned short*)malloc( channels* sizeof ( unsigned short));
	unsigned short *stddev=(unsigned short*)malloc(channels*sizeof(unsigned short));
#endif
	xf::Mat<TYPE, HEIGHT, WIDTH, _NPPC> imgInput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);

	///HLS function call
#if __SDSCC__
perf_counter hw_ctr;
hw_ctr.start();
#endif
	mean_stddev_accel(imgInput,mean, stddev);
#if __SDSCC__
hw_ctr.stop();uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	for(int c=0;c<channels;c++)
	{
		stddev_c[c] = sqrt(var_c[c]);
		mean_hls[c] =(float)mean[c]/256;
		stddev_hls[c] = (float)stddev[c]/256;
		diff_mean[c] = mean_c[c] - mean_hls[c] ;
	diff_stddev[c] = stddev_c[c] - stddev_hls[c];
	fprintf(stderr,"Ref. Mean     = %f\t Result = %f\tERROR = %f \n",mean_c[c], mean_hls[c], diff_mean[c]);
	fprintf(stderr,"Ref. Std.Dev. = %f\t Result = %f\tERROR = %f \n",stddev_c[c], stddev_hls[c], diff_stddev[c]);


	if(abs(diff_mean[c]) > 1 | abs(diff_stddev[c]) > 1){
		printf("\nTest Failed\n");
		return -1;
	}
	}
	return 0;
}
