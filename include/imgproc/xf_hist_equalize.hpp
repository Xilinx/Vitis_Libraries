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

#ifndef _XF_HIST_EQUALIZE_HPP_
#define _XF_HIST_EQUALIZE_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_histogram.hpp"

/**
 *  xfEqualize : Computes the histogram and performs
 *               Histogram Equalization
 *  _src1	: Input image
 *  _dst_mat	: Output image
 */
namespace xf{

template<int SRC_T, int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int SRC_TC>
void xFEqualize(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,uint32_t hist_stream[0][256], xf::Mat<SRC_T, ROWS, COLS, NPC> &_dst_mat,
		uint16_t img_height, uint16_t img_width)
{
	XF_SNAME (WORDWIDTH)
	in_buf, temp_buf;
	// Array to hold the values after cumulative distribution
	ap_uint<8> cum_hist[256];
#pragma HLS ARRAY_PARTITION variable=cum_hist complete dim=1
	// Temporary array to hold data
	ap_uint<8> tmp_cum_hist[(1 << XF_BITSHIFT(NPC))][256];
#pragma HLS ARRAY_PARTITION variable=tmp_cum_hist complete dim=1
	// Array which holds histogram of the image

	/*	Normalization	*/
	uint32_t temp_val =(uint32_t) (img_height * (img_width << XF_BITSHIFT(NPC)));
	uint32_t init_val = (uint32_t)(temp_val - hist_stream[0][0]);
	uint32_t scale;
	if (init_val == 0) {
		scale = 0;
	} else {
		scale = (uint32_t)(((1 << 31)) / init_val);
	}

	ap_uint<40> scale1 = (ap_uint<40>) ((ap_uint<40>)255 * (ap_uint<40>)scale);
	ap_uint32_t temp_sum = 0;

	cum_hist[0] = 0;
	Normalize_Loop: for (ap_uint<9> i = 1; i < 256; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
#pragma HLS PIPELINE
		temp_sum = (uint32_t)temp_sum + (uint32_t)hist_stream[0][i];
		uint64_t sum = (uint64_t)((uint64_t)temp_sum * (uint64_t)scale1);
		sum = (uint64_t)(sum + 0x40000000);
		cum_hist[i] = sum >> 31;
	}


	for (ap_uint<9> i = 0; i < 256; i++) {
#pragma HLS PIPELINE
		for (ap_uint<5> j = 0; j < (1 << XF_BITSHIFT(NPC)); j++) {
#pragma HLS UNROLL
			ap_uint<8> tmpval = cum_hist[i];
			tmp_cum_hist[j][i] = tmpval;
		}
	}




	NORMALISE_ROW_LOOP: for (ap_uint<13> row = 0; row < img_height; row++) {
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		NORMALISE_COL_LOOP: for (ap_uint<13> col = 0; col < img_width; col++) {
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max=SRC_TC
#pragma HLS PIPELINE
#pragma HLS LOOP_FLATTEN OFF
			in_buf = _src1.read(row*img_width+col);
			Normalise_Extract: for (ap_uint<9> i = 0, j = 0; i < (8 << XF_BITSHIFT(NPC));
					j++, i += 8) {
#pragma HLS DEPENDENCE variable=tmp_cum_hist array intra false
#pragma HLS unroll
				XF_PTNAME (DEPTH)
				val;
				val = in_buf.range(i + 7, i);
				temp_buf(i + 7, i) = tmp_cum_hist[j][val];
			}
			_dst_mat.write(row*img_width+col,temp_buf);
		}
	}
}


/****************************************************************
 * equalizeHist : Wrapper function which calls the main kernel
 ****************************************************************/
//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst.data":AXIDMA_SIMPLE)

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)

#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void equalizeHist(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS inline off

	uint16_t img_height = _src1.rows;
	uint16_t img_width = _src1.cols;
	assert(((img_height <= ROWS) && (img_width <= COLS))
					&& "ROWS and COLS should be greater than input image");

	assert((SRC_T == XF_8UC1) && "Type must be of XF_8UC1");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8))
					&& " NPC must be XF_NPPC1, XF_NPPC8");

	uint32_t histogram[1][256];

	img_width = img_width >> XF_BITSHIFT(NPC);
	xFHistogramKernel<SRC_T, ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), ((COLS>>(XF_BITSHIFT(NPC)))>>1), XF_CHANNELS(SRC_T,NPC)>
		(_src, histogram, img_height, img_width);


	xFEqualize<SRC_T, ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), (COLS >> XF_BITSHIFT(NPC))>
	(_src1,histogram, _dst, img_height, img_width);

}
}//namespace
#endif // _XF_HIST_EQUALIZE_H_
