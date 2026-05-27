/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef _XF_DEGAMMA_HPP_
#define _XF_DEGAMMA_HPP_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "hls_math.h"
#include "hls_stream.h"
#include "xf_cvt_color.hpp"
#include <iostream>
#include <assert.h>

namespace xf {
namespace cv {

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int N>
void compute_pxl(XF_DTUNAME(SRC_T, NPC) pxl_val, XF_DTUNAME(DST_T, NPC) & out_val, uint32_t params[3][N][3], int idx) {
// clang-format off
		#pragma HLS INLINE
    // clang-format on
    bool flag = 0;
    // int pxl_val_32 = (int)(pxl_val);

    for (int i = 0; i < N / 4; i++) {
        if ((i > 0) && pxl_val < params[idx][i * 4 - 1][0]) {
            break;
        } else if (pxl_val < params[idx][i * 4][0]) {
            out_val = ((params[idx][i * 4][1] * (uint32_t)(pxl_val)) >> 14) - (uint32_t)params[idx][i * 4][2];

        } else if (pxl_val < params[idx][i * 4 + 1][0]) {
            out_val = ((params[idx][i * 4 + 1][1] * (uint32_t)(pxl_val)) >> 14) - (uint32_t)params[idx][i * 4 + 1][2];
        } else if (pxl_val < params[idx][i * 4 + 2][0]) {
            out_val = ((params[idx][i * 4 + 2][1] * (uint32_t)(pxl_val)) >> 14) - (uint32_t)params[idx][i * 4 + 2][2];
        } else if (pxl_val < params[idx][i * 4 + 3][0]) {
            out_val = ((params[idx][i * 4 + 3][1] * (uint32_t)(pxl_val)) >> 14) - (uint32_t)params[idx][i * 4 + 3][2];
        }
    }
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int TC,
          int N>
void xFcompute(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
               uint32_t params[3][N][3],
               unsigned short bayerp,
               int rows,
               int cols) {
// clang-format off
		#pragma HLS INLINE OFF
		// clang-format on				
		
		constexpr int PXL_WIDTH_IN = XF_PIXELWIDTH(SRC_T, NPC);
		constexpr int PXL_WIDTH_OUT = XF_PIXELWIDTH(DST_T, NPC);
		
		int rd_ptr = 0,wr_ptr = 0, row_idx, col_idx,  color_idx;
		
		ap_int<13> i,j,k;
		
		ap_uint<24> val_out;
		
		XF_TNAME(SRC_T, NPC) val_src;
		XF_TNAME(DST_T, NPC) val_dst;
		
		XF_DTUNAME(SRC_T, NPC) pxl_in;
		XF_DTUNAME(DST_T, NPC) pxl_out;
	rowLoop:
		for (i = 0; i < rows; i++) {
	// clang-format off
			#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
			#pragma HLS LOOP_FLATTEN off
    // clang-format on
    colLoop:
        for (j = 0; j < cols; j++) {
// clang-format off
				#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
				#pragma HLS PIPELINE
            // clang-format on

            val_src = src.read(rd_ptr++);
            row_idx = i;
            col_idx = 0;

        procLoop:
            for (k = 0; k < XF_NPIXPERCYCLE(NPC); k++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                pxl_in = val_src.range((k + 1) * PXL_WIDTH_IN - 1,
                                       k * PXL_WIDTH_IN); // Get bits from certain range of positions.

                col_idx = j * NPC + k;

                if (bayerp == XF_BAYER_GB) {
                    col_idx += 1;
                }
                if (bayerp == XF_BAYER_GR) {
                    row_idx += 1;
                }
                if (bayerp == XF_BAYER_RG) {
                    col_idx += 1;
                    row_idx += 1;
                }
                if ((row_idx & 0x00000001) == 0) {     // even row
                    if ((col_idx & 0x00000001) == 0) { // even col
                        color_idx = 0;                 // R location
                        compute_pxl<SRC_T, DST_T, ROWS, COLS, NPC, N>(pxl_in, pxl_out, params, color_idx);
                    } else {           // odd col
                        color_idx = 1; // G location
                        compute_pxl<SRC_T, DST_T, ROWS, COLS, NPC, N>(pxl_in, pxl_out, params, color_idx);
                    }
                } else {                               // odd row
                    if ((col_idx & 0x00000001) == 0) { // even col
                        color_idx = 1;                 // G location
                        compute_pxl<SRC_T, DST_T, ROWS, COLS, NPC, N>(pxl_in, pxl_out, params, color_idx);
                    } else {           // odd col
                        color_idx = 2; // B location
                        compute_pxl<SRC_T, DST_T, ROWS, COLS, NPC, N>(pxl_in, pxl_out, params, color_idx);
                    }
                }

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
          int N,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void degamma(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
             xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
             uint32_t params[3][N][3],
             unsigned short bayerp) {
#ifndef __SYNTHESIS__
    assert(((bayerp == XF_BAYER_BG) || (bayerp == XF_BAYER_GB) || (bayerp == XF_BAYER_GR) || (bayerp == XF_BAYER_RG)) &&
           ("Unsupported Bayer pattern. Use anyone among: "
            "XF_BAYER_BG;XF_BAYER_GB;XF_BAYER_GR;XF_BAYER_RG"));
    assert(((SRC_T == XF_8UC1) || (SRC_T == XF_14UC1) || (SRC_T == XF_16UC1)) &&
           "Input TYPE must be XF_8UC1 or XF_14UC1 or XF_16UC1");
    assert(((DST_T == XF_8UC1) || (DST_T == XF_14UC1) || (DST_T == XF_16UC1)) &&
           "OUTPUT TYPE must be XF_8UC1 or XF_14UC1 or XF_16UC1");
    assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC2) || (NPC == XF_NPPC4) || (NPC == XF_NPPC8)) &&
           "NPC must be XF_NPPC1, XF_NPPC2 ");
    assert((src.rows <= ROWS) && (src.cols <= COLS) && "ROWS and COLS should be greater than input image size ");
#endif
    int rows = src.rows;
    int cols = src.cols;

    uint16_t cols_shifted = cols >> (XF_BITSHIFT(NPC));
    uint32_t copy_params[3][N][3];

    /*  params values are in Q18.14 format  */

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < 3; k++) {
                // float params_fixed = (float)(params[i][j][k]) / (1 << 14);
                // copy_params[i][j][k] = (ap_ufixed<32, 18>)params_fixed;

                copy_params[i][j][k] = params[i][j][k];
            }
        }
    }
#pragma HLS ARRAY_PARTITION variable = copy_params complete dim = 0

    xFcompute<SRC_T, DST_T, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_OUT, (COLS >> (XF_BITSHIFT(NPC))), N>(
        src, dst, copy_params, bayerp, rows, cols_shifted);

    return;
}

////////////Multi//////////////////
template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int N,
          int STREAMS = 2>
void degamma_multi(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
                   xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                   unsigned int dgam_params[STREAMS][3][N][3],
                   unsigned short dgam_bayer[STREAMS],
                   int strm_id) {
// clang-format off
#pragma HLS ARRAY_PARTITION variable= dgam_params dim=1 complete
#pragma HLS ARRAY_PARTITION variable= dgam_bayer dim=1 complete
   // clang-format on                
   degamma<SRC_T, DST_T, ROWS, COLS, NPC, N, XFCVDEPTH_IN, XFCVDEPTH_OUT>(src, dst, dgam_params[strm_id],
                                          dgam_bayer[strm_id]);             

}
} // namespace cv
} // namespace xf

#endif