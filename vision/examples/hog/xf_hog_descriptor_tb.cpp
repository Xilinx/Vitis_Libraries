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
#include "xf_hog_descriptor_config.h"
#include "ObjDet_reference.hpp"

//for masking the output value
#define operatorAND 0x0000FFFF

// main function
int main(int argc, char **argv)
{
	if(argc != 2)
	{
		printf("Usage: <executable> <input image path> \n");
		return -1;
	}

	cv::Mat img_raw,img,in_gray;

//	printf("%s\n",CV_MAJOR_VERSION);

	// Test image
	img_raw = cv::imread(argv[1],1); // load as color image through command line argument

	if(!img_raw.data)
	{
		std::cout<<"No data! -- Exiting the program"<<std::endl;
		return -1;
	}

	// Converting the image type based on the configuration
#if GRAY_T
	cvtColor(img_raw, img, CV_BGR2GRAY);
#elif RGB_T
	cvtColor(img_raw, img, CV_BGR2RGB);
#endif

	// creating the input pointers
	//IN_T* in_ptr = (IN_T*) img.data;
	uint16_t image_height = img.rows;
	uint16_t image_width = img.cols;

	int novw_tb = ((image_height-XF_WIN_HEIGHT)/XF_WIN_STRIDE)+1, nohw_tb = ((image_width-XF_WIN_WIDTH)/XF_WIN_STRIDE)+1;
	int total_no_of_windows = novw_tb*nohw_tb;
	int novb_tb = ((image_height/XF_CELL_HEIGHT)-1), nohb_tb = ((image_width/XF_CELL_WIDTH)-1);

#if REPETITIVE_BLOCKS
	int dim_rb = (total_no_of_windows*XF_NODPW)>>1;
	int no_of_descs = dim_rb;
#elif NON_REPETITIVE_BLOCKS
	int dim_nrb = (nohb_tb*novb_tb*XF_NOBPB)>>1;
	int no_of_descs = dim_nrb;
	int dim_expand = (dim_nrb << 1);
#endif
	int dim = (total_no_of_windows*XF_NODPW);

	/////////////     Reference for HOG implementation     /////////
	AURHOGDescriptor d(Size(XF_WIN_WIDTH,XF_WIN_HEIGHT),Size(XF_BLOCK_WIDTH,XF_BLOCK_HEIGHT),Size(XF_CELL_WIDTH,XF_CELL_HEIGHT),Size(XF_CELL_WIDTH,XF_CELL_HEIGHT),XF_NO_OF_BINS);

	vector<float> descriptorsValues;
	vector<Point> locations;
#if RGB_T
	cvtColor(img_raw, img_raw, CV_BGR2RGB);
#endif
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
#if GRAY_T
	d.AURcompute(img, descriptorsValues, Size(XF_CELL_WIDTH,XF_CELL_HEIGHT), Size(0,0), locations);
#elif RGB_T
	d.AURcompute(img_raw, descriptorsValues, Size(XF_CELL_WIDTH,XF_CELL_HEIGHT), Size(0,0), locations);
#endif
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	float* ocv_out_fl = (float*)malloc(dim * sizeof(float));

	// output of the OCV will be in column major form, for comparison reason we convert that into row major
	cmToRmConv(descriptorsValues,ocv_out_fl,total_no_of_windows);
	////////////////////////////   END OF REFERENCE   ////////////////////////////

	//////////////////	HLS TOP Function Call  ////////////////////////
	static xf::Mat<XF_INPUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1> inMat(img.rows,img.cols);
	inMat.copyTo(img.data);
#if REPETITIVE_BLOCKS
	static xf::Mat<XF_32UC1, 1, XF_DESC_SIZE, XF_NPPC1> outMat(1,dim_rb);
#elif NON_REPETITIVE_BLOCKS
	static xf::Mat<XF_32UC1, 1, XF_DESC_SIZE, XF_NPPC1> outMat(1,dim_nrb);
#endif

#ifdef __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif
	hog_descriptor_accel(inMat,outMat);
#ifdef __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif

#if REPETITIVE_BLOCKS
	ap_uint32_t* output = (ap_uint32_t*)malloc(dim_rb * sizeof(ap_uint32_t));
#elif NON_REPETITIVE_BLOCKS
	ap_uint32_t* output = (ap_uint32_t*)malloc(dim_nrb * sizeof(ap_uint32_t));
#endif

#if NON_REPETITIVE_BLOCKS

	for(int i=0;i<dim_nrb;i++)
	{
		output[i]=outMat.read(i);
	}

#elif REPETITIVE_BLOCKS
	OUT_T1* output1 = (OUT_T1*)malloc(dim * sizeof(OUT_T1));

	ap_uint32_t temp;
	int high=15,low=0;

	int cnt=0;
	for(int i=0;i<(dim_rb);i++)
	{
		temp=outMat.read(i);
		high=15;low=0;
		for(int j=0;j<2;j++)
		{
			output1[i*2+j]=temp.range(high,low);
			high+=16;
			low+=16;
			cnt++;
		}
	}
#endif

	float* out_fl = (float*)malloc(dim * sizeof(float));

	// converting the fixed point data to floating point data for comparison
#if REPETITIVE_BLOCKS
	for(int i=0;i<dim;i++)
	{
		out_fl[i] = ((float)output1[i]/(float)65536.0);
	}
#elif NON_REPETITIVE_BLOCKS   // Reading in the NRB mode and arranging the data for comparison
	float out_fl_tmp[dim_expand];
	int tmp_cnt=0;

	for(int i = 0; i < dim_nrb; i++)
	{
		for(int j=0;j<2;j++)
		{
			out_fl_tmp[((i<<1)+j)] = (float)((output[i]>>(j<<4))&operatorAND)/(float)65536.0;
		}
	}

	int mul_factor = (nohb_tb*XF_NOBPB);

	for(int i=0;i<novw_tb;i++)
	{
		for(int j=0;j<nohw_tb;j++)
		{
			for(int k=0;k<XF_NOVBPW;k++)
			{
				for(int l=0;l<XF_NOHBPW;l++)
				{
					for(int m=0;m<XF_NOBPB;m++)
					{
						int index = (i*mul_factor)+(j*XF_NOBPB)+(k*mul_factor)+(l*XF_NOBPB)+m;
						out_fl[tmp_cnt] = out_fl_tmp[index];
						tmp_cnt++;
					}
				}
			}
		}
	}
#endif

	/************** error analysis **************/
	float acc_diff=0, max_diff=0, max_pos=0, counter_for_diff=0;
	float ocv_desc_at_max, hls_desc_at_max;

	for(int i=0;i<dim;i++)
	{
		float diff = ocv_out_fl[i] - out_fl[i];
		if(diff < 0)  diff=(-diff);

		if(diff > max_diff)
		{
			max_diff=diff;
			max_pos=i;
			ocv_desc_at_max = ocv_out_fl[i];
			hls_desc_at_max = out_fl[i];
		}

		if(diff>0.01)
		{
			counter_for_diff++;
		}

		acc_diff += diff;
	}

	float avg_diff = (float)acc_diff/(float)dim;

	// printing the descriptor details to the console
	cout << "img dimensions: " << img.cols << "width x " << img.rows << "height" << endl;
	cout << "Found " << descriptorsValues.size() << " descriptor values" << endl;

	cout<<"descriptors having difference more than (0.01f): "<<counter_for_diff<<endl;
	cout<<"avg_diff:"<<avg_diff<<endl<<"max_diff:"<<max_diff<<endl<<"max_position:"<<max_pos<<endl;
	cout<<"ocv_desc_val_at_max_diff:"<<ocv_desc_at_max<<endl<<"hls_desc_val_at_max_diff:"<<hls_desc_at_max<<endl;
	cout<<endl;
	/***********  End Of Error Analysis  ***********/

	free(output);
	free(out_fl);
	free(ocv_out_fl);

	if (max_diff > 0.1f)
		return -1;

	return 0;
}
