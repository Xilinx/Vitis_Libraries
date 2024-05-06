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
#ifndef _XF_AEC_HPP_
#define _XF_AEC_HPP_

#include "common/xf_common.hpp"
#include "hls_math.h"
#include "hls_stream.h"
#include "xf_bgr2hsv.hpp"
#include "xf_channel_combine.hpp"
#include "xf_channel_extract.hpp"
#include "xf_cvt_color.hpp"
#include "xf_cvt_color.hpp"
#include "xf_duplicateimage.hpp"
#include "xf_hist_equalize.hpp"
#include "xf_histogram.hpp"

/*template <typename T>
T xf_satcast_aec(int in_val){};

template <>
inline ap_uint<8> xf_satcast_aec<ap_uint<8> >(int v) {
    v = (v > 255 ? 255 : v);
    v = (v < 0 ? 0 : v);
    return v;
};
template <>
inline ap_uint<10> xf_satcast_aec<ap_uint<10> >(int v) {
    v = (v > 1023 ? 1023 : v);
    v = (v < 0 ? 0 : v);
    return v;
};
template <>
inline ap_uint<12> xf_satcast_aec<ap_uint<12> >(int v) {
    v = (v > 4095 ? 4095 : v);
    v = (v < 0 ? 0 : v);
    return v;
};
template <>
inline ap_uint<16> xf_satcast_aec<ap_uint<16> >(int v) {
    v = (v > 65535 ? 65535 : v);
    v = (v < 0 ? 0 : v);
    return v;
};*/

