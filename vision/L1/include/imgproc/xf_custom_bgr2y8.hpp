/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _XF_CUSTOM_BGR2Y8_HPP_
#define _XF_CUSTOM_BGR2Y8_HPP_

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_bgr2hsv.hpp"
#include "hls_stream.h"

namespace xf {
namespace cv {

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT,
          int TC>
void hsv2y8(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src_mat,
            xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& _dst_mat,
            struct bgr2y8_params params) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    XF_SNAME(XF_WORDWIDTH(SRC_T, NPC)) in_pix;
    XF_SNAME(XF_WORDWIDTH(DST_T, NPC)) out_pix;
    ap_uint<8> h, s, v;

    int rows = _src_mat.rows;
    int cols = (_src_mat.cols >> XF_BITSHIFT(NPC));
    int H_degree, V_per, sat_per;
    int p, l;

    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);
    const int STEP_OUT = XF_DTPIXELDEPTH(DST_T, NPC);

    for (uint16_t row = 0; row < rows; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
        // clang-format on
        for (uint16_t col = 0; col < cols; col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS PIPELINE
            // clang-format on
            in_pix = _src_mat.read(row * cols + col);

            for (p = 0, l = 0; p < (XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC));
                 p = p + XF_CHANNELS(SRC_T, NPC), l++) {
// clang-format off
					#pragma HLS UNROLL
                // clang-format on

                h = in_pix.range(p * STEP + STEP - 1, p * STEP);
                s = in_pix.range(p * STEP + (2 * STEP) - 1, p * STEP + STEP);
                v = in_pix.range(p * STEP + (3 * STEP) - 1, p * STEP + 2 * STEP);

                H_degree = h * 2;
                V_per = (v * 100) / 255;
                sat_per = (s * 100) / 255;

                unsigned char max = v;

                if ((V_per < params.black_Vmax) && (sat_per < params.black_Smax)) { // obvious defect and background
                    out_pix.range(l * STEP_OUT + STEP_OUT - 1, l * STEP_OUT) = 0;
                } else {
                    // defect Brown color range
                    if ((H_degree < params.brown_Hmax) && (V_per < params.brown_Vmax) && (sat_per < params.Smax) &&
                        (sat_per > params.Smin))
                        out_pix.range(l * STEP_OUT + STEP_OUT - 1, l * STEP_OUT) = 0;

                    // dark green color range
                    else if ((H_degree > params.darkgreen_Hmin) && (H_degree < params.darkgreen_Hmax) &&
                             (V_per < params.darkgreen_Vmax) && (sat_per < params.Smax) && (sat_per > params.Smin))
                        out_pix.range(l * STEP_OUT + STEP_OUT - 1, l * STEP_OUT) = 0;

                    // dimmer region enhancement
                    else if ((H_degree > params.green_Hmin) && (H_degree < params.green_Hmax) &&
                             (V_per < params.green_Vmax))
                        out_pix.range(l * STEP_OUT + STEP_OUT - 1, l * STEP_OUT) = (max + 30) < 256 ? (max + 30) : 255;

                    else // other cases
                        out_pix.range(l * STEP_OUT + STEP_OUT - 1, l * STEP_OUT) = max;
                }
            }

            _dst_mat.write(row * cols + col, out_pix);
        }
    }
    return;
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT>
void custom_bgr2y8(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src_mat,
                   xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& _dst_mat,
                   struct bgr2y8_params params) {
// clang-format off
#pragma HLS INLINE OFF
// clang-format on

#ifndef __SYNTHESIS__
    assert(((SRC_T == XF_8UC3) && (DST_T == XF_8UC1)) &&
           "Input and Output TYPE must be XF_8UC1 for 1-channel, XF_8UC3 for 3-channel");
    assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC2) || (NPC == XF_NPPC4) || (NPC == XF_NPPC8)) &&
           "NPC must be XF_NPPC1,XF_NPPC2, XF_NPPC4, XF_NPPC8 ");
    assert((_src_mat.rows <= ROWS) && (_src_mat.cols <= COLS) &&
           "ROWS and COLS should be greater or equal to input image size ");
#endif

    int rows = _src_mat.rows;
    int cols = _src_mat.cols;

    xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1> src_hsv(rows, cols);

// clang-format off
		#pragma HLS DATAFLOW
    // clang-format on

    // Convert BGR to XYZ:
    xf::cv::bgr2hsv<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(_src_mat, src_hsv);
    xf::cv::hsv2y8<SRC_T, DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1, XFCVDEPTH_OUT_1, (COLS >> XF_BITSHIFT(NPC))>(
        src_hsv, _dst_mat, params);
}
} // namespace cv
} // namespace xf
#endif
