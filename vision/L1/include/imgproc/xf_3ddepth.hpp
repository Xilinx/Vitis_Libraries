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

#ifndef _XF_3D_DEPTH_FLOAT_HPP_
#define _XF_3D_DEPTH_FLOAT_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
//#include "imgproc/xf_reproject3D_float.hpp"
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
          int XFCVDEPTH_out = _XFCVDEPTH_DEFAULT,
          int TC>
int xFdepth3D(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_disp>& src1,
              xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_out>& dst,
              float focal_len,
              float base_dis,
              uint16_t height,
              uint16_t width) {
    XF_TNAME(SRC_T, NPC) pxl_pack1;
    XF_TNAME(DST_T, NPC) pxl_pack_out;
    float out_map;
    float nu = focal_len * base_dis;

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
            for (int k = 0; k < 1; k += 16) {
// clang-format off
                #pragma HLS UNROLL
                // clang-format on
                XF_CTUNAME(SRC_T, NPC) pxl1 = pxl_pack1.range(k + 15, k);
                if (pxl1 == 0) {
                    out_map = 0.0f;

                } else {
                    out_map = nu / pxl1;
                }

                pxl_pack_out.range(31, 0) = *((unsigned int*)(&out_map));
            }
            dst.write(i * width + j, pxl_pack_out);
        }
    }

    // printf("reprojectimageto3d is done\n");
    return 0;
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_disp = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_out = _XFCVDEPTH_DEFAULT>
void depth3D(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_disp>& _disp_mat,
             xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_out>& _depth_mat,
             float focal_len,
             float base_dis) {
#ifndef __SYNTHESIS__
    assert(((SRC_T == XF_16UC1) || (SRC_T == XF_16SC1)) && " SRC_T must be XF_16UC1 or 16SC1");
    assert((DST_T == XF_32FC1) && " DST_T must be XF_32FC1 ");
    assert((NPC == XF_NPPC1) && " NPC must be XF_NPPC1 ");
#endif
    uint16_t rows = _disp_mat.rows;
    uint16_t cols = _disp_mat.cols >> XF_BITSHIFT(NPC);
    xFdepth3D<SRC_T, DST_T, ROWS, COLS, NPC, XFCVDEPTH_disp, XFCVDEPTH_out, (COLS >> XF_BITSHIFT(NPC))>(
        _disp_mat, _depth_mat, focal_len, base_dis, rows, cols);
}

} // namespace cv
} // namespace xf

#endif // _XF_STEREOBM_HPP_
