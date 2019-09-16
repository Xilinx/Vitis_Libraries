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

#ifndef _XF_INTEGRAL_IMAGE_HPP_
#define _XF_INTEGRAL_IMAGE_HPP_

typedef unsigned short  uint16_t;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{

template<int SRC_TYPE, int DST_TYPE, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void XFIntegralImageKernel(
		xf::Mat<SRC_TYPE, ROWS, COLS, NPC> & _src_mat, xf::Mat<DST_TYPE, ROWS, COLS, NPC> & _dst_mat,
		uint16_t height, uint16_t width)
{
#pragma HLS inline
	XF_SNAME(XF_32UW) linebuff[COLS];

	uint32_t cur_sum;
	ap_uint<22> prev;
	ap_uint<13> i, j;
	RowLoop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		XF_SNAME(XF_8UW) val;cur_sum = 0; prev = 0;

		ColLoop:
		for(j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS PIPELINE

			val = (XF_SNAME(XF_8UW))_src_mat.read(i*width+j);

			prev = prev + val;

			if(i == 0)
			{
				cur_sum = prev;
			}
			else
			{
				cur_sum = (prev + linebuff[j]);
			}

			linebuff[j] = cur_sum;
			_dst_mat.write(i*width+j,cur_sum);
		}//ColLoop

	}//rowLoop
}


// XFIntegralImage

//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_mat.data":AXIDMA_SIMPLE)
#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])

template<int SRC_TYPE,int DST_TYPE, int ROWS, int COLS, int NPC>
void integral(xf::Mat<SRC_TYPE, ROWS, COLS, NPC> & _src_mat, xf::Mat<DST_TYPE, ROWS, COLS, NPC> & _dst_mat)
{
#pragma HLS INLINE OFF
	assert((( NPC == XF_NPPC1 )) && "NPC must be XF_NPPC1");
	assert(((_src_mat.rows <= ROWS ) && (_dst_mat.cols <= COLS)) && "ROWS and COLS should be greater than or equal to input image size");

	XFIntegralImageKernel<SRC_TYPE, DST_TYPE, ROWS,COLS,NPC,XF_WORDWIDTH(SRC_TYPE,NPC),XF_WORDWIDTH(DST_TYPE,NPC),(COLS>>XF_BITSHIFT(NPC))>(_src_mat,_dst_mat,_src_mat.rows,_src_mat.cols);

}
}

#endif//_XF_INTEGRAL_IMAGE_HPP_

