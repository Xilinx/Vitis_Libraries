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
#include "xf_reduce_config.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, dst_hls, ocv_ref, in_gray, diff, in_mask;

	unsigned short in_width,in_height;

#if GRAY
	/*  reading in the color image  */
	in_img = cv::imread(argv[1],0);
#endif

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	in_width = in_img.cols;
	in_height = in_img.rows;

#if DIM
	dst_hls.create(in_img.rows, 1, CV_8UC1);
	ocv_ref.create(in_img.rows, 1, CV_8UC1);
#else
	dst_hls.create( 1,in_img.cols, CV_8UC1);
	ocv_ref.create( 1,in_img.cols, CV_8UC1);
#endif

        int bytes;

	////////////////  reference code  ////////////////
       if((REDUCTION_OP        == XF_REDUCE_AVG) || (REDUCTION_OP      == XF_REDUCE_SUM))
        {
                 bytes=4;
        #if __SDSCC__
                perf_counter hw_ctr1;
        hw_ctr1.start();
        #endif
                cv::reduce(in_img,ocv_ref,DIM,REDUCTION_OP,CV_32SC1);   //avg,sum
        
        #if __SDSCC__
                hw_ctr1.stop();
        uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
        #endif
        }
        else
        {
                 bytes=1;
        #if __SDSCC__
                perf_counter hw_ctr2;
        hw_ctr2.start();
        #endif

        cv::reduce(in_img,ocv_ref,DIM,REDUCTION_OP,CV_8UC1);

        #if __SDSCC__
                hw_ctr2.stop();
        uint64_t hw_cycles2 = hw_ctr2.avg_cpu_cycles();
        #endif
        }


	////////////////////// HLS TOP function call ////////////////////////////

	static xf::Mat<SRC_T, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows,in_img.cols);
#if DIM
	static xf::Mat<DST_T, ONE_D_HEIGHT, ONE_D_WIDTH, XF_NPPC1> imgOutput(in_img.rows, 1);
#else
	static xf::Mat<DST_T, ONE_D_HEIGHT, ONE_D_WIDTH, XF_NPPC1> imgOutput( 1,in_img.cols);
#endif

	imgInput.copyTo(in_img.data);

#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif

	reduce_accel(imgInput, imgOutput,DIM);

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	dst_hls.data = imgOutput.copyFrom();


	FILE *fp=fopen("hls","w");
	FILE *fp1=fopen("cv","w");
#if DIM==1
	for(unsigned int i=0;i< dst_hls.rows;i++)
	{

		fprintf(fp,"%d\n",(unsigned char)dst_hls.data[i]);
		fprintf(fp1,"%d\n",ocv_ref.data[i]);
		unsigned int diff=ocv_ref.data[i]-(unsigned char)dst_hls.data[i];
		if(diff>1)
		{
			printf("Output is not matched with opnecv\n");
		}

	}
#endif
#if DIM==0
	for(unsigned int i=0;i< dst_hls.cols;i++)
	{

		fprintf(fp,"%d\n",(unsigned char)dst_hls.data[i]);
		fprintf(fp1,"%d\n",ocv_ref.data[i]);
		unsigned int diff=ocv_ref.data[i]-(unsigned char)dst_hls.data[i];
		if(diff>1)
		{
			printf("Output is not matched with opnecv\n");
		}

	}
#endif
	fclose(fp);
	fclose(fp1);

	return 0;
}
