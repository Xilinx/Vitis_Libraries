/*
 * Copyright 2020 Xilinx, Inc.
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

#ifndef _XF_CCM_HPP_
#define _XF_CCM_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

typedef unsigned short uint16_t;
typedef unsigned char uchar;

/**
 * @file xf_colorcorrectionmatrix.hpp
 * This file is part of Vitis Vision Library.
 */

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "hls_stream.h"

template <typename T>
T xf_satcast_ccm(int in_val){};

template <>
inline ap_uint<8> xf_satcast_ccm<ap_uint<8> >(int v) {
    v = (v > 255 ? 255 : v);
    v = (v < 0 ? 0 : v);
    return v;
};
template <>
inline ap_uint<10> xf_satcast_ccm<ap_uint<10> >(int v) {
    v = (v > 1023 ? 1023 : v);
    v = (v < 0 ? 0 : v);
    return v;
};
template <>
inline ap_uint<12> xf_satcast_ccm<ap_uint<12> >(int v) {
    v = (v > 4095 ? 4095 : v);
    v = (v < 0 ? 0 : v);
    return v;
};
template <>
inline ap_uint<16> xf_satcast_ccm<ap_uint<16> >(int v) {
    v = (v > 65535 ? 65535 : v);
    v = (v < 0 ? 0 : v);
    return v;
};

namespace xf {
namespace cv {

template <int SRC_T,
          int ROWS,
          int COLS,
          int DEPTH,
          int NPC,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT,
          int WORDWIDTH_SRC,
          int WORDWIDTH_DST,
          int COLS_TRIP>
void xfccmkernel(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src_mat,
                 xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& _dst_mat,
                 signed int ccm_matrix_1[3][3],
                 signed int offsetarray_1[3],
                 unsigned short height,
                 unsigned short width) {
    ap_fixed<32, 4> ccm_matrix[3][3];
    ap_fixed<32, 4> offsetarray[3];

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            ap_fixed<44, 24> temp1 = (float)ccm_matrix_1[i][j];
            ccm_matrix[i][j] = temp1 >> 20;
        }
        ap_fixed<44, 24> temp2 = (float)offsetarray_1[i];
        offsetarray[i] = temp2 >> 20;
    }

    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);
    ap_uint<13> i, j, k;
    XF_SNAME(WORDWIDTH_SRC) val_src;
    XF_SNAME(WORDWIDTH_DST) val_dst;

    int value_r = 0, value_g = 0, value_b = 0;

    XF_CTUNAME(SRC_T, NPC) r, g, b;
rowLoop:
    for (i = 0; i < height; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
    // clang-format on

    colLoop:
        for (j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
            // clang-format on

            val_src =
                (XF_SNAME(WORDWIDTH_SRC))(_src_mat.read(i * width + j)); // reading the source stream _src into val_src

            for (int p = 0; p < (XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC)); p = p + XF_CHANNELS(SRC_T, NPC)) {
// clang-format off
#pragma HLS unroll
                // clang-format on

                r = val_src.range(p * STEP + STEP - 1, p * STEP);
                g = val_src.range(p * STEP + (2 * STEP) - 1, p * STEP + STEP);
                b = val_src.range(p * STEP + (3 * STEP) - 1, p * STEP + 2 * STEP);

                ap_fixed<32, 24> value1 = (r * ccm_matrix[0][0]);
                ap_fixed<32, 24> value2 = (g * ccm_matrix[0][1]);
                ap_fixed<32, 24> value3 = (b * ccm_matrix[0][2]);

                ap_fixed<32, 24> value4 = (r * ccm_matrix[1][0]);
                ap_fixed<32, 24> value5 = (g * ccm_matrix[1][1]);
                ap_fixed<32, 24> value6 = (b * ccm_matrix[1][2]);

                ap_fixed<32, 24> value7 = (r * ccm_matrix[2][0]);
                ap_fixed<32, 24> value8 = (g * ccm_matrix[2][1]);
                ap_fixed<32, 24> value9 = (b * ccm_matrix[2][2]);

                value_r = (int)(value1 + value2 + value3 + offsetarray[0]);
                value_g = (int)(value4 + value5 + value6 + offsetarray[1]);
                value_b = (int)(value7 + value8 + value9 + offsetarray[2]);

                val_dst.range(p * STEP + STEP - 1, p * STEP) = xf_satcast_ccm<XF_CTUNAME(SRC_T, NPC)>(value_r);
                val_dst.range(p * STEP + (2 * STEP) - 1, p * STEP + STEP) =
                    xf_satcast_ccm<XF_CTUNAME(SRC_T, NPC)>(value_g);
                val_dst.range(p * STEP + (3 * STEP) - 1, p * STEP + 2 * STEP) =
                    xf_satcast_ccm<XF_CTUNAME(SRC_T, NPC)>(value_b);
            }

            _dst_mat.write(i * width + j, (val_dst)); // writing the val_dst into output stream _dst
        }
    }
}
/**
 * @tparam CCM_TYPE colorcorrection type
 * @tparam SRC_T input type
 * @tparam DST_T ouput type
 * @tparam ROWS rows of the input and output image
 * @tparam COLS cols of the input and output image
 * @tparam NPC number of pixels processed per cycle
 * @param _src_mat input image
 * @param _dst_mat output image
 */
template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT>
void colorcorrectionmatrix(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src_mat,
                           xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& _dst_mat,
                           signed int ccm_matrix[3][3],
                           signed int offsetarray[3]) {
    unsigned short width = _src_mat.cols >> XF_BITSHIFT(NPC);
    unsigned short height = _src_mat.rows;
    assert(((height <= ROWS) && (width <= COLS)) && "ROWS and COLS should be greater than input image");

// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    xfccmkernel<SRC_T, ROWS, COLS, XF_DEPTH(SRC_T, NPC), NPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1, XF_WORDWIDTH(SRC_T, NPC),
                XF_WORDWIDTH(SRC_T, NPC), (COLS >> XF_BITSHIFT(NPC))>(_src_mat, _dst_mat, ccm_matrix, offsetarray,
                                                                      height, width);
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int STREAMS = 2,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT_1 = _XFCVDEPTH_DEFAULT>
void colorcorrectionmatrix_multi(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src_mat,
                                 xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT_1>& _dst_mat,
                                 signed int ccm_matrix[STREAMS][3][3],
                                 signed int offsetarray[STREAMS][3],
                                 int strm_id) {
    colorcorrectionmatrix<SRC_T, DST_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1, XFCVDEPTH_OUT_1>(
        _src_mat, _dst_mat, ccm_matrix[strm_id], offsetarray[strm_id]);
}
} // namespace cv
} // namespace xf

#endif
