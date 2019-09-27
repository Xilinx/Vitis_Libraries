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

#ifndef _XF_AWB_HPP_
#define _XF_AWB_HPP_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "imgproc/xf_duplicateimage.hpp"

#ifndef XF_IN_STEP
#define XF_IN_STEP 8
#endif
#ifndef XF_OUT_STEP
#define XF_OUT_STEP 8
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

namespace xf {
namespace cv {

/*
 * Simple auto white balancing algorithm requires compute histogram and normalize function based on maximum and minimum
 * of the histogram.
 */

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int PLANES,
          int DEPTH_SRC,
          int DEPTH_DST,
          int WORDWIDTH_SRC,
          int WORDWIDTH_DST,
          int TC>
int balanceWhiteKernel_simple(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src1,
                              xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src2,
                              xf::cv::Mat<DST_T, ROWS, COLS, NPC>& dst,
                              float thresh,
                              uint16_t height,
                              uint16_t width,
                              float inputMin,
                              float inputMax,
                              float outputMin,
                              float outputMax) {
    // float inputMin = 0.0f;
    // float inputMax = 255.0f;
    // float outputMin = 0.0f;
    // float outputMax = 255.0f;
    float p = 2.0f;

    XF_TNAME(SRC_T, NPC) in_pix, in_pix1, out_pix;
    ap_uint<8> r, g, b, b1 = 0, g1 = 0, r1 = 0;

    //******************** Simple white balance ********************

    unsigned char depth = 2; // depth of histogram tree

    unsigned char bins = 16; // number of bins at each histogram level

    short int nElements = 256; // int(pow((float)bins, (float)depth));

    int hist[3][nElements];
// clang-format off
#pragma HLS RESOURCE variable=hist core=RAM_T2P_BRAM
#pragma HLS ARRAY_PARTITION variable=hist complete dim=1
    // clang-format on

    int val[3];

// histogram initialization

INITIALIZE_HIST:
    for (int k = 0; k < 256; k++) {
// clang-format off
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
    // clang-format on
    INITIALIZE:
        for (int hi = 0; hi < 3; hi++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            hist[hi][k] = 0;
        }
    }

    // Temporary array used while computing histogram
    uint32_t tmp_hist[XF_NPIXPERCYCLE(NPC) * PLANES][256] = {0};
// clang-format off
#pragma HLS RESOURCE variable=tmp_hist core=RAM_T2P_BRAM
    // clang-format on
    uint32_t tmp_hist1[XF_NPIXPERCYCLE(NPC) * PLANES][256] = {0};
// clang-format off
#pragma HLS RESOURCE variable=tmp_hist1 core=RAM_T2P_BRAM

#pragma HLS ARRAY_PARTITION variable=tmp_hist complete dim=1
#pragma HLS ARRAY_PARTITION variable=tmp_hist1 complete dim=1
    // clang-format on
    XF_TNAME(SRC_T, NPC) in_buf, in_buf1, temp_buf;

    bool flag = 0;

HIST_INITIALIZE_LOOP:
    for (ap_uint<10> i = 0; i < 256; i++) //
    {
// clang-format off
#pragma HLS PIPELINE
        // clang-format on
        for (ap_uint<5> j = 0; j < XF_NPIXPERCYCLE(NPC) * PLANES; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
            // clang-format on
            tmp_hist[j][i] = 0;
            tmp_hist1[j][i] = 0;
        }
    }

ROW_LOOP:
    for (ap_uint<13> row = 0; row != (height); row++) // histogram filling
    {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
    // clang-format on
    COL_LOOP:
        for (ap_uint<13> col = 0; col < (width); col = col + 2) // histogram filling
        {
// clang-format off
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC/2
#pragma HLS DEPENDENCE variable=hist array intra false
#pragma HLS DEPENDENCE variable=hist array inter false
            // clang-format on

            in_pix = src1.read(row * (width) + col);
            in_pix1 = src1.read((row * (width) + col) + 1);

        PLANES_LOOP:
            for (ap_uint<9> j = 0; j < XF_NPIXPERCYCLE(NPC) * PLANES; j++) {
// clang-format off
#pragma HLS DEPENDENCE variable=tmp_hist array intra false
#pragma HLS DEPENDENCE variable=tmp_hist1 array intra false
#pragma HLS UNROLL
                // clang-format on

                ap_uint<8> val = 0, val1 = 0;
                val = in_pix.range(j * 8 + 7, j * 8);
                val1 = in_pix1.range(j * 8 + 7, j * 8);

                short int pos = 0, pos1 = 0;
                short int currentBin = 0, currentBin1 = 0;
                ap_fixed<16, 12> min_vals = inputMin - 0.5f;
                ap_fixed<16, 12> max_vals = inputMax + 0.5f;

                ap_fixed<16, 12> minValue = min_vals, minValue1 = min_vals;
                ap_fixed<16, 12> maxValue = max_vals, maxValue1 = max_vals;

                ap_fixed<16, 12> interval = ap_fixed<24, 12>(maxValue - minValue) / bins;

                for (ap_uint<2> k = 0; k < 2; ++k) {
// clang-format off

#pragma HLS UNROLL
                    // clang-format on

                    currentBin = int((val - minValue + (ap_fixed<24, 12>)(1e-4f)) / interval);
                    currentBin1 = int((val1 - minValue1 + (ap_fixed<24, 12>)(1e-4f)) / interval);

                    ++tmp_hist[j][pos + currentBin];
                    ++tmp_hist1[j][pos1 + currentBin1];

                    pos = (pos + currentBin) << 4;
                    pos1 = (pos1 + currentBin1) << 4;

                    minValue = minValue + currentBin * interval;
                    minValue1 = minValue1 + currentBin1 * interval;
                    maxValue = minValue + interval;
                    maxValue1 = minValue1 + interval;

                    interval /= bins;
                }
            }
        }
    }

    uint32_t cnt, p1;
    uint32_t plane[PLANES];
COPY_LOOP:
    for (ap_uint<10> i = 0; i < 256; i++) {
// clang-format off
#pragma HLS pipeline
        // clang-format on
        cnt = 0;
        p1 = 0;
        for (ap_uint<5> j = 0, k = 0; j < XF_NPIXPERCYCLE(NPC) * PLANES; j++, k++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            uint32_t value = tmp_hist[j][i] + tmp_hist1[j][i];
            cnt = cnt + value;
            if (PLANES != 1) {
                plane[p1] = cnt;
                p1++;
                cnt = 0;
                value = 0;
            }
        }
        if (PLANES == 1) {
            hist[0][i] = cnt;
        } else {
            hist[0][i] = plane[0];
            hist[1][i] = plane[1];
            hist[2][i] = plane[2];
        }
    }

    ap_uint<24> total = width * height;
    ap_fixed<16, 12> min_vals = inputMin - 0.5f;
    ap_fixed<16, 12> max_vals = inputMax + 0.5f;
    ap_fixed<16, 12> minValue[3] = {min_vals, min_vals, min_vals};
    ap_fixed<16, 12> maxValue[3] = {max_vals, max_vals, max_vals};

    for (int j = 0; j < 3; ++j)
    // searching for s1 and s2
    {
        ap_uint<8> p1 = 0;
        ap_uint<8> p2 = bins - 1;
        ap_uint<24> n1 = 0;
        ap_uint<24> n2 = total;

        ap_fixed<16, 12> s1 = 2.0f;
        ap_fixed<16, 12> s2 = 2.0f;

        ap_fixed<16, 12> interval = (max_vals - min_vals) / bins;

        for (ap_uint<2> k = 0; k < 2; ++k)
        // searching for s1 and s2
        {
            int value = hist[j][p1];
            int value1 = hist[j][p2];

            int rval = (s1 * total) / 100;

            int rval1 = (100 - s2) * total / 100;

            while (n1 + hist[j][p1] < rval) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=255 max=255
#pragma HLS DEPENDENCE variable=hist array intra false
                // clang-format on
                n1 += hist[j][p1++];
                minValue[j] += interval;
            }
            p1 *= bins;

            while (n2 - hist[j][p2] > rval1) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=255 max=255
#pragma HLS DEPENDENCE variable=hist array intra false
                // clang-format on
                n2 -= hist[j][p2--];
                maxValue[j] -= interval;
            }
            p2 = (p2 + 1) * bins - 1;

            interval /= bins;
        }
    }

