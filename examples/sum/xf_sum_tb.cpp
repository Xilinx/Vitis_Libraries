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
#include "xf_sum_config.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path1> \n");
		return -1;
	}

	cv::Mat in_gray, in_gray1,out_gray,diff;
#if GRAY
	in_gray  = cv::imread(argv[1], 0); // read image
#endif
	if (in_gray.data == NULL)
	{
		fprintf(stderr, "Cannot open image %s\n", argv[1]);
		return -1;
	}

	int channels=in_gray.channels();

	double ocv_ref[3];
	// OpenCV function
	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	 ocv_ref[0]=cv::sum(in_gray)[0];


	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

//	 ocv_ref[1]=cv::sum(in_gray)[1];
//	 ocv_ref[2]=cv::sum(in_gray)[2];

	static xf::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray.rows,in_gray.cols);
#if __SDSCC___
	double *scl=(double *)sds_alloc_non_cacheable(XF_CHANNELS(IN_TYPE,NPC1)*sizeof(double));
#else
	double *scl=(double *)malloc(XF_CHANNELS(IN_TYPE,NPC1)*sizeof(double));
#endif


	imgInput1.copyTo(in_gray.data);

	#if __SDSCC__
		perf_counter hw_ctr1;
hw_ctr1.start();
	#endif

	sum_accel(imgInput1,scl);

	#if __SDSCC__
		hw_ctr1.stop();
uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif

	for(int i=0;i< in_gray.channels();i++)
	{

	printf("sum of opencv is=== %lf\n",ocv_ref[i]);
	printf("sum of hls is====== %lf\n",scl[i]);
	}

	return 0;
}
