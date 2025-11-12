/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef _XF_REPROJECT_3D_HPP_
#define _XF_REPROJECT_3D_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_reproject3D.hpp"
#include <fstream>
#define _ABS(x) ((x) < 0 ? -(x) : (x))

namespace xf {
namespace cv {
template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_disp = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_out = _XFCVDEPTH_DEFAULT>
void compute_matrx_mul(XF_CTUNAME(SRC_T, NPC) col,
                       XF_CTUNAME(SRC_T, NPC) row,
                       XF_CTUNAME(SRC_T, NPC) disp_val,
                       XF_CTUNAME(SRC_T, NPC) _one,
                       short int q_mtrx[16],
                       ap_int<32>* x,
                       ap_int<32>* y,
                       ap_int<32>* z /*, FILE *fph*/) {
    int temp = (col * q_mtrx[0]) + (row * q_mtrx[1]) + (disp_val * q_mtrx[2]) + (_one * q_mtrx[3]);
    int temp1 = (col * q_mtrx[4]) + (row * q_mtrx[5]) + (disp_val * q_mtrx[6]) + (_one * q_mtrx[7]);
    int temp2 = (col * q_mtrx[8]) + (row * q_mtrx[9]) + (disp_val * q_mtrx[10]) + (_one * q_mtrx[11]);
    int temp3 = (col * q_mtrx[12]) + (row * q_mtrx[13]) + (disp_val * q_mtrx[14]) + (_one * q_mtrx[15]);

    *x = ((int)temp / (int)temp3);
    *y = ((int)temp1 / (int)temp3);
    *z = ((int)temp2 / (int)temp3);
}
template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_disp = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_out = _XFCVDEPTH_DEFAULT,
          int TC>
int xFreprojectimageto3D(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_disp>& src1,
                         xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_out>& dst,
                         short int* q_mtrx,
                         int16_t min_disp,
                         bool handle_missval,
                         uint16_t height,
                         uint16_t width) {
    XF_TNAME(DST_T, NPC) pxl_pack_out;
    XF_TNAME(SRC_T, NPC) pxl_pack1, pxl_pack2;
    ap_int<32> x, y, z;
    XF_CTUNAME(SRC_T, NPC) _one = 1;
    short int q_m[16];
    for (int i = 0; i < 16; i++) {
        q_m[i] = *q_mtrx++;
    }

RowLoop:
    for (int i = 0; i < height; i++) {
// clang-format off
        #pragma HLS LOOP_FLATTEN off
        #pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on
    ColLoop:
        for (int j = 0; j < (width >> XF_BITSHIFT(NPC)); j++) {
// clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=TC max=TC
            #pragma HLS pipeline
            // clang-format on
            int y;
            pxl_pack1 = src1.read(i * (width >> XF_BITSHIFT(NPC)) + j);

        ProcLoop:
            for (int k = 0, l = 0; k < 1; k += 16, l += 16) {
// clang-format off
                #pragma HLS UNROLL
                // clang-format on
                XF_CTUNAME(SRC_T, NPC) pxl1 = pxl_pack1.range(k + 15, k);
                if (pxl1 == 0) {
                    x = 0;
                    y = 0;
                    z = -1;
                } else {
                    compute_matrx_mul<SRC_T, DST_T, ROWS, COLS, NPC, XFCVDEPTH_disp, XFCVDEPTH_out>(
                        (XF_CTUNAME(SRC_T, NPC))j, (XF_CTUNAME(SRC_T, NPC))i, pxl1, (XF_CTUNAME(SRC_T, NPC))_one, q_m,
                        (ap_int<32>*)&x, (ap_int<32>*)&y, (ap_int<32>*)&z);
                }

                pxl_pack_out.range(l + 15, 0) = (short)x;
                pxl_pack_out.range(l + 31, 16) = (short)y;
                pxl_pack_out.range(l + 47, 32) = (short)z;

                if (handle_missval) {
                    if (_ABS((short int)pxl1 - min_disp) <= 0) {
                        pxl_pack_out.range(l + 47, 32) = 10000;
                    }
                }
            }
            dst.write(i * width + j, pxl_pack_out);
        }
    }
    return 0;
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_disp = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_out = _XFCVDEPTH_DEFAULT>
void reprojectimageto3D(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_disp>& _disp_mat,
                        xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_out>& _3D_mat,
                        short int* Q_mtrx,
                        int16_t min_disp,
                        bool handle_missval = false) {
#ifndef __SYNTHESIS__
    assert(((SRC_T == XF_16UC1) || (SRC_T == XF_16SC1)) && " SRC_T must be XF_16UC1 or 16SC1");
    assert((DST_T == XF_16SC3) && " DST_T must be XF_16SC3 ");
    assert((NPC == XF_NPPC1) && " NPC must be XF_NPPC1 ");
#endif

    uint16_t rows = _disp_mat.rows;
    uint16_t cols = _disp_mat.cols >> XF_BITSHIFT(NPC);

    xFreprojectimageto3D<SRC_T, DST_T, ROWS, COLS, NPC, XFCVDEPTH_disp, XFCVDEPTH_out, (COLS >> XF_BITSHIFT(NPC))>(
        _disp_mat, _3D_mat, Q_mtrx, min_disp, handle_missval, rows, cols);
}

} // namespace cv
} // namespace xf

#endif // _XF_STEREOBM_HPP_
