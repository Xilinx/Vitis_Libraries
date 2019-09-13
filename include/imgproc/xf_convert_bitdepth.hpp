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

#ifndef _XF_CONVERT_BITDEPTH_HPP_
#define _XF_CONVERT_BITDEPTH_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"

namespace xf{
/**
 *   xfConvertBitDepthKernel : Converts the input image bit depth to specified bit depth
 *  _src_mat  : Input  image
 *  _dst_mat : Output image
 *  _convert_type : conversion type
 *  _shift : scale factor
 */
template<int SRC_T, int DST_T, int ROWS,int COLS,int DEPTH_SRC, int DEPTH_DST, int WORDWIDTH_SRC,  int WORDWIDTH_DST, int NPC, int TRIP_CNT>
void xfConvertBitDepthKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> &_src_mat, xf::Mat<DST_T, ROWS, COLS, NPC> &_dst_mat,
		ap_uint<4> _convert_type, int _shift, unsigned short _height, unsigned short _width)
{
	XF_SNAME(WORDWIDTH_SRC)   buf;
	XF_SNAME(WORDWIDTH_DST)  result;
	int min, max;
	ap_uint<13> col,row;
	ap_uint<10> j,k,i;
	ap_uint<10> out_step,in_step;
	if(DEPTH_DST == XF_8UP)
	{
		min = 0; max = 255;
	}
	else if(DEPTH_DST == XF_16UP)
	{
		min = 0; max = 65535;
	}
	else if(DEPTH_DST == XF_16SP)
	{
		min = -32768; max = 32767;
	}
	else if(DEPTH_DST == XF_32SP)
	{
		min = -2147483648; max = 2147483647;
	}

	ROW_LOOP:
	for( row = 0; row < _height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS


		COL_LOOP:
		for( col = 0; col < _width; col++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TRIP_CNT max=TRIP_CNT
#pragma HLS LOOP_FLATTEN off
#pragma HLS pipeline

			buf = (XF_SNAME(WORDWIDTH_SRC))(_src_mat.data[row*_width+col]);
			out_step = XF_PIXELDEPTH(DEPTH_DST),
					in_step  = XF_PIXELDEPTH(DEPTH_SRC);

			Extract:
			for( j = 0, k = 0, i = 0; j<(1 << XF_BITSHIFT(NPC)); j++, k += in_step, i += out_step)
			{
#pragma HLS UNROLL

				XF_PTNAME(DEPTH_SRC) val = buf.range(k+(in_step-1), k);

				if(_convert_type == XF_CONVERT_16U_TO_8U  ||
						_convert_type == XF_CONVERT_16S_TO_8U  ||
						_convert_type == XF_CONVERT_32S_TO_8U  ||
						_convert_type == XF_CONVERT_32S_TO_16U ||
						_convert_type == XF_CONVERT_32S_TO_16S)
				{
					val = val >> _shift;
					if(val < min)
						val = min;
					if(val > max)
						val = max;
					result(i+(out_step-1), i) = (XF_PTNAME(DEPTH_DST))val;
				}
				else
				{
					if(((XF_PTNAME(DEPTH_DST))val << _shift) > max)
						result(i+(out_step-1), i) = max;
					else if(((XF_PTNAME(DEPTH_DST))val << _shift) < min)
						result(i+(out_step-1), i) = min;
					else
						result(i+(out_step-1), i) = (XF_PTNAME(DEPTH_DST))val << _shift;
				}
			}
			_dst_mat.data[row*_width+col] = ((XF_SNAME(WORDWIDTH_DST))result);
		}
	}
}


//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL,"_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"])
#pragma SDS data copy("_dst_mat.data"[0:"_dst_mat.size"])
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void convertTo(xf::Mat<SRC_T, ROWS, COLS, NPC> &_src_mat, xf::Mat<DST_T, ROWS, COLS, NPC> &_dst_mat, ap_uint<4> _convert_type, int _shift)
{

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
				"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((_src_mat.rows <= ROWS ) && (_src_mat.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_dst_mat.rows <= ROWS ) && (_dst_mat.cols <= COLS)) && "ROWS and COLS should be greater than input image");

	assert(( ((_convert_type == XF_CONVERT_16U_TO_8U)  || (_convert_type == XF_CONVERT_16S_TO_8U)
			|| ( _convert_type == XF_CONVERT_32S_TO_8U) || ( _convert_type == XF_CONVERT_32S_TO_16S)
			|| (_convert_type == XF_CONVERT_32S_TO_16U) || (_convert_type == XF_CONVERT_8U_TO_16U)
			|| (_convert_type == XF_CONVERT_8U_TO_16S)  || (_convert_type == XF_CONVERT_8U_TO_32S)
			|| (_convert_type == XF_CONVERT_16U_TO_32S) || (_convert_type == XF_CONVERT_16S_TO_32S))
			&& " conversion type is not valid "));

#pragma HLS INLINE OFF

	uint16_t width = _src_mat.cols>>(XF_BITSHIFT(NPC));
	uint16_t height = _src_mat.rows;

	xfConvertBitDepthKernel<SRC_T, DST_T, ROWS, COLS, XF_DEPTH(SRC_T, NPC), XF_DEPTH(DST_T, NPC), XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(DST_T,NPC), NPC,(COLS>>XF_BITSHIFT(NPC))>
	(_src_mat, _dst_mat, _convert_type, _shift, height, width);


}
}



#endif  // _XF_CONVERT_BITDEPTH_HPP_
