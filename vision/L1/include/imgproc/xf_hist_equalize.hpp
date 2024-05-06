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

#ifndef _XF_HIST_EQUALIZE_HPP_
#define _XF_HIST_EQUALIZE_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_histogram.hpp"

template <typename T>
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
inline ap_uint<14> xf_satcast_aec<ap_uint<14> >(int v) {
    v = (v > 16384 ? 16384 : v);
    v = (v < 0 ? 0 : v);
    return v;
};
template <>
inline ap_uint<16> xf_satcast_aec<ap_uint<16> >(int v) {
    v = (v > 65535 ? 65535 : v);
    v = (v < 0 ? 0 : v);
    return v;
};

/**
 *  xfEqualize : Computes the histogram and performs
 *               Histogram Equalization
 *  _src1	: Input image
 *  _dst_mat	: Output image
 */
namespace xf {
namespace cv {

template <int SRC_T,
          int ROWS,
          int COLS,
          int DEPTH,
          int NPC,
          int USE_URAM = 0,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int WORDWIDTH,
          int SRC_TC>
void xFEqualize(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src1,
                uint32_t hist_stream[0][256],
                xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& _dst_mat,
                uint16_t img_height,
                uint16_t img_width) {
    XF_SNAME(WORDWIDTH)
    in_buf, temp_buf;
    // Array to hold the values after cumulative distribution
    ap_uint<8> cum_hist[256];
// clang-format off
    #pragma HLS ARRAY_PARTITION variable=cum_hist complete dim=1
    // clang-format on
    // Temporary array to hold data
    ap_uint<8> tmp_cum_hist[(1 << XF_BITSHIFT(NPC))][256];
    // clang-format off
    if(USE_URAM){
        #pragma HLS bind_storage variable=tmp_cum_hist type=RAM_T2P impl=URAM
    }else{
        #pragma HLS ARRAY_PARTITION variable=tmp_cum_hist complete dim=1
    }
    // clang-format on
    // Array which holds histogram of the image

    /*	Normalization	*/
    uint32_t temp_val = (uint32_t)(img_height * (img_width << XF_BITSHIFT(NPC)));
    uint32_t init_val = (uint32_t)(temp_val - hist_stream[0][0]);
    uint32_t scale;
    if (init_val == 0) {
        scale = 0;
    } else {
        scale = (uint32_t)(((1 << 31)) / init_val);
    }

    ap_uint<40> scale1 = (ap_uint<40>)((ap_uint<40>)255 * (ap_uint<40>)scale);
    ap_uint32_t temp_sum = 0;

    cum_hist[0] = 0;
Normalize_Loop:
    for (ap_uint<9> i = 1; i < 256; i++) {
// clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=256 max=256
        #pragma HLS PIPELINE
        // clang-format on
        temp_sum = (uint32_t)temp_sum + (uint32_t)hist_stream[0][i];
        uint64_t sum = (uint64_t)((uint64_t)temp_sum * (uint64_t)scale1);
        sum = (uint64_t)(sum + 0x40000000);
        cum_hist[i] = sum >> 31;
    }

    for (ap_uint<9> i = 0; i < 256; i++) {
// clang-format off
        #pragma HLS PIPELINE
        // clang-format on
        for (ap_uint<5> j = 0; j < (1 << XF_BITSHIFT(NPC)); j++) {
// clang-format off
            #pragma HLS UNROLL
            // clang-format on
            ap_uint<8> tmpval = cum_hist[i];
            tmp_cum_hist[j][i] = tmpval;
        }
    }

NORMALISE_ROW_LOOP:
    for (ap_uint<13> row = 0; row < img_height; row++) {
// clang-format off
        #pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on
    NORMALISE_COL_LOOP:
        for (ap_uint<13> col = 0; col < img_width; col++) {
// clang-format off
            #pragma HLS LOOP_TRIPCOUNT min=SRC_TC max=SRC_TC
            #pragma HLS PIPELINE
            #pragma HLS LOOP_FLATTEN OFF
            // clang-format on
            in_buf = _src1.read(row * img_width + col);
        Normalise_Extract:
            for (ap_uint<9> i = 0, j = 0; i < (8 << XF_BITSHIFT(NPC)); j++, i += 8) {
// clang-format off
                #pragma HLS DEPENDENCE variable=tmp_cum_hist array intra false
                #pragma HLS unroll
                // clang-format on
                XF_PTNAME(DEPTH)
                val;
                val = in_buf.range(i + 7, i);
                temp_buf(i + 7, i) = tmp_cum_hist[j][val];
            }
            _dst_mat.write(row * img_width + col, temp_buf);
        }
    }
}
////16bit support///////////

template <int SRC_T,
          int ROWS,
          int COLS,
          int DEPTH,
          int NPC,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int WORDWIDTH,
          int SRC_TC,
          int AEC_HISTSIZE>
void xFEqualize_norm_sin(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& src,
                         uint32_t hist[AEC_HISTSIZE],
                         xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                         int p,
                         float inputMin,
                         float inputMax,
                         float outputMin,
                         float outputMax) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    short width = dst.cols >> XF_BITSHIFT(NPC);
    short height = dst.rows;
    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);

