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
#include "xf_paintmask_config.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, out_img, ocv_ref, in_gray, diff, in_mask;

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

	ocv_ref.create(in_img.rows,in_img.cols,in_img.depth());
	out_img.create(in_img.rows,in_img.cols,in_img.depth());
	diff.create(in_img.rows,in_img.cols,in_img.depth());
	in_mask.create(in_img.rows,in_img.cols,CV_8UC1);

	ap_uint<64> q=0;
	for(int i=0;i<in_img.rows;i++)
	{
		for(int j=0;j<in_img.cols;j++)
		{
			for(int c=0;c<in_img.channels();c++)
			{

				if((j > 250)&&(j<750))
				{
					in_mask.data[q+c]=255;
				}
				else
				{
					in_mask.data[q+c]=0;
				}
			}
			q+=in_img.channels();
		}
	}


	////////////////////// HLS TOP function call ////////////////////////////

	static xf::Mat<SRC_T, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<MASK_T, HEIGHT, WIDTH, NPIX> In_mask(in_mask.rows,in_mask.cols);
	static xf::Mat<SRC_T, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows,in_img.cols);

#if __SDSCC__
	unsigned char *color=(unsigned char *)sds_alloc_non_cacheable(XF_CHANNELS(SRC_T,NPIX)*sizeof(unsigned char));
#else
	unsigned char color[XF_CHANNELS(SRC_T,NPIX)];
#endif
	for(int i=0; i < XF_CHANNELS(SRC_T,NPIX);i++)
	{
		color[i]=150;
	}

	imgInput.copyTo(in_img.data);
	In_mask.copyTo(in_mask.data);
	imwrite("in_mask.jpg", in_mask);

	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	paintmask_accel(imgInput,In_mask, imgOutput,color);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif



	////////////////  reference code  ////////////////
	unsigned long long int p=0;
	for(int i=0;i<ocv_ref.rows;i++)
	{
		for(int j=0;j<ocv_ref.cols;j++)
		{
			for(int c=0;c<ocv_ref.channels();c++)
			{
				if(in_mask.data[p+c] !=0 )
				{
					ocv_ref.data[p+c] = color[c];
				}
				else
				{
					ocv_ref.data[p+c] = in_img.data[p+c];
				}

			}
			p+=in_img.channels();

		}
	}
   //////////////////  end C reference code//////////


	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);
	cv::imwrite("ref_img.jpg", ocv_ref);  // reference image


	xf::absDiff(ocv_ref, imgOutput, diff);
	imwrite("diff_img.jpg",diff); // Save the difference image for debugging purpose

	// Find minimum and maximum differences.
	double minval = 256, maxval1 = 0;
	int cnt = 0;
	for (int i = 0; i < in_img.rows; i++)
	{
		for(int j = 0; j < in_img.cols;j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v > 1)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval1 < v)
				maxval1 = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval1,err_per);


	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
