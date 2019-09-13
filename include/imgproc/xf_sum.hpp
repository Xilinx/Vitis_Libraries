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

#ifndef _XF_ACCUMULATE_WEIGHTED_HPP_
#define _XF_ACCUMULATE_WEIGHTED_HPP_

#include "hls_stream.h"
#include "common/xf_common.h"

#ifndef XF_IN_STEP
#define XF_IN_STEP  8
#endif
#ifndef XF_OUT_STEP
#define XF_OUT_STEP 16
#endif
namespace xf {
template<int SRC_T, int ROWS, int COLS, int NPC,int PLANES, int DEPTH_SRC,int WORDWIDTH_SRC, int TC>
int sumKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1,  xf::Scalar<XF_CHANNELS(SRC_T,NPC), double> &scl, uint16_t height,uint16_t width)
{

	ap_uint<13> i,j,k,l,c;

	ap_uint<64> internal_sum[PLANES];
	for(int i=0;i<PLANES;i++)
	{
#pragma HLS unroll
		internal_sum[i]=0;
	}

	int STEP= XF_PIXELDEPTH(DEPTH_SRC)/PLANES;

	XF_SNAME(WORDWIDTH_SRC)  pxl_pack1, pxl_pack2;
	RowLoop:



	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN OFF
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

			pxl_pack1 = (XF_SNAME(WORDWIDTH_SRC))(src1.read(i*width+j));
			ProcLoop:
			for( k = 0,c=0; k < ((8<<XF_BITSHIFT(NPC))*PLANES); k+=XF_IN_STEP,c++ )
			{
				XF_PTNAME(DEPTH_SRC) pxl1 = pxl_pack1.range(k+7, k);

				if(PLANES==1)
				{
				internal_sum[0] = internal_sum[0]+pxl1;
				}
				else
				{
					internal_sum[c] = internal_sum[c]+pxl1;
				}

			}


		}
	}
	if(PLANES==1)
	{
	scl.val[0] = (ap_uint<64>)internal_sum[0];
	}
	else
	{
		scl.val[0] = (ap_uint<64>)internal_sum[0];
		scl.val[1] = (ap_uint<64>)internal_sum[1];
		scl.val[2] = (ap_uint<64>)internal_sum[2];
	}
	return 0;
}


#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
template< int SRC_T, int ROWS, int COLS, int NPC = 1>
void sum(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, double sum[XF_CHANNELS(SRC_T,NPC)] )
{
	assert(((SRC_T == XF_8UC1)) && "Input TYPE must be XF_8UC1 for 1-channel image");
	assert(((src1.rows <= ROWS ) && (src1.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) && "NPC must be XF_NPPC1, XF_NPPC8 ");

	short width = src1.cols  >> XF_BITSHIFT(NPC);
	xf::Scalar<XF_CHANNELS(SRC_T,NPC), double> scl;

	sumKernel<SRC_T, ROWS, COLS,NPC ,XF_CHANNELS(SRC_T,NPC),XF_DEPTH(SRC_T,NPC),XF_WORDWIDTH(SRC_T,NPC),(COLS>>XF_BITSHIFT(NPC))>(src1, scl, src1.rows, width);
	for(int i=0;i<XF_CHANNELS(SRC_T,NPC);i++)
	{
		sum[i]=scl.val[i];
	}
}
}
#endif//_XF_SUM_HPP_