    ap_uint<STEP + 1> bins = AEC_HISTSIZE; // number of bins at each histogram level

    ap_uint<STEP + 1> nElements = AEC_HISTSIZE; // int(pow((float)bins, (float)depth));

    int total = dst.cols * dst.rows;

    ap_fixed<STEP + 8, STEP + 2> min_vals = inputMin - 0.5f;

    ap_fixed<STEP + 8, STEP + 2> max_vals = inputMax + 0.5f;

    ap_fixed<STEP + 8, STEP + 2> minValue = min_vals; //{-0.5, -0.5, -0.5};
    ap_fixed<STEP + 8, STEP + 2> maxValue = max_vals; //{12287.5, 16383.5, 12287.5};.

    ap_fixed<32, 16> p_float = (float)p;

    p_float = p_float >> 8;

    ap_fixed<STEP + 8, 4> s1 = (ap_fixed<STEP + 8, 4>)p_float;
    ap_fixed<STEP + 8, 4> s2 = (ap_fixed<STEP + 8, 4>)p_float;

    int rval = s1 * total / 100;
    int rval1 = (100 - s2) * total / 100;

    for (int j = 0; j < 1; ++j)
    // searching for s1 and s2
    {
        ap_uint<STEP + 2> p1 = 0;
        ap_uint<STEP + 2> p2 = bins - 1;
        ap_uint<32> n1 = 0;
        ap_uint<32> n2 = total;

        ap_fixed<STEP + 8, STEP + 2> interval = (max_vals - min_vals) / bins;

        for (int k = 0; k < 1; ++k)
        // searching for s1 and s2
        {
            int value = hist[p1];
            int value1 = hist[p2];

            while (n1 + hist[p1] < rval && p1 < AEC_HISTSIZE) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false
#pragma HLS DEPENDENCE variable = minValue intra false
                n1 += hist[p1++];
                minValue += interval;
            }
            // p1 *= bins;

            while (n2 - hist[p2] > rval1 && p2 != 0) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false
#pragma HLS DEPENDENCE variable = maxValue intra false
                n2 -= hist[p2--];
                maxValue -= interval;
            }
        }
    }

    ap_fixed<STEP + 8, STEP + 2> maxmin_diff;
    ap_fixed<STEP + 8, STEP + 2> newmax = inputMax;
    ap_fixed<STEP + 8, STEP + 2> newmin = 0.0f;
    maxmin_diff = maxValue - minValue;

    ap_fixed<STEP + 8, STEP + 2> newdiff = newmax - newmin;

    XF_TNAME(SRC_T, NPC) in_buf_n, in_buf_n1, out_buf_n;
    printf("valuesmin max :%f %f\n", (float)maxValue, (float)minValue);

    int pval = 0, read_index = 0, write_index = 0;
    ap_uint<13> row, col;

    ap_fixed<STEP + 18, 2> inv_val;

    if (maxmin_diff != 0) inv_val = ((ap_fixed<STEP + 18, 2>)1 / maxmin_diff);

