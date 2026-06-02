/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#ifndef _XF_STITCH_HPP_
#define _XF_STITCH_HPP_

#include <ostream>
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "hls_stream.h"

namespace xf {
namespace cv {

// Stitch module declarations / utilities can be added here.

inline int stitch_roi(int img_sizes[8], int mask_corners[8], int& dst_height, int& dst_width) {
    // Compute the minimum top-left x (tl.x) and y (tl.y)
    int tl_x = mask_corners[0];
    int tl_y = mask_corners[1];

    // Compute bottom-right (br.x, br.y) from (corner.x + size.width, corner.y + size.height)
    int br_x = mask_corners[0] + img_sizes[1];
    int br_y = mask_corners[1] + img_sizes[0];
    for (int j = 1; j < 4; ++j) {
        int corner_x = mask_corners[j * 2];
        int corner_y = mask_corners[j * 2 + 1];
        int size_h = img_sizes[j * 2];
        int size_w = img_sizes[j * 2 + 1];
        if (corner_x < tl_x) tl_x = corner_x;
        if (corner_y < tl_y) tl_y = corner_y;
        int calc_br_x = corner_x + size_w;
        int calc_br_y = corner_y + size_h;
        if (calc_br_x > br_x) br_x = calc_br_x;
        if (calc_br_y > br_y) br_y = calc_br_y;
    }

    dst_width = br_x - tl_x;
    dst_height = br_y - tl_y;

    return 0;
}

template <int SRC_T,
          int MASK_T,
          int ROWS_1,
          int COLS_1,
          int ROWS_2,
          int COLS_2,
          int ROWS_3,
          int COLS_3,
          int ROWS_4,
          int COLS_4,
          int ROWS_DST,
          int COLS_DST,
          int CHANNEL,
          int NPC = 1,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT>
void stitch_feed(xf::cv::Mat<SRC_T, ROWS_1, COLS_1, NPC, XFCVDEPTH_IN_1>& src1,
                 xf::cv::Mat<SRC_T, ROWS_2, COLS_2, NPC, XFCVDEPTH_IN_1>& src2,
                 xf::cv::Mat<SRC_T, ROWS_3, COLS_3, NPC, XFCVDEPTH_IN_1>& src3,
                 xf::cv::Mat<SRC_T, ROWS_4, COLS_4, NPC, XFCVDEPTH_IN_1>& src4,
                 xf::cv::Mat<MASK_T, ROWS_1, COLS_1, NPC, XFCVDEPTH_IN_1>& mask1,
                 xf::cv::Mat<MASK_T, ROWS_2, COLS_2, NPC, XFCVDEPTH_IN_1>& mask2,
                 xf::cv::Mat<MASK_T, ROWS_3, COLS_3, NPC, XFCVDEPTH_IN_1>& mask3,
                 xf::cv::Mat<MASK_T, ROWS_4, COLS_4, NPC, XFCVDEPTH_IN_1>& mask4,
                 int mask_corners[8],
                 xf::cv::Mat<SRC_T, ROWS_DST, COLS_DST, NPC, XFCVDEPTH_OUT_1>& dst,
                 int dst_height,
                 int dst_width) {
#pragma HLS INLINE OFF

    XF_TNAME(SRC_T, NPC) pxl_out1_actual;
    XF_TNAME(SRC_T, NPC) pxl_out2_actual;
    XF_TNAME(SRC_T, NPC) pxl_out3_actual;
    XF_TNAME(SRC_T, NPC) pxl_out4_actual;

    XF_TNAME(SRC_T, NPC) pxl_out;
    for (int i = 0; i < dst_height; i++) {
#pragma HLS LOOP_TRIPCOUNT min = ROWS_DST max = ROWS_DST
#pragma HLS LOOP_FLATTEN off

        for (int j = 0; j < dst_width; j++) {
#pragma HLS LOOP_TRIPCOUNT min = COLS_DST max = COLS_DST
#pragma HLS pipeline
            ap_uint<8> pxl_mask1 = 0;
            ap_uint<8> pxl_mask2 = 0;
            ap_uint<8> pxl_mask3 = 0;
            ap_uint<8> pxl_mask4 = 0;
            pxl_out1_actual = 0;
            pxl_out2_actual = 0;
            pxl_out3_actual = 0;
            pxl_out4_actual = 0;
            pxl_out = 0;
            // Read current pixel from each source/mask when in region (one read per mat per (i,j) for stream
            // correctness)
            if (j < dst_width && i < mask_corners[1]) {
                int mask1_i = i - mask_corners[1] + mask1.rows;
                if (mask1_i < mask1.rows && mask1_i >= 0 && j < mask1.cols) {
                    pxl_mask1 = mask1.read(mask1_i * mask1.cols + j);
                    pxl_out1_actual = src1.read(mask1_i * src1.cols + j);
                }
            }

            if ((j >= (mask_corners[2] + mask2.cols > dst_width ? (dst_width - mask2.cols) : mask_corners[2])) &&
                (i < dst_height)) {
                int mask1_i = i - (dst_height - mask2.rows) / 2;
                if (j >= dst_width - mask2.cols && mask1_i >= 0 && mask1_i < mask2.rows) {
                    pxl_mask2 = mask2.read(mask1_i * mask2.cols + (j - dst_width + mask2.cols));
                    pxl_out2_actual = src2.read(mask1_i * src2.cols + (j - dst_width + mask2.cols));
                }
            }

            if ((j < dst_width) &&
                (i >= (mask_corners[5] + mask3.rows > dst_height ? (dst_height - mask3.rows) : mask_corners[5]))) {
                if (i >= dst_height - mask3.rows && j < mask3.cols) {
                    pxl_mask3 = mask3.read((i - dst_height + mask3.rows) * mask3.cols + j);
                    pxl_out3_actual = src3.read((i - dst_height + mask3.rows) * src3.cols + j);
                }
            }

            if (j < mask4.cols && i < dst_height) {
                if (j < mask4.cols && i < mask4.rows) {
                    pxl_mask4 = mask4.read(i * mask4.cols + j);
                    pxl_out4_actual = src4.read(i * src4.cols + j);
                }
            }
            // Weights from current pixel only (stream-safe: no extra reads)
            ap_uint<16> w1 = pxl_mask1, w2 = pxl_mask2, w3 = pxl_mask3, w4 = pxl_mask4;
            ap_uint<32> w_sum = (ap_uint<32>)w1 + w2 + w3 + w4;
            float fw1 = (w_sum > 0) ? static_cast<float>(w1) / w_sum : 0.0f;
            float fw2 = (w_sum > 0) ? static_cast<float>(w2) / w_sum : 0.0f;
            float fw3 = (w_sum > 0) ? static_cast<float>(w3) / w_sum : 0.0f;
            float fw4 = (w_sum > 0) ? static_cast<float>(w4) / w_sum : 0.0f;

            // Weighted blend using normalized weights fw1..fw4 (feather blending)
            if (w_sum > 0) {
                if (CHANNEL == 3) {
                    ap_uint<8> b1 = pxl_out1_actual.range(7, 0);
                    ap_uint<8> g1 = pxl_out1_actual.range(15, 8);
                    ap_uint<8> r1 = pxl_out1_actual.range(23, 16);
                    ap_uint<8> b2 = pxl_out2_actual.range(7, 0);
                    ap_uint<8> g2 = pxl_out2_actual.range(15, 8);
                    ap_uint<8> r2 = pxl_out2_actual.range(23, 16);
                    ap_uint<8> b3 = pxl_out3_actual.range(7, 0);
                    ap_uint<8> g3 = pxl_out3_actual.range(15, 8);
                    ap_uint<8> r3 = pxl_out3_actual.range(23, 16);
                    ap_uint<8> b4 = pxl_out4_actual.range(7, 0);
                    ap_uint<8> g4 = pxl_out4_actual.range(15, 8);
                    ap_uint<8> r4 = pxl_out4_actual.range(23, 16);
                    int b = (int)(fw1 * b1 + fw2 * b2 + fw3 * b3 + fw4 * b4 + 0.5f);
                    int g = (int)(fw1 * g1 + fw2 * g2 + fw3 * g3 + fw4 * g4 + 0.5f);
                    int r = (int)(fw1 * r1 + fw2 * r2 + fw3 * r3 + fw4 * r4 + 0.5f);
                    if (b > 255) b = 255;
                    if (g > 255) g = 255;
                    if (r > 255) r = 255;
                    pxl_out = ((ap_uint<24>)r << 16) | ((ap_uint<24>)g << 8) | (ap_uint<24>)b;
                } else if (CHANNEL == 1) {
                    ap_uint<8> gray1 = pxl_out1_actual;
                    ap_uint<8> gray2 = pxl_out2_actual;
                    ap_uint<8> gray3 = pxl_out3_actual;
                    ap_uint<8> gray4 = pxl_out4_actual;
                    int gray = (int)(fw1 * gray1 + fw2 * gray2 + fw3 * gray3 + fw4 * gray4 + 0.5f);
                    if (gray > 255) gray = 255;
                    pxl_out = gray;
                }
            } else {
                pxl_out = 0;
            }
            dst.write(i * dst_width + j, pxl_out);
        }
    }
}
template <int SRC_T,
          int MASK_T,
          int ROWS_1,
          int COLS_1,
          int ROWS_2,
          int COLS_2,
          int ROWS_3,
          int COLS_3,
          int ROWS_4,
          int COLS_4,
          int ROWS_DST,
          int COLS_DST,
          int NPC = 1,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT>
void stitch(xf::cv::Mat<SRC_T, ROWS_1, COLS_1, NPC, XFCVDEPTH_IN_1>& src1,
            xf::cv::Mat<SRC_T, ROWS_2, COLS_2, NPC, XFCVDEPTH_IN_1>& src2,
            xf::cv::Mat<SRC_T, ROWS_3, COLS_3, NPC, XFCVDEPTH_IN_1>& src3,
            xf::cv::Mat<SRC_T, ROWS_4, COLS_4, NPC, XFCVDEPTH_IN_1>& src4,
            xf::cv::Mat<MASK_T, ROWS_1, COLS_1, NPC, XFCVDEPTH_IN_1>& mask1,
            xf::cv::Mat<MASK_T, ROWS_2, COLS_2, NPC, XFCVDEPTH_IN_1>& mask2,
            xf::cv::Mat<MASK_T, ROWS_3, COLS_3, NPC, XFCVDEPTH_IN_1>& mask3,
            xf::cv::Mat<MASK_T, ROWS_4, COLS_4, NPC, XFCVDEPTH_IN_1>& mask4,
            int mask_corners[8],
            xf::cv::Mat<SRC_T, ROWS_DST, COLS_DST, NPC, XFCVDEPTH_OUT_1>& dst) {
#ifndef __SYNTHESIS__
    assert(((src1.rows <= ROWS_1) && (src1.cols <= COLS_1)) && "ROWS and COLS should be greater than input image");
    assert(((src2.rows <= ROWS_2) && (src2.cols <= COLS_2)) && "ROWS and COLS should be greater than input image");
    assert(((src3.rows <= ROWS_3) && (src3.cols <= COLS_3)) && "ROWS and COLS should be greater than input image");
    assert(((src4.rows <= ROWS_4) && (src4.cols <= COLS_4)) && "ROWS and COLS should be greater than input image");
    assert(((mask1.rows <= ROWS_1) && (mask1.cols <= COLS_1)) && "ROWS and COLS should be greater than input image");
    assert(((mask2.rows <= ROWS_2) && (mask2.cols <= COLS_2)) && "ROWS and COLS should be greater than input image");
    assert(((mask3.rows <= ROWS_3) && (mask3.cols <= COLS_3)) && "ROWS and COLS should be greater than input image");
    assert(((mask4.rows <= ROWS_4) && (mask4.cols <= COLS_4)) && "ROWS and COLS should be greater than input image");
    assert(((dst.rows <= ROWS_DST) && (dst.cols <= COLS_DST)) && "ROWS and COLS should be greater than input image");
    assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC2) || (NPC == XF_NPPC4) || (NPC == XF_NPPC8)) &&
           "NPC must be XF_NPPC1, XF_NPPC2, XF_NPPC4,XF_NPPC8 ");
#endif

// clang-format off
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
    int img_sizes[8] = {src1.rows, src1.cols, src2.rows, src2.cols, src3.rows, src3.cols, src4.rows, src4.cols};
    int dst_height, dst_width;
    // clang-format on
    stitch_roi(img_sizes, mask_corners, dst_height, dst_width);

    stitch_feed<SRC_T, MASK_T, ROWS_1, COLS_1, ROWS_2, COLS_2, ROWS_3, COLS_3, ROWS_4, COLS_4, ROWS_DST, COLS_DST,
                XF_CHANNELS(SRC_T, NPC), NPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
        src1, src2, src3, src4, mask1, mask2, mask3, mask4, mask_corners, dst, dst_height, dst_width);
}

} // namespace cv
} // namespace xf

#endif /* _XF_STITCH_HPP_ */
