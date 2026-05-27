/*
 * Copyright (C) 2023 Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _XF_CONVERTTO_HPP_
#define _XF_CONVERTTO_HPP_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "hls_math.h"
#include "hls_stream.h"
#include <iostream>
#include <assert.h>

namespace xf {
namespace cv {

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC>
void compute_pxl(XF_DTUNAME(SRC_T, NPC) pxl_val, XF_DTUNAME(DST_T, NPC) & out_val, ap_ufixed<48, 24> params[4][3]) {
// clang-format off
		#pragma HLS INLINE
    // clang-format on

    if (pxl_val < params[0][0]) {
        out_val = params[0][1] * pxl_val + (int)params[0][2];

    } else if (pxl_val < params[1][0]) {
        out_val = params[1][1] * pxl_val + (int)params[1][2];

    } else if (pxl_val < params[2][0]) {
        out_val = params[2][1] * pxl_val + (int)params[2][2];

    } else {
        out_val = params[3][1] * pxl_val + (int)params[3][2];
    }
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int TC>
void xFcompute(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
               ap_ufixed<48, 24> params[3][4][3],
               unsigned short bayerp,
               int rows,
               int cols) {
// clang-format off
		#pragma HLS INLINE OFF
		// clang-format on				
		
		constexpr int PXL_WIDTH_IN = XF_PIXELWIDTH(SRC_T, NPC);
		constexpr int PXL_WIDTH_OUT = XF_PIXELWIDTH(DST_T, NPC);
		
		int rd_ptr = 0,wr_ptr = 0, row_idx, col_idx;
		ap_uint<2> row_rem, col_rem, sum_rem, row_incr, col_incr, color_idx;
        
		ap_int<13> i,j,k;
		
		XF_TNAME(SRC_T, NPC) val_src;
		XF_TNAME(DST_T, NPC) val_dst;
		
		XF_DTUNAME(SRC_T, NPC) pxl_in;
		XF_DTUNAME(DST_T, NPC) pxl_out;
        
        row_incr = 0;
        col_incr = 0;

        if((bayerp == XF_BAYER_GB) || (bayerp == XF_BAYER_RG))
            col_incr = 1;
        else col_incr = 0;
        
        if((bayerp == XF_BAYER_GR) || (bayerp == XF_BAYER_RG))
            row_incr = 1;
        else row_incr = 0;
		
	rowLoop:
		for (i = 0; i < rows; i++) {
	// clang-format off
			#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
			#pragma HLS LOOP_FLATTEN off
        // clang-format on

        row_idx = i + row_incr;
        row_rem = row_idx & 0x00000001;

    colLoop:
        for (j = 0; j < cols; j++) {
// clang-format off
				#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
				#pragma HLS PIPELINE II=1
            // clang-format on

            val_src = src.read(rd_ptr++);

        procLoop:
            for (k = 0; k < XF_NPIXPERCYCLE(NPC); k++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                pxl_in = val_src.range((k + 1) * PXL_WIDTH_IN - 1,
                                       k * PXL_WIDTH_IN); // Get bits from certain range of positions.

                col_idx = j * NPC + k + col_incr;

                col_rem = col_idx & 0x00000001;

                color_idx = row_rem + col_rem;

                compute_pxl<SRC_T, DST_T, ROWS, COLS, NPC>(pxl_in, pxl_out, params[color_idx]);

                val_dst.range((k + 1) * PXL_WIDTH_OUT - 1, k * PXL_WIDTH_OUT) = pxl_out;
            }

            dst.write(wr_ptr++, val_dst);
        }
    }
    return;
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void convert24To14bit(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
                      xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                      ap_ufixed<48, 24> params[3][4][3],
                      unsigned short bayerp) {
#ifndef __SYNTHESIS__
    assert(((bayerp == XF_BAYER_BG) || (bayerp == XF_BAYER_GB) || (bayerp == XF_BAYER_GR) || (bayerp == XF_BAYER_RG)) &&
           ("Unsupported Bayer pattern. Use anyone among: "
            "XF_BAYER_BG;XF_BAYER_GB;XF_BAYER_GR;XF_BAYER_RG"));
    assert(((SRC_T == XF_24UC1) || (SRC_T == XF_32UC1)) && "Input TYPE must be XF_12UC1 or XF_16UC1");
    assert((DST_T == XF_14UC1) || (DST_T == XF_16UC1) && "OUTPUT TYPE must be XF_32UC1 or XF_24UC1");
    assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC2)) && "NPC must be XF_NPPC1, XF_NPPC2 ");
    assert((src.rows <= ROWS) && (src.cols <= COLS) && "ROWS and COLS should be greater than input image size ");
#endif
    int rows = src.rows;
    int cols = src.cols;

    uint16_t cols_shifted = cols >> (XF_BITSHIFT(NPC));
    ap_ufixed<48, 24> copy_params[3][4][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 3; k++) {
                copy_params[i][j][k] = params[i][j][k];
            }
        }
    }

#pragma HLS ARRAY_PARTITION variable = copy_params complete dim = 0
#pragma HLS ARRAY_PARTITION variable = copy_params complete dim = 1
#pragma HLS ARRAY_PARTITION variable = copy_params complete dim = 2

    xFcompute<SRC_T, DST_T, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_OUT, (COLS >> (XF_BITSHIFT(NPC)))>(
        src, dst, copy_params, bayerp, rows, cols_shifted);

    return;
}

} // namespace cv
} // namespace xf

#endif //_XF_CONVERTTO_HPP_