NORMALISE_ROW_LOOP:
    for (row = 0; row < height; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on
    NORMALISE_COL_LOOP:
        for (col = 0; col < width; col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPC max=COLS/NPC
#pragma HLS pipeline II=1
#pragma HLS LOOP_FLATTEN OFF
            // clang-format on
            in_buf_n = src.read(read_index++);

            ap_fixed<STEP + 8, STEP + 2> value = 0;
            ap_fixed<STEP + STEP + 16, STEP + 8> divval = 0;
            ap_fixed<STEP + 16, STEP + 16> finalmul = 0;
            ap_int<32> dstval;

            for (int p = 0; p < XF_NPIXPERCYCLE(NPC); p++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                XF_CTUNAME(SRC_T, NPC)
                val = in_buf_n.range(p * STEP + STEP - 1, p * STEP);

                value = val - minValue;
                divval = value * inv_val;
                finalmul = divval * newdiff;
                dstval = (int)(finalmul + newmin);
                if (dstval.range(31, 31) == 1) {
                    dstval = 0;
                }
                out_buf_n.range(p * STEP + STEP - 1, p * STEP) = xf_satcast_aec<XF_CTUNAME(SRC_T, NPC)>(dstval);
            }

            dst.write(row * width + col, out_buf_n);
        }
    }
}

/////////
template <int SRC_T,
          int ROWS,
          int COLS,
          int DEPTH,
          int NPC,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int WORDWIDTH,
          int SRC_TC,
          int AEC_HISTSIZE>
void xFEqualize_norm_multi(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& src,
                           uint32_t hist[AEC_HISTSIZE],
                           xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& dst,
                           float p,
                           float inputMin,
                           float inputMax,
                           float outputMin,
                           float outputMax,
                           unsigned short org_height) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    short width = dst.cols >> XF_BITSHIFT(NPC);
    short height = dst.rows;
    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);

    ap_uint<STEP + 1> bins = AEC_HISTSIZE; // number of bins at each histogram level

    ap_uint<STEP + 1> nElements = AEC_HISTSIZE; // int(pow((float)bins, (float)depth));

    int total = dst.cols * org_height;

    ap_fixed<STEP + 8, STEP + 2> min_vals = inputMin - 0.5f;

    ap_fixed<STEP + 8, STEP + 2> max_vals = inputMax + 0.5f;

    ap_fixed<STEP + 8, STEP + 2> minValue = min_vals; //{-0.5, -0.5, -0.5};
    ap_fixed<STEP + 8, STEP + 2> maxValue = max_vals; //{12287.5, 16383.5, 12287.5};
    ap_fixed<STEP + 8, 4> s1 = p;
    ap_fixed<STEP + 8, 4> s2 = p;

    int rval = s1 * total / 100;
    int rval1 = (100 - s2) * total / 100;

    for (int j = 0; j < 1; ++j)
    // searching for s1 and s2
    {
        ap_uint<STEP + 2> p1 = 0;
        ap_uint<STEP + 2> p2 = bins - 1;
        ap_uint<32> n1 = 0;
        ap_uint<32> n2 = total;

        ap_fixed<STEP + 8, STEP + 2> interval = (max_vals - min_vals) / bins;

        for (int k = 0; k < 1; ++k)
        // searching for s1 and s2
        {
            int value = hist[p1];
            int value1 = hist[p2];

            while (n1 + hist[p1] < rval && p1 < AEC_HISTSIZE) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false
#pragma HLS DEPENDENCE variable = minValue intra false
                n1 += hist[p1++];
                minValue += interval;
            }
            // p1 *= bins;

            while (n2 - hist[p2] > rval1 && p2 != 0) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false
#pragma HLS DEPENDENCE variable = maxValue intra false
                n2 -= hist[p2--];
                maxValue -= interval;
            }
        }
    }

    ap_fixed<STEP + 8, STEP + 2> maxmin_diff;
    ap_fixed<STEP + 8, STEP + 2> newmax = inputMax;
    ap_fixed<STEP + 8, STEP + 2> newmin = 0.0f;
    maxmin_diff = maxValue - minValue;

    ap_fixed<STEP + 8, STEP + 2> newdiff = newmax - newmin;

    XF_TNAME(SRC_T, NPC) in_buf_n, in_buf_n1, out_buf_n;

    int pval = 0, read_index = 0, write_index = 0;
    ap_uint<13> row, col;

    ap_fixed<STEP + 16, 2> inv_val;

    if (maxmin_diff != 0) inv_val = ((ap_fixed<STEP + 16, 2>)1 / maxmin_diff);