namespace xf {
namespace cv {

template <int SRC_T,
          int DST_T,
          int SIN_CHANNEL_TYPE,
          int ROWS,
          int COLS,
          int NPC = 1,
          int USE_URAM = 0,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void autoexposurecorrection_mono(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
                                 xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                                 uint32_t hist_array1[1][256],
                                 uint32_t hist_array2[1][256]) {
#pragma HLS INLINE OFF

    int rows = src.rows;
    int cols = src.cols;

    uint16_t cols_shifted = cols >> (XF_BITSHIFT(NPC));
    uint16_t rows_shifted = rows;

    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> vimage1(rows, cols);
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> vimage2(rows, cols);

    xf::cv::duplicateMat(src, vimage1, vimage2);

    xFHistogramKernel<SIN_CHANNEL_TYPE, ROWS, COLS, XF_DEPTH(SIN_CHANNEL_TYPE, NPC), NPC, USE_URAM, XFCVDEPTH_IN,
                      XF_WORDWIDTH(SIN_CHANNEL_TYPE, NPC), ((COLS >> (XF_BITSHIFT(NPC))) >> 1),
                      XF_CHANNELS(SIN_CHANNEL_TYPE, NPC)>(vimage1, hist_array1, rows_shifted, cols_shifted);

    xFEqualize<SIN_CHANNEL_TYPE, ROWS, COLS, XF_DEPTH(SIN_CHANNEL_TYPE, NPC), NPC, USE_URAM, XFCVDEPTH_IN,
               XFCVDEPTH_OUT, XF_WORDWIDTH(SIN_CHANNEL_TYPE, NPC), (COLS >> XF_BITSHIFT(NPC))>(
        vimage2, hist_array2, dst, rows_shifted, cols_shifted);
}

template <int SRC_T,
          int DST_T,
          int SIN_CHANNEL_TYPE,
          int ROWS,
          int COLS,
          int NPC = 1,
          int USE_URAM = 0,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void autoexposurecorrection(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
                            xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                            uint32_t hist_array1[1][256],
                            uint32_t hist_array2[1][256]) {
#pragma HLS INLINE OFF

    int rows = src.rows;
    int cols = src.cols;

    uint16_t cols_shifted = cols >> (XF_BITSHIFT(NPC));
    uint16_t rows_shifted = rows;

    xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN> bgr2hsv(rows, cols);

    xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN> hsvimg1(rows, cols);
    xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN> hsvimg2(rows, cols);
    xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN> hsvimg3(rows, cols);

    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> himage(rows, cols);
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> simage(rows, cols);
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> vimage(rows, cols);
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> vimage1(rows, cols);
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> vimage2(rows, cols);

    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> vimage_eq(rows, cols);
    xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN> imgHelper6(rows, cols);

    assert(((rows <= ROWS) && (cols <= COLS)) && "ROWS and COLS should be greater than input image");

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    // Convert RGBA to HSV:
    xf::cv::bgr2hsv<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_IN>(src, bgr2hsv);

    xf::cv::duplicateimages<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_IN, XFCVDEPTH_IN, XFCVDEPTH_IN>(
        bgr2hsv, hsvimg1, hsvimg2, hsvimg3);

    xf::cv::extractChannel<SRC_T, SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_IN>(hsvimg1, himage, 0);

    xf::cv::extractChannel<SRC_T, SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_IN>(hsvimg2, simage, 1);

    xf::cv::extractChannel<SRC_T, SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_IN>(hsvimg3, vimage, 2);

    xf::cv::duplicateMat(vimage, vimage1, vimage2);

    // xf::cv::equalizeHist<SIN_CHANNEL_TYPE, ROWS, COLS, NPC>(vimage1, vimage2,
    // vimage_eq);
    xFHistogramKernel<SIN_CHANNEL_TYPE, ROWS, COLS, XF_DEPTH(SIN_CHANNEL_TYPE, NPC), NPC, USE_URAM, XFCVDEPTH_IN,
                      XF_WORDWIDTH(SIN_CHANNEL_TYPE, NPC), ((COLS >> (XF_BITSHIFT(NPC))) >> 1),
                      XF_CHANNELS(SIN_CHANNEL_TYPE, NPC)>(vimage1, hist_array1, rows_shifted, cols_shifted);

    xFEqualize<SIN_CHANNEL_TYPE, ROWS, COLS, XF_DEPTH(SIN_CHANNEL_TYPE, NPC), NPC, USE_URAM, XFCVDEPTH_IN, XFCVDEPTH_IN,
               XF_WORDWIDTH(SIN_CHANNEL_TYPE, NPC), (COLS >> XF_BITSHIFT(NPC))>(vimage2, hist_array2, vimage_eq,
                                                                                rows_shifted, cols_shifted);

    xf::cv::merge<SIN_CHANNEL_TYPE, SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_IN, XFCVDEPTH_IN, XFCVDEPTH_IN>(
        vimage_eq, simage, himage, imgHelper6);

    xf::cv::hsv2bgr<SRC_T, SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_OUT>(imgHelper6, dst);
}

/////////16bit single channel/////////////
template <int SRC_T,
          int DST_T,
          int SIN_CHANNEL_TYPE,
          int ROWS,
          int COLS,
          int NPC = 1,
          int USE_URAM = 0,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int AEC_HISTSIZE>
void autoexposurecorrection_sin(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src1,
                                xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                                uint32_t hist_array1[AEC_HISTSIZE],
                                uint32_t hist_array2[AEC_HISTSIZE],
                                int p,
                                float inputMin,
                                float inputMax,
                                float outputMin,
                                float outputMax) {
#pragma HLS INLINE OFF

    int rows = src1.rows;
    int cols = src1.cols;

    uint16_t cols_shifted = cols >> (XF_BITSHIFT(NPC));
    uint16_t rows_shifted = rows;
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> src2(rows, cols);
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> src3(rows, cols);

    assert(((rows <= ROWS) && (cols <= COLS)) && "ROWS and COLS should be greater than input image");

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    xFHistogramKernel_sin<SIN_CHANNEL_TYPE, ROWS, COLS, XF_DEPTH(SIN_CHANNEL_TYPE, NPC), NPC, USE_URAM, XFCVDEPTH_IN,
                          XFCVDEPTH_IN, XF_WORDWIDTH(SIN_CHANNEL_TYPE, NPC), ((COLS >> (XF_BITSHIFT(NPC))) >> 1),
                          XF_CHANNELS(SIN_CHANNEL_TYPE, NPC), AEC_HISTSIZE>(src1, src2, hist_array1, p, inputMin,
                                                                            inputMax, outputMin, outputMax);

    xFEqualize_norm_sin<SIN_CHANNEL_TYPE, ROWS, COLS, XF_DEPTH(SIN_CHANNEL_TYPE, NPC), NPC, XFCVDEPTH_IN, XFCVDEPTH_IN,
                        XF_WORDWIDTH(SIN_CHANNEL_TYPE, NPC), (COLS >> XF_BITSHIFT(NPC)), AEC_HISTSIZE>(
        src2, hist_array2, dst, p, inputMin, inputMax, outputMin, outputMax);
}

template <int SRC_T,
          int DST_T,
          int SIN_CHANNEL_TYPE,
          int ROWS,
          int COLS,
          int NPC = 1,
          int USE_URAM = 0,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int AEC_HISTSIZE>
void autoexposurecorrection_multi(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src1,
                                  xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                                  uint32_t hist_array1[AEC_HISTSIZE],
                                  uint32_t hist_array2[AEC_HISTSIZE],
                                  float p,
                                  float inputMin,
                                  float inputMax,
                                  float outputMin,
                                  float outputMax,
                                  int slc_id,
                                  unsigned short org_height) {
#pragma HLS INLINE OFF

    int rows = src1.rows;
    int cols = src1.cols;

    uint16_t cols_shifted = cols >> (XF_BITSHIFT(NPC));
    uint16_t rows_shifted = rows;
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> src2(rows, cols);
    xf::cv::Mat<SIN_CHANNEL_TYPE, ROWS, COLS, NPC, XFCVDEPTH_IN> src3(rows, cols);

    assert(((rows <= ROWS) && (cols <= COLS)) && "ROWS and COLS should be greater than input image");

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    xFHistogramKernel_multi<SIN_CHANNEL_TYPE, ROWS, COLS, XF_DEPTH(SIN_CHANNEL_TYPE, NPC), NPC, USE_URAM, XFCVDEPTH_IN,
                            XFCVDEPTH_IN, XF_WORDWIDTH(SIN_CHANNEL_TYPE, NPC), ((COLS >> (XF_BITSHIFT(NPC))) >> 1),
                            XF_CHANNELS(SIN_CHANNEL_TYPE, NPC), AEC_HISTSIZE>(src1, src2, hist_array1, p, inputMin,
                                                                              inputMax, outputMin, outputMax, slc_id);

    xFEqualize_norm_multi<SIN_CHANNEL_TYPE, ROWS, COLS, XF_DEPTH(SIN_CHANNEL_TYPE, NPC), NPC, XFCVDEPTH_IN,
                          XFCVDEPTH_IN, XF_WORDWIDTH(SIN_CHANNEL_TYPE, NPC), (COLS >> XF_BITSHIFT(NPC)), AEC_HISTSIZE>(
        src2, hist_array2, dst, p, inputMin, inputMax, outputMin, outputMax, org_height);
}
template <int SRC_T,
          int DST_T,
          int SIN_CHANNEL_TYPE,
          int ROWS,
          int COLS,
          int NPC = 1,
          int USE_URAM = 0,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int AEC_HISTSIZE>
void aec_wrap(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src1,
              xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
              uint32_t hist_array1[AEC_HISTSIZE],
              uint32_t hist_array2[AEC_HISTSIZE],
              float thresh,
              float inputMin,
              float inputMax,
              float outputMin,
              float outputMax,
              bool& flag,
              bool& eof,
              int slc_id,
              unsigned short org_height) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    if (!flag) {
        xf::cv::autoexposurecorrection_multi<SRC_T, DST_T, SIN_CHANNEL_TYPE, ROWS, COLS, NPC, USE_URAM, XFCVDEPTH_IN,
                                             XFCVDEPTH_OUT, AEC_HISTSIZE>(
            src1, dst, hist_array1, hist_array2, thresh, inputMin, inputMax, outputMin, outputMax, slc_id, org_height);

        if (eof) flag = 1;

    } else {
        xf::cv::autoexposurecorrection_multi<SRC_T, DST_T, SIN_CHANNEL_TYPE, ROWS, COLS, NPC, USE_URAM, XFCVDEPTH_IN,
                                             XFCVDEPTH_OUT, AEC_HISTSIZE>(
            src1, dst, hist_array2, hist_array1, thresh, inputMin, inputMax, outputMin, outputMax, slc_id, org_height);

        if (eof) flag = 0;
    }

