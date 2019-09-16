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
#include "xf_histogram_config.h"


int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray,hist_ocv;



#if GRAY
	// reading in the color image
	in_img = cv::imread(argv[1], 0);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}
	//cvtColor(in_img, in_img, CV_BGR2GRAY);
	//////////////////	Opencv Reference  ////////////////////////
	int histSize = 256;
	/// Set the ranges ( for B,G,R) )
	float range[] = { 0, 256 } ;
	const float* histRange = { range };
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	cv::calcHist( &in_img, 1, 0, cv::Mat(), hist_ocv, 1, &histSize, &histRange, 1, 0 );
#if __SDSCC__
		hw_ctr.stop();
	 uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

#else
	// reading in the color image
	in_img = cv::imread(argv[1], 1);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}
	//////////////////	Opencv Reference  ////////////////////////
	cv::Mat b_hist,g_hist,r_hist;
	std::vector<cv::Mat> bgr_planes;
	cv::split( in_img, bgr_planes );
	int histSize = 256;
	/// Set the ranges ( for B,G,R) )
	float range[] = { 0, 256 } ;
	const float* histRange[] = {range};
#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif
	cv::calcHist( &bgr_planes[0], 1, 0, cv::Mat(), b_hist, 1, &histSize, histRange, 1, 0 );
	cv::calcHist( &bgr_planes[1], 1, 0, cv::Mat(), g_hist, 1, &histSize, histRange, 1, 0 );
	cv::calcHist( &bgr_planes[2], 1, 0, cv::Mat(), r_hist, 1, &histSize, histRange, 1, 0 );
#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif
#endif
#if __SDSCC__
	uint32_t *histogram=(uint32_t *)sds_alloc_non_cacheable(256*sizeof(uint32_t)*in_img.channels());

#else

	uint32_t *histogram=(uint32_t *)malloc(256*sizeof(uint32_t)*in_img.channels());

#endif

////////////////////////////////////////////////////////////////////////////
	static xf::Mat<TYPE, HEIGHT, WIDTH, _NPPC> imgInput(in_img.rows,in_img.cols);
	imgInput.copyTo(in_img.data);

	#if __SDSCC__
	perf_counter hw_ctr2;
	hw_ctr2.start();
	#endif

	histogram_accel (imgInput, histogram);

	#if __SDSCC__
	hw_ctr2.stop();
	uint64_t hw_cycles2 = hw_ctr2.avg_cpu_cycles();
	#endif


#if GRAY
	FILE *fp, *fp1;
	fp = fopen("out_hls.txt", "w");
	fp1 = fopen("out_ocv.txt", "w");
	for(int cnt=0;cnt<256;cnt++)
	{
		fprintf(fp,"%u\n",histogram[cnt]);
		uint32_t val = (uint32_t)hist_ocv.at<float>(cnt);
		if(val != histogram[cnt]){
			printf("\nTest Failed\n");
			return 1;
		}
	fprintf(fp1,"%u\n",val);
	}
	fclose(fp);
	fclose(fp1);
#else
	FILE *total=fopen("total.txt","w");
	for(int i=0;i<768;i++)
	{
	fprintf(total,"%d\n",histogram[i]);
	}
	fclose(total);
	FILE *fp, *fp1;
	fp = fopen("out_hls.txt", "w");
	fp1 = fopen("out_ocv.txt", "w");
	for(int cnt=0;cnt<256;cnt++)
	{
		fprintf(fp,"%u	%u	%u\n",histogram[cnt],histogram[cnt+256],histogram[cnt+512]);
		uint32_t b_val = (uint32_t)b_hist.at<float>(cnt);
		uint32_t g_val = (uint32_t)g_hist.at<float>(cnt);
		uint32_t r_val = (uint32_t)r_hist.at<float>(cnt);
		if((b_val != histogram[cnt]) && (g_val != histogram[256+cnt]) &&(r_val != histogram[512+cnt])){
			printf("\nTest Failed\n");
			return 1;
		}
		fprintf(fp1,"%u	%u	%u\n",b_val,g_val,r_val);

	}
	fclose(fp);
	fclose(fp1);
#endif
	return 0;
}