NORMALISE_ROW_LOOP:
    for (row = 0; row < height; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on
    NORMALISE_COL_LOOP:
        for (col = 0; col < width; col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPC max=COLS/NPC
#pragma HLS pipeline II=1
#pragma HLS LOOP_FLATTEN OFF
            // clang-format on
            in_buf_n = src.read(read_index++);

            ap_fixed<STEP + 8, STEP + 2> value = 0;
            ap_fixed<STEP + STEP + 16, STEP + 8> divval = 0;
            ap_fixed<STEP + 16, STEP + 16> finalmul = 0;
            ap_int<32> dstval;

            for (int p = 0; p < XF_NPIXPERCYCLE(NPC); p++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                XF_CTUNAME(SRC_T, NPC)
                val = in_buf_n.range(p * STEP + STEP - 1, p * STEP);

                value = val - minValue;
                divval = value * inv_val;
                finalmul = divval * newdiff;
                dstval = (int)(finalmul + newmin);
                if (dstval.range(31, 31) == 1) {
                    dstval = 0;
                }
                out_buf_n.range(p * STEP + STEP - 1, p * STEP) = xf_satcast_aec<XF_CTUNAME(SRC_T, NPC)>(dstval);
            }

            dst.write(row * width + col, out_buf_n);
        }
    }
}
/****************************************************************
 * equalizeHist : Wrapper function which calls the main kernel
 ****************************************************************/

template <int SRC_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int USE_URAM = 0,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_IN_1 = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void equalizeHist(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& _src,
                  xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN_1>& _src1,
                  xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& _dst) {
// clang-format off
    #pragma HLS inline off
    // clang-format on

    uint16_t img_height = _src1.rows;
    uint16_t img_width = _src1.cols;
#ifndef __SYNTHESIS__
    assert(((img_height <= ROWS) && (img_width <= COLS)) && "ROWS and COLS should be greater than input image");

    assert((SRC_T == XF_8UC1) && "Type must be of XF_8UC1");
    assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) && " NPC must be XF_NPPC1, XF_NPPC8");
#endif
    uint32_t histogram[1][256];

    img_width = img_width >> XF_BITSHIFT(NPC);
    xFHistogramKernel<SRC_T, ROWS, COLS, XF_DEPTH(SRC_T, NPC), NPC, USE_URAM, XFCVDEPTH_IN, XF_WORDWIDTH(SRC_T, NPC),
                      ((COLS >> (XF_BITSHIFT(NPC))) >> 1), XF_CHANNELS(SRC_T, NPC)>(_src, histogram, img_height,
                                                                                    img_width);

    xFEqualize<SRC_T, ROWS, COLS, XF_DEPTH(SRC_T, NPC), NPC, USE_URAM, XFCVDEPTH_IN_1, XFCVDEPTH_OUT,
               XF_WORDWIDTH(SRC_T, NPC), (COLS >> XF_BITSHIFT(NPC))>(_src1, histogram, _dst, img_height, img_width);
}

} // namespace cv
} // namespace xf
#endif // _XF_HIST_EQUALIZE_H_