    return;
}
template <int SRC_T,
          int DST_T,
          int SIN_CHANNEL_TYPE,
          int ROWS,
          int COLS,
          int NPC = 1,
          int USE_URAM = 0,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int AEC_HISTSIZE,
          int STREAMS = 2>
void aec_multi(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src1,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
               uint32_t hist_array0[STREAMS][AEC_HISTSIZE],
               uint32_t hist_array1[STREAMS][AEC_HISTSIZE],
               unsigned short pawb[STREAMS],
               float inputMin,
               float inputMax,
               float outputMin,
               float outputMax,
               bool flag[STREAMS],
               bool eof[STREAMS],
               int strm_id,
               int slc_id,
               unsigned short full_height) {
// clang-format off
#pragma HLS ARRAY_PARTITION variable=hist_array0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=hist_array1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=pawb complete dim=1
#pragma HLS ARRAY_PARTITION variable=flag complete dim=1
#pragma HLS ARRAY_PARTITION variable=eof complete dim=1

    // clang-format on

    float thresh = (float)pawb[strm_id] / 256;

    xf::cv::aec_wrap<SRC_T, DST_T, SIN_CHANNEL_TYPE, ROWS, COLS, NPC, USE_URAM, XFCVDEPTH_IN, XFCVDEPTH_OUT,
                     AEC_HISTSIZE>(src1, dst, hist_array0[strm_id], hist_array1[strm_id], thresh, inputMin, inputMax,
                                   outputMin, outputMax, flag[strm_id], eof[strm_id], slc_id, full_height);
}
}
}

#endif