    ap_fixed<16, 12> maxmin_diff[3];

    ap_fixed<16, 12> newmax = 255.0f;
    ap_fixed<16, 12> newmin = 0.0f;
    maxmin_diff[0] = maxValue[0] - minValue[0];
    maxmin_diff[1] = maxValue[1] - minValue[1];
    maxmin_diff[2] = maxValue[2] - minValue[2];
    ap_fixed<24, 12> newdiff = newmax - newmin;

    XF_TNAME(SRC_T, NPC) in_buf_n, out_buf_n;

    ap_fixed<24, 6> inv_val[3];
    inv_val[0] = ((ap_fixed<16, 12>)1 / maxmin_diff[0]);
    inv_val[1] = ((ap_fixed<16, 12>)1 / maxmin_diff[1]);
    inv_val[2] = ((ap_fixed<16, 12>)1 / maxmin_diff[2]);

    int pval = 0, read_index = 0, write_index = 0;
    ap_uint<13> row, col;

Row_Loop1:
    for (row = 0; row < height; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on
    Col_Loop1:
        for (col = 0; col < width; col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPC max=COLS/NPC
#pragma HLS pipeline II=1
#pragma HLS LOOP_FLATTEN OFF
            // clang-format on
            in_buf_n = src2.read(read_index++);

            ap_fixed<24, 12> value = 0;    //[3]={0,0,0};
            ap_fixed<24, 12> divval = 0;   //[3]={0,0,0};
            ap_fixed<24, 12> finalmul = 0; //[3]={0,0,0};
            ap_int<32> dstval = 0;         //[3]={0,0,0};

            for (int p = 0, bit = 0; p < XF_NPIXPERCYCLE(NPC) * PLANES; p++, bit = p % 3) {
// clang-format off
#pragma HLS unroll
                // clang-format on
                ap_uint<8> val = in_buf_n.range(p * 8 + 7, p * 8);
                // dstval[p%3] = (newmax - newmin) * (val - minValue[p%3]) / (maxValue[p%3] - minValue[p%3]) + newmin;

                value = val - minValue[bit];
                divval = value / maxmin_diff[p % 3];
                finalmul = divval * newdiff;
                dstval = (int)(finalmul + newmin);

                if (dstval.range(31, 31) == 1) {
                    dstval = 0;
                }

                if (dstval > 255) {
                    out_buf_n.range((p * 8) + 7, p * 8) = 255; //(unsigned char)dstval[j];
                } else {
                    out_buf_n.range((p * 8) + 7, p * 8) = (unsigned char)dstval;
                }
            }

            dst.write(row * width + col, out_buf_n);
        }
    }
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int PLANES,
          int DEPTH_SRC,
          int DEPTH_DST,
          int WORDWIDTH_SRC,
          int WORDWIDTH_DST,
          int TC>
int balanceWhiteKernel(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src1,
                       xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src2,
                       xf::cv::Mat<DST_T, ROWS, COLS, NPC>& dst,
                       float thresh,
                       uint16_t height,
                       uint16_t width) {
    ap_uint<13> i = 0, j = 0;

    XF_TNAME(SRC_T, NPC) in_pix, out_pix;
    ap_uint<8> r, g, b, b1 = 0, g1 = 0, r1 = 0;

    XF_SNAME(WORDWIDTH_DST) pxl_pack_out;
    XF_SNAME(WORDWIDTH_SRC) pxl_pack1, pxl_pack2;

    int thresh255 = int(thresh * 255);

    int minRGB, maxRGB;

    ap_ufixed<32, 32> tmpsum_vals[(1 << XF_BITSHIFT(NPC)) * PLANES];

    ap_ufixed<32, 32> sum[PLANES];
// clang-format off
#pragma HLS ARRAY_PARTITION variable=tmpsum_vals complete dim=0
#pragma HLS ARRAY_PARTITION variable=sum complete dim=0
    // clang-format on

    for (j = 0; j < ((1 << XF_BITSHIFT(NPC)) * PLANES); j++) {
// clang-format off
#pragma HLS UNROLL
        // clang-format on
        tmpsum_vals[j] = 0;
    }
    for (j = 0; j < PLANES; j++) {
// clang-format off
#pragma HLS UNROLL
        // clang-format on
        sum[j] = 0;
    }

    int p = 0, read_index = 0;

Row_Loop:
    for (i = 0; i < height; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on
    Col_Loop:
        for (j = 0; j < (width); j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPC max=COLS/NPC
#pragma HLS pipeline II=1
#pragma HLS LOOP_FLATTEN OFF
            // clang-format on
            XF_TNAME(SRC_T, NPC) in_buf;
            in_buf = src1.read(i * width + j);

        PLANES_LOOP:
            for (int p = 0; p < XF_NPIXPERCYCLE(NPC) * PLANES; p = p + PLANES) {
// clang-format off
#pragma HLS unroll
                // clang-format on
                ap_uint<8> val1 = in_buf.range(p * 8 + 7, p * 8);
                ap_uint<8> val2 = in_buf.range(p * 8 + 15, p * 8 + 8);
                ap_uint<8> val3 = in_buf.range(p * 8 + 23, p * 8 + 16);

                minRGB = MIN(val1, MIN(val2, val3));
                maxRGB = MAX(val1, MAX(val2, val3));

                if ((maxRGB - minRGB) * 255 > thresh255 * maxRGB) continue;

                tmpsum_vals[p] = tmpsum_vals[p] + val1;
                tmpsum_vals[(p) + 1] = tmpsum_vals[(p) + 1] + val2;
                tmpsum_vals[(p) + 2] = tmpsum_vals[(p) + 2] + val3;
            }
        }
    }

    for (int c = 0; c < PLANES; c++) {
        for (j = 0; j < (1 << XF_BITSHIFT(NPC)); j++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            sum[c] = (sum[c] + tmpsum_vals[j * PLANES + c]);
        }
    }

    ap_ufixed<32, 32> max_sum_fixed = MAX(sum[0], MAX(sum[1], sum[2]));

    ap_ufixed<32, 2> bval = (float)0.1;
    ap_ufixed<32, 32> zero = 0;

    ap_ufixed<40, 32> dinB1;
    ap_ufixed<40, 32> dinG1;
    ap_ufixed<40, 32> dinR1;

    if (sum[0] < bval) {
        dinB1 = 0;
    } else {
        dinB1 = (ap_ufixed<40, 32>)((ap_ufixed<40, 32>)max_sum_fixed / sum[0]);
    }

    if (sum[1] < bval) {
        dinG1 = 0;
    } else {
        dinG1 = (ap_ufixed<40, 32>)((ap_ufixed<40, 32>)max_sum_fixed / sum[1]);
    }
    if (sum[2] < bval) {
        dinR1 = 0;
    } else {
        dinR1 = (ap_ufixed<40, 32>)((ap_ufixed<40, 32>)max_sum_fixed / sum[2]);
    }

    ap_ufixed<40, 32> gain_max1 = MAX(dinB1, MAX(dinG1, dinR1));

    if (gain_max1 > 0) {
        dinB1 /= gain_max1;
        dinG1 /= gain_max1;
        dinR1 /= gain_max1;
    }

    float a1 = dinB1;
    float a2 = dinG1;
    float a3 = dinR1;

    int i_gain[3] = {0, 0, 0};
    i_gain[0] = (dinB1 * (1 << 8));
    i_gain[1] = (dinG1 * (1 << 8));
    i_gain[2] = (dinR1 * (1 << 8));

    for (i = 0; i < height; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN OFF
    // clang-format on
    ColLoop1:
        for (j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
            // clang-format on
            in_pix = src2.read(i * width + j);

            for (int p = 0; p < XF_NPIXPERCYCLE(NPC) * PLANES; p++) {
// clang-format off
#pragma HLS unroll
                // clang-format on
                ap_uint<8> val = in_pix.range(p * 8 + 7, p * 8);
                ap_uint<8> outval = (unsigned char)((val * i_gain[p % 3]) >> 8);

                out_pix.range(p * 8 + 7, p * 8) = outval;
            }

            dst.write(i * width + j, out_pix);
        }
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, bool WB_TYPE>
void balanceWhite(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src1,
                  xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src2,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC>& dst,
                  float thresh,
                  float inputMin,
                  float inputMax,
                  float outputMin,
                  float outputMax) {
#ifndef __SYNTHESIS__
    assert(((src1.rows == dst.rows) && (src1.cols == dst.cols)) && "Input and output image should be of same size");
    assert(((src1.rows <= ROWS) && (src1.cols <= COLS)) && "ROWS and COLS should be greater than input image");
#endif

    short width = src1.cols >> XF_BITSHIFT(NPC);

    if (WB_TYPE == 0) {
        balanceWhiteKernel<SRC_T, DST_T, ROWS, COLS, NPC, XF_CHANNELS(SRC_T, NPC), XF_DEPTH(SRC_T, NPC),
                           XF_DEPTH(DST_T, NPC), XF_WORDWIDTH(SRC_T, NPC), XF_WORDWIDTH(DST_T, NPC),
                           (COLS >> XF_BITSHIFT(NPC))>(src1, src2, dst, thresh, src1.rows, width);
    } else {
        balanceWhiteKernel_simple<SRC_T, DST_T, ROWS, COLS, NPC, XF_CHANNELS(SRC_T, NPC), XF_DEPTH(SRC_T, NPC),
                                  XF_DEPTH(DST_T, NPC), XF_WORDWIDTH(SRC_T, NPC), XF_WORDWIDTH(DST_T, NPC),
                                  (COLS >> XF_BITSHIFT(NPC))>(src1, src2, dst, thresh, src1.rows, width, inputMin,
                                                              inputMax, outputMin, outputMax);
    }
}
}
}
#endif //_XF_AWB_HPP_
