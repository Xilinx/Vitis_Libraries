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

#ifndef _XF_OTSUTHRESHOLD_HPP_
#define _XF_OTSUTHRESHOLD_HPP_

#include "common/xf_types.h"
#include "core/xf_math.h"
#include "imgproc/xf_histogram.hpp"

namespace xf{

static void xfOtsuKernel(uint32_t _hist[0][256], uint16_t _height, uint16_t _width, uint8_t &thresh)
{
#pragma HLS INLINE off

	ap_uint<33> HistArray[256] = {0};
	ap_uint<45> tmp1, tmp2;

	ap_uint<25> total;
	uint8_t max_val = 0;
	unsigned int sum = 0;

	unsigned short int cols = _width;
	unsigned short int rows = _height;

	char shift1 = 0, shift2 = 0;
	char shift3, shift4;

	unsigned int wB = 0;
	unsigned int wF = 0;
	unsigned long long int sumB = 0;
	unsigned long long int varMax = 0;

	unsigned int wdt = xf::Inverse(cols, 16, &shift1);				//Q0.24
	unsigned int hgt = xf::Inverse(rows, 16, &shift2);				//Q0.24
	char n1, n2;
	if(shift1 > 24)
	{
		wdt = wdt >> (shift1-24);
		shift1 = 24;
	}
	else if(((shift1 >> 1) << 1) != shift1)
	{
		wdt = wdt << (24-shift1);
		shift1 = 24;
	}
	if(shift2 > 24)
	{
		hgt = hgt >> (shift2-24);
		shift2 = 24;
	}
	else if(((shift2 >> 1) << 1) != shift2)
	{
		hgt = hgt << (24-shift2);
		shift2 = 24;
	}
	shift3 = shift1 >> 1;
	shift4 = shift2 >> 1;

	tmp1 = (cols * wdt) >> shift3;									//Q0.12
	tmp2 = (rows * hgt) >> shift4;									//Q0.12
	total = tmp1 * tmp2;											//Q0.25

	HISTOGRAM_NORM_LOOP:
	for(uint16_t i = 0; i < 256 ; i++)
	{
#pragma HLS PIPELINE
		tmp1 = (ap_uint<45>)_hist[0][i];
		tmp2 = (tmp1 * wdt) >> shift3;
		HistArray[i] = (tmp2 * hgt) >> shift4;						//Histogram array is expressed in Q8.25
	}
	SUM_LOOP:
	for(uint16_t i = 0; i < 256 ; i++)
	{
#pragma HLS PIPELINE
		sum = sum + i*HistArray[i];									//sum is expressed in Q8.24
	}

	THRESHOLD_LOOP:
	for(uint16_t i = 0; i < 256; i++)
	{
#pragma HLS LOOP_TRIPCOUNT	min=256 max=256
#pragma HLS PIPELINE
		wB = wB + HistArray[i];										//wB is expressed in Q0.25
		if(wB > 0)
		{
			if((wB >> (shift3+shift4)) == 1)
				break;
			wF = total - wB;										//wF is expressed in Q0.25
			sumB = sumB + HistArray[i]*i;							//sumB is expressed in Q8.24
			unsigned int b = (wF+wB);								//b is expressed in Q0.25

			long long int a1 = (sumB*b) >> (shift3+shift4);				//a1 is expressed in Q8.25

			long long int c1 = ((long long int)sum*wB) >> (shift3+shift4);	//c1 is expressed in Q8.25
			long long int d = a1-c1;
			d = __ABS(d);

			unsigned int res = (d * d) >> (shift3+shift4+10);		//res is expressed in Q16.16

			unsigned short int x_inv1 = (unsigned short int)(wB >> 9);
			unsigned short int x_inv2 = (unsigned short int)(wF >> 9);

			unsigned int val1 = xf::Inverse(x_inv1, 0, &n1);
			unsigned int val2 = xf::Inverse(x_inv2, 0, &n2);

			unsigned long long int maxtmp = (unsigned long long int)((unsigned long long int)res * (unsigned long long int)val1) >> n1;
			unsigned long long max = (maxtmp * (unsigned long long)val2)>>n2;

			if(max > varMax)
			{
				varMax = max;
				max_val = i;
			}

		}
	}

	thresh = max_val;
}



/*********************************************************************
 * Otsuthreshold : Computes the otsu threshold for the input image
 *********************************************************************/
//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"])

template<int SRC_T, int ROWS, int COLS,int NPC=1>
void OtsuThreshold(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, uint8_t &_thresh)
{

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) )&& "NPC must be XF_NPPC1, XF_NPPC8 ");
	assert(((_src_mat.rows <= ROWS ) && (_src_mat.cols <= COLS)) && "ROWS and COLS should be greater than input image");

	uint32_t hist[XF_CHANNELS(SRC_T,NPC)][256];
	uint8_t thresh ;

#pragma HLS INLINE off
#pragma HLS interface ap_fifo port=hist

	uint16_t width = _src_mat.cols >> (XF_BITSHIFT(NPC));
	uint16_t height = _src_mat.rows;

	xFHistogramKernel<SRC_T, ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), ((COLS>>(XF_BITSHIFT(NPC)))>>1), XF_CHANNELS(SRC_T,NPC)>
	(_src_mat, hist, height, width);

	xfOtsuKernel(hist, height, _src_mat.cols, thresh);
	_thresh = thresh;
}
}

#endif //
