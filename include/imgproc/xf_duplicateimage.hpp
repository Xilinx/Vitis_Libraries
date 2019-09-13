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

#ifndef _XF_Duplicate_HPP_
#define _XF_Duplicate_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf {
template<int ROWS, int COLS,int SRC_T, int DEPTH, int NPC, int WORDWIDTH>
void xFDuplicate(hls::stream< XF_TNAME(SRC_T,NPC) > &_src_mat,
				 hls::stream< XF_TNAME(SRC_T,NPC) > &_dst1_mat,
				 hls::stream< XF_TNAME(SRC_T,NPC) > &_dst2_mat, uint16_t img_height, uint16_t img_width)
{
	img_width = img_width >> XF_BITSHIFT(NPC);

	ap_uint<13> row, col;
	Row_Loop:
	for(row = 0 ; row < img_height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		Col_Loop:
		for(col = 0; col < img_width; col++)
		{
#pragma HLS LOOP_TRIPCOUNT min=240 max=240
#pragma HLS pipeline
			XF_TNAME(SRC_T,NPC) tmp_src;
			tmp_src = _src_mat.read();
			_dst1_mat.write(tmp_src);
			_dst2_mat.write(tmp_src);
		}
	}
}



#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data access_pattern("_dst1.data":SEQUENTIAL)
#pragma SDS data copy("_dst1.data"[0:"_dst1.size"])
#pragma SDS data access_pattern("_dst2.data":SEQUENTIAL)
#pragma SDS data copy("_dst2.data"[0:"_dst2.size"])

template<int SRC_T, int ROWS, int COLS,int NPC>
void duplicateMat(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst1,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst2)
{
#pragma HLS inline off

#pragma HLS dataflow

	hls::stream<XF_TNAME(SRC_T,NPC)>src;
	hls::stream< XF_TNAME(SRC_T,NPC)> dst;
	hls::stream< XF_TNAME(SRC_T,NPC)> dst1;

	/********************************************************/

	Read_yuyv_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write(_src.read (i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFDuplicate< ROWS, COLS,SRC_T, XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC)>(src,dst,dst1, _src.rows,_src.cols);

	for(int i=0; i<_dst1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_dst1.write((i*(_dst1.cols>>(XF_BITSHIFT(NPC))) +j),dst.read());
			_dst2.write((i*(_dst2.cols>>(XF_BITSHIFT(NPC))) +j),dst1.read());

		}
	}
}
}
#endif
