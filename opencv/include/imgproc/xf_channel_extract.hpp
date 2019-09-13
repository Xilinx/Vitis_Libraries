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

#ifndef _XF_CHANNEL_EXTRACT_HPP_
#define _XF_CHANNEL_EXTRACT_HPP_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{

/*****************************************************************************
 * 	xFChannelExtract: Extracts one channel from a multiple _channel image
 *
 *	# Parameters
 *	_src	  :	 source image as stream
 *	_dst	  :	 destination image as stream
 * 	_channel :  enumeration specified in < xf_channel_extract_e >
 ****************************************************************************/
template <int ROWS, int COLS, int SRC_T, int DST_T, int NPC, int TC>
void xfChannelExtractKernel(
		xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat,
		uint16_t _channel, uint16_t height,uint16_t width)
{
//#define XF_STEP 8
const int noofbits = XF_DTPIXELDEPTH(SRC_T, NPC);

	ap_uint<13> i,j,k;
	XF_TNAME(SRC_T, NPC) in_pix;
	XF_TNAME(DST_T, NPC) out_pix;
	ap_uint<XF_PIXELDEPTH(DST_T)> result;
	int shift = 0;
	int bitdepth_src = XF_DTPIXELDEPTH(SRC_T, NPC) / XF_CHANNELS(SRC_T, NPC);
	int bitdepth_dst = XF_DTPIXELDEPTH(DST_T, NPC) / XF_CHANNELS(DST_T, NPC);

	if(_channel==XF_EXTRACT_CH_0 | _channel==XF_EXTRACT_CH_R | _channel==XF_EXTRACT_CH_Y)
	{
		shift = 0;
	}
	else if(_channel==XF_EXTRACT_CH_1 | _channel==XF_EXTRACT_CH_G | _channel==XF_EXTRACT_CH_U)
	{
		shift = noofbits;
	}
	else if(_channel==XF_EXTRACT_CH_2 | _channel==XF_EXTRACT_CH_B | _channel==XF_EXTRACT_CH_V)
	{
		shift = noofbits*2;
	}
	else if(_channel==XF_EXTRACT_CH_3 | _channel==XF_EXTRACT_CH_A)
	{
		shift = noofbits*3;
	}

	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			int y;
			in_pix = _src_mat.read(i*width+j);

			ProcLoop:
			for( k = 0; k < (noofbits<<XF_BITSHIFT(NPC)); k += noofbits)
			{
#pragma HLS unroll
				y = k * (XF_CHANNELS(SRC_T, NPC));
				result = in_pix.range(y+shift+noofbits-1, y+shift);
				out_pix.range(k+(noofbits-1), k) = result;
			}

			_dst_mat.write(i*width+j, out_pix);
		}//ColLoop
	}//RowLoop
}

//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"])
#pragma SDS data copy("_dst_mat.data"[0:"_dst_mat.size"])

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void extractChannel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat, uint16_t _channel)
{
	assert(((_channel == XF_EXTRACT_CH_0) || (_channel == XF_EXTRACT_CH_1) || (_channel == XF_EXTRACT_CH_2) ||
		    (_channel == XF_EXTRACT_CH_3) || (_channel == XF_EXTRACT_CH_R) || (_channel == XF_EXTRACT_CH_G) ||
		    (_channel == XF_EXTRACT_CH_B) || (_channel == XF_EXTRACT_CH_A) || (_channel == XF_EXTRACT_CH_Y) ||
		    (_channel == XF_EXTRACT_CH_U) || (_channel == XF_EXTRACT_CH_V)) && "Invalid Channel Value. See xf_channel_extract_e enumerated type");
	assert(((_src_mat.rows <= ROWS ) && (_src_mat.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_dst_mat.rows <= ROWS ) && (_dst_mat.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert((SRC_T == XF_8UC4 || SRC_T == XF_8UC3) && (DST_T == XF_8UC1) && "Source image should be of 4 channels and destination image of 1 channel");
//	assert(((NPC == XF_NPPC1)) && "NPC must be XF_NPPC1");

	short width=_src_mat.cols>>XF_BITSHIFT(NPC);

#pragma HLS INLINE OFF

	xfChannelExtractKernel<ROWS, COLS, SRC_T, DST_T, NPC, (COLS>>XF_BITSHIFT(NPC))>(_src_mat, _dst_mat, _channel, _src_mat.rows, width);
}
}

#endif//_XF_CHANNEL_EXTRACT_HPP_
