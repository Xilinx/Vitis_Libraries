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

#ifndef _XF_ACCUMULATE_IMAGE_HPP_
#define _XF_ACCUMULATE_IMAGE_HPP_

#include "hls_stream.h"
#include "common/xf_common.h"

#ifndef XF_IN_STEP
#define XF_IN_STEP  8
#endif
#ifndef XF_OUT_STEP
#define XF_OUT_STEP 16
#endif

namespace xf {

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC,int PLANES, int DEPTH_SRC, int DEPTH_DST, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>

int AccumulateImageKernel(
		xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<DST_T, ROWS, COLS, NPC> & dst,
		uint16_t height,uint16_t width)
{
	ap_uint<13> i,j,k,l;
	XF_TNAME(DST_T,NPC) pxl_pack_out;
	XF_TNAME(SRC_T,NPC) pxl_pack1, pxl_pack2;
	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			int y;
			pxl_pack1 = (XF_SNAME(WORDWIDTH_SRC))src1.read(i*width+j);
			pxl_pack2 = (XF_SNAME(WORDWIDTH_SRC))src2.read(i*width+j);
			ProcLoop:
			for(  k = 0, l = 0; k < ((8<< XF_BITSHIFT(NPC))*PLANES); k+=XF_IN_STEP, l+=XF_OUT_STEP)
			{
#pragma HLS UNROLL
				XF_CTUNAME(SRC_T,NPC) pxl1 = pxl_pack1.range(k+7, k);
				XF_CTUNAME(SRC_T,NPC) pxl2 = pxl_pack2.range(k+7, k);
				pxl_pack_out.range(l+XF_OUT_STEP-1, l) = (XF_CTUNAME(DST_T,NPC))pxl1 +(XF_CTUNAME(DST_T,NPC))pxl2;
			}

           // printf("%d: %d (%d, %d)\n", (int)(i*width+j), (unsigned short)pxl_pack_out, (unsigned int)pxl_pack1, (unsigned int)pxl_pack2);
			dst.write(i*width+j,pxl_pack_out);

		}
	}
	return 0;
}

//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover ("src1.data":FASTDMA,"src2.data":FASTDMA, "dst.data":FASTDMA)

#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int DST_T,int ROWS, int COLS, int NPC=1>
void accumulate(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<DST_T, ROWS, COLS, NPC> & dst)
{

	assert(((src1.rows <= ROWS ) && (src1.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((src2.rows <= ROWS ) && (src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((dst.rows <= ROWS ) && (dst.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((src1.rows == src2.rows ) && (src1.cols == src2.cols)) && "Both input images should have same size");
	assert(((src1.rows == dst.rows ) && (src1.cols == dst.cols)) && "Input and output image should be of same size");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) && "NPC must be XF_NPPC1, XF_NPPC8 ");

	uint16_t lwidth = src2.cols >> XF_BITSHIFT(NPC);

#pragma HLS INLINE OFF

	AccumulateImageKernel<SRC_T, DST_T, ROWS,COLS,NPC,XF_CHANNELS(SRC_T,NPC),XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC))> (src1,src2,dst,src1.rows,lwidth);

}
}
#endif//_XF_ACCUMULATE_IMAGE_HPP_
