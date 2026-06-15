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

#ifndef _XF_PRE_PROCESS_
#define _XF_PRE_PROCESS_

#include <cassert>
#include <cstring>
#include "ap_int.h"
#include "common/xf_params.hpp"
#include "common/xf_types.hpp"
#include "hls_stream.h"
#include <ap_float.h>
#include <utils/x_hls_utils.h>

typedef unsigned short uint16_t;

namespace xf {
namespace cv {

template <int W, int E>
ap_uint<W> get_bits2(ap_float<W, E>& f) {
    ap_uint<W> v = (f.sign_ref(), f.exponent_ref(), f.mantissa_ref());
    return v;
}
template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
          int WIDTH_A,
          int IBITS_A,
          int WIDTH_B,
          int IBITS_B,
          int WIDTH_OUT,
          int IBITS_OUT>
void xFpreProcessKernel_int8(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                             xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                             float alpha_reg[XF_CHANNELS(IN_TYPE, NPC)],
                             float beta_reg[XF_CHANNELS(IN_TYPE, NPC)],
                             int loop_count) {
    int BITDEPTH = XF_DTPIXELDEPTH(OUT_TYPE, NPC);
    XF_CTUNAME(IN_TYPE, NPC) x_1pix;

    for (int k = 0; k < loop_count; k++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT*WIDTH
        // clang-format on
        XF_TNAME(IN_TYPE, NPC) x_pack = in_mat.read(k);
        XF_TNAME(OUT_TYPE, NPC) out_pack = 0;

        for (int i = 0; i < XF_NPIXPERCYCLE(NPC); i++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            for (int j = 0; j < XF_CHANNELS(IN_TYPE, NPC); j++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                x_1pix = x_pack.range((j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) + (XF_DTPIXELDEPTH(IN_TYPE, NPC) - 1) +
                                          (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)),
                                      (j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) +
                                          (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)));

                // ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp_val = 0;
                ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>* out_val = NULL; // = &temp_val;

                constexpr int BITS_8 = ap_uint<8>::width;
                ap_ufixed<WIDTH_A, IBITS_A, AP_RND> a_int = (ap_ufixed<WIDTH_A, IBITS_A, AP_RND>)alpha_reg[j];
                ap_ufixed<WIDTH_B, IBITS_B, AP_RND> b_int = (ap_ufixed<WIDTH_B, IBITS_B, AP_RND>)beta_reg[j];
                ap_ufixed<WIDTH_OUT, IBITS_OUT, AP_RND> out_pix_int = 0;
                auto intr_result = (x_1pix - a_int) * b_int;
                out_pix_int = intr_result;
                // out_pix_int = (ap_ufixed<WIDTH_OUT, IBITS_OUT, AP_RND>)((ap_ufixed<WIDTH_OUT, IBITS_OUT,
                // AP_RND>)(x_1pix - a_int) * b_int);
                unsigned char* temp = reinterpret_cast<unsigned char*>(&out_pix_int);
                ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp1 = (*temp) << (BITDEPTH - 8);
                out_val = &temp1;

                out_pack.range((j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) + (XF_DTPIXELDEPTH(OUT_TYPE, NPC) - 1) +
                                   (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC)),
                               (j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) +
                                   (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC))) = *out_val;
            }
        }
        out_mat.write(k, out_pack);
    }
}

template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void xFpreProcessKernel_fp16(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                             xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                             float alpha_reg[XF_CHANNELS(IN_TYPE, NPC)],
                             float beta_reg[XF_CHANNELS(IN_TYPE, NPC)],
                             int loop_count) {
    int BITDEPTH = XF_DTPIXELDEPTH(OUT_TYPE, NPC);
    XF_CTUNAME(IN_TYPE, NPC) x_1pix;

    for (int k = 0; k < loop_count; k++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT*WIDTH
        // clang-format on
        XF_TNAME(IN_TYPE, NPC) x_pack = in_mat.read(k);
        XF_TNAME(OUT_TYPE, NPC) out_pack = 0;

        for (int i = 0; i < XF_NPIXPERCYCLE(NPC); i++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            for (int j = 0; j < XF_CHANNELS(IN_TYPE, NPC); j++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                x_1pix = x_pack.range((j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) + (XF_DTPIXELDEPTH(IN_TYPE, NPC) - 1) +
                                          (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)),
                                      (j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) +
                                          (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)));

                // ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp_val = 0;
                ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>* out_val = NULL; // = &temp_val;

                half a_half = (half)alpha_reg[j];
                half b_half = (half)beta_reg[j];
                half out_pix_half = 0;
                out_pix_half = (half)(((half)x_1pix - a_half) * b_half);
                unsigned short* temp_fp = reinterpret_cast<unsigned short*>(&out_pix_half);
                ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp1_fp = (*temp_fp) << (BITDEPTH - 16);
                out_val = &temp1_fp;

                out_pack.range((j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) + (XF_DTPIXELDEPTH(OUT_TYPE, NPC) - 1) +
                                   (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC)),
                               (j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) +
                                   (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC))) = *out_val;
            }
        }
        out_mat.write(k, out_pack);
    }
}
// BF16
template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void xFpreProcessKernel_bf16(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                             xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                             ap_float<16, 8> alpha_reg[XF_CHANNELS(IN_TYPE, NPC)],
                             ap_float<16, 8> beta_reg[XF_CHANNELS(IN_TYPE, NPC)],
                             int loop_count) {
    int BITDEPTH = XF_DTPIXELDEPTH(OUT_TYPE, NPC);
    XF_CTUNAME(IN_TYPE, NPC) x_1pix = 0;

    for (int k = 0; k < loop_count; k++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT*WIDTH
        // clang-format on
        XF_TNAME(IN_TYPE, NPC) x_pack = in_mat.read(k);
        XF_TNAME(OUT_TYPE, NPC) out_pack = 0;

        for (int i = 0; i < XF_NPIXPERCYCLE(NPC); i++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            for (int j = 0; j < XF_CHANNELS(IN_TYPE, NPC); j++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                x_1pix = x_pack.range((j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) + (XF_DTPIXELDEPTH(IN_TYPE, NPC) - 1) +
                                          (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)),
                                      (j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) +
                                          (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)));

                // ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp_val = 0;
                ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>* out_val = NULL; // = &temp_val;

                constexpr int BITS_16 = ap_uint<16>::width;
                ap_float<16, 8> a_bf = (ap_float<16, 8>)alpha_reg[j];
                ap_float<16, 8> b_bf = (ap_float<16, 8>)beta_reg[j];

                ap_float<16, 8> out_pix_bf = 0;
                out_pix_bf = (ap_float<16, 8>)(((ap_float<16, 8>)x_1pix - a_bf) * b_bf);

                ap_uint<16> rawbits = get_bits2(out_pix_bf);
                ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> rawbits_ext = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>)rawbits
                                                                      << (BITDEPTH - 16);
                out_val = &rawbits_ext;

                /*float temp = (float)out_pix_bf;

                unsigned int *temp_int = reinterpret_cast<unsigned int*>(&temp);
                out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> *)temp_int;*/

                /*unsigned short* temp_bf = reinterpret_cast<unsigned short*>(&out_pix_bf);
                ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp1_bf = (temp_bf)<< (BITDEPTH - 16);
                out_val = &temp1_bf;*/

                out_pack.range((j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) + (XF_DTPIXELDEPTH(OUT_TYPE, NPC) - 1) +
                                   (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC)),
                               (j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) +
                                   (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC))) = *out_val;
            }
        }
        out_mat.write(k, out_pack);
    }
}

template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void xFpreProcessKernel_fp32(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                             xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                             float alpha_reg[XF_CHANNELS(IN_TYPE, NPC)],
                             float beta_reg[XF_CHANNELS(IN_TYPE, NPC)],
                             int loop_count) {
    int BITDEPTH = XF_DTPIXELDEPTH(OUT_TYPE, NPC);
    XF_CTUNAME(IN_TYPE, NPC) x_1pix;

    for (int k = 0; k < loop_count; k++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT*WIDTH
        // clang-format on
        XF_TNAME(IN_TYPE, NPC) x_pack = in_mat.read(k);
        XF_TNAME(OUT_TYPE, NPC) out_pack = 0;

        for (int i = 0; i < XF_NPIXPERCYCLE(NPC); i++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            for (int j = 0; j < XF_CHANNELS(IN_TYPE, NPC); j++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                x_1pix = x_pack.range((j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) + (XF_DTPIXELDEPTH(IN_TYPE, NPC) - 1) +
                                          (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)),
                                      (j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) +
                                          (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)));

                // ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp_val = 0;
                ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>* out_val = NULL; // = &temp_val;

                float a = alpha_reg[j];
                float b = beta_reg[j];
                float out_1pix = 0;
                out_1pix = (float)(((float)x_1pix - a) * b);
                out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>*)&out_1pix;

                out_pack.range((j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) + (XF_DTPIXELDEPTH(OUT_TYPE, NPC) - 1) +
                                   (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC)),
                               (j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) +
                                   (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC))) = *out_val;
            }
        }
        out_mat.write(k, out_pack);
    }
}

template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int WIDTH_A,
          int IBITS_A,
          int WIDTH_B,
          int IBITS_B,
          int WIDTH_OUT,
          int IBITS_OUT,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void preProcess_int8(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                     xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                     float params[2 * XF_CHANNELS(IN_TYPE, NPC)]) {
#pragma HLS INLINE OFF

#ifndef __SYNTHESIS__

/*    if ((SELECT & (1 << datatype)) == 0) {
        assert(false && "Runtime datatype not supported as it was not selected during compile time");
    }*/
#endif

    // ap_ufixed<WIDTH_A, IBITS_A, AP_RND> alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
    // ap_fixed<WIDTH_B, IBITS_B, AP_RND> beta_reg[XF_CHANNELS(IN_TYPE, NPC)];
    float alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
    float beta_reg[XF_CHANNELS(IN_TYPE, NPC)];

// clang-format off
#pragma HLS ARRAY_PARTITION variable=alpha_reg dim=0 complete
#pragma HLS ARRAY_PARTITION variable=beta_reg dim=0 complete

    // clang-format on
    int channels = XF_CHANNELS(IN_TYPE, NPC);
    for (int i = 0; i < 2 * XF_CHANNELS(IN_TYPE, NPC); i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=12
#pragma HLS PIPELINE II=1
        // clang-format on
        float temp = params[i];
        if (i % 2 == 0)
            alpha_reg[i / 2] = temp;
        else
            beta_reg[i / 2] = temp;
    }

    // clang-format off
//#pragma HLS DATAFLOW
    // clang-format on

    uint16_t width = in_mat.cols >> XF_BITSHIFT(NPC);
    uint16_t height = in_mat.rows;

    int loop_count = width * height;

    xFpreProcessKernel_int8<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN, XFCVDEPTH_OUT, WIDTH_A, IBITS_A,
                            WIDTH_B, IBITS_B, WIDTH_OUT, IBITS_OUT>(in_mat, out_mat, alpha_reg, beta_reg, loop_count);
}

template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void preProcess_fp16(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                     xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                     float params[2 * XF_CHANNELS(IN_TYPE, NPC)]) {
#pragma HLS INLINE OFF

#ifndef __SYNTHESIS__

/*    if ((SELECT & (1 << datatype)) == 0) {
        assert(false && "Runtime datatype not supported as it was not selected during compile time");
    }*/
#endif

    // ap_ufixed<WIDTH_A, IBITS_A, AP_RND> alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
    // ap_fixed<WIDTH_B, IBITS_B, AP_RND> beta_reg[XF_CHANNELS(IN_TYPE, NPC)];
    float alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
    float beta_reg[XF_CHANNELS(IN_TYPE, NPC)];

// clang-format off
#pragma HLS ARRAY_PARTITION variable=alpha_reg dim=0 complete
#pragma HLS ARRAY_PARTITION variable=beta_reg dim=0 complete

    // clang-format on
    int channels = XF_CHANNELS(IN_TYPE, NPC);
    for (int i = 0; i < 2 * XF_CHANNELS(IN_TYPE, NPC); i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=12
#pragma HLS PIPELINE II=1
        // clang-format on
        float temp = params[i];
        if (i % 2 == 0)
            alpha_reg[i / 2] = temp;
        else
            beta_reg[i / 2] = temp;
    }

    // clang-format off
//#pragma HLS DATAFLOW
    // clang-format on

    uint16_t width = in_mat.cols >> XF_BITSHIFT(NPC);
    uint16_t height = in_mat.rows;

    int loop_count = width * height;

    xFpreProcessKernel_fp16<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN, XFCVDEPTH_OUT>(
        in_mat, out_mat, alpha_reg, beta_reg, loop_count);
}

template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void preProcess_bf16(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                     xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                     float params[2 * XF_CHANNELS(IN_TYPE, NPC)]) {
//#pragma HLS INLINE OFF

#ifndef __SYNTHESIS__

/*    if ((SELECT & (1 << datatype)) == 0) {
        assert(false && "Runtime datatype not supported as it was not selected during compile time");
    }*/
#endif

    // ap_ufixed<WIDTH_A, IBITS_A, AP_RND> alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
    // ap_fixed<WIDTH_B, IBITS_B, AP_RND> beta_reg[XF_CHANNELS(IN_TYPE, NPC)];
    ap_float<16, 8> alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
    ap_float<16, 8> beta_reg[XF_CHANNELS(IN_TYPE, NPC)];

// clang-format off
#pragma HLS ARRAY_PARTITION variable=alpha_reg dim=0 complete
#pragma HLS ARRAY_PARTITION variable=beta_reg dim=0 complete

    // clang-format on
    int channels = XF_CHANNELS(IN_TYPE, NPC);
    for (int i = 0; i < 2 * XF_CHANNELS(IN_TYPE, NPC); i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=12
#pragma HLS PIPELINE II=1
        // clang-format on
        float temp = params[i];
        if (i % 2 == 0)
            alpha_reg[i / 2] = (ap_float<16, 8>)temp;
        else
            beta_reg[i / 2] = (ap_float<16, 8>)temp;
    }

    // clang-format off
//#pragma HLS DATAFLOW
    // clang-format on

    uint16_t width = in_mat.cols >> XF_BITSHIFT(NPC);
    uint16_t height = in_mat.rows;

    int loop_count = width * height;

    xFpreProcessKernel_bf16<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN, XFCVDEPTH_OUT>(
        in_mat, out_mat, alpha_reg, beta_reg, loop_count);
}
template <int IN_TYPE,
          int OUT_TYPE,
          int HEIGHT,
          int WIDTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
void preProcess_fp32(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                     xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                     float params[2 * XF_CHANNELS(IN_TYPE, NPC)]) {
#pragma HLS INLINE OFF

#ifndef __SYNTHESIS__

/*    if ((SELECT & (1 << datatype)) == 0) {
        assert(false && "Runtime datatype not supported as it was not selected during compile time");
    }*/
#endif

    // ap_ufixed<WIDTH_A, IBITS_A, AP_RND> alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
    // ap_fixed<WIDTH_B, IBITS_B, AP_RND> beta_reg[XF_CHANNELS(IN_TYPE, NPC)];
    float alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
    float beta_reg[XF_CHANNELS(IN_TYPE, NPC)];

// clang-format off
#pragma HLS ARRAY_PARTITION variable=alpha_reg dim=0 complete
#pragma HLS ARRAY_PARTITION variable=beta_reg dim=0 complete

    // clang-format on
    int channels = XF_CHANNELS(IN_TYPE, NPC);
    for (int i = 0; i < 2 * XF_CHANNELS(IN_TYPE, NPC); i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=12
#pragma HLS PIPELINE II=1
        // clang-format on
        float temp = params[i];
        if (i % 2 == 0)
            alpha_reg[i / 2] = temp;
        else
            beta_reg[i / 2] = temp;
    }

    // clang-format off
//#pragma HLS DATAFLOW
    // clang-format on

    uint16_t width = in_mat.cols >> XF_BITSHIFT(NPC);
    uint16_t height = in_mat.rows;

    int loop_count = width * height;

    xFpreProcessKernel_fp32<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN, XFCVDEPTH_OUT>(
        in_mat, out_mat, alpha_reg, beta_reg, loop_count);
}
}
}

#endif

/*
This commented out code is more efficient in coding style,
but the runtime datatype selection through 'datatype'
variable does not happen correctly when this kernel is
used in Vivado IP flow with Linux drivers. The 'datatype' value becomes garbage
once it is parsed from accel to kernel. Reasons not known yet.*/ /*
 template <typename XF_T,
                   int SELECT,
                   int IN_TYPE,
           int OUT_TYPE,
           int HEIGHT,
           int WIDTH,
           int NPC,
           int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
           int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT,
           int WIDTH_A,
           int IBITS_A,
           int WIDTH_B,
           int IBITS_B,
           int WIDTH_OUT,
           int IBITS_OUT>
 void xFpreProcessKernel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                         xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                                                 float alpha_reg[XF_CHANNELS(IN_TYPE, NPC)],
                                                 float beta_reg[XF_CHANNELS(IN_TYPE, NPC)],
                         int loop_count, int datatype) {

     int BITDEPTH = XF_DTPIXELDEPTH(OUT_TYPE, NPC);
     XF_CTUNAME(IN_TYPE, NPC) x_1pix;

     for (int k = 0; k < loop_count; k++) {
 // clang-format off
 #pragma HLS PIPELINE II=1
 #pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT*WIDTH
         // clang-format on
         XF_TNAME(IN_TYPE, NPC) x_pack = in_mat.read(k);
         XF_TNAME(OUT_TYPE, NPC) out_pack = 0;
         XF_T out_pix = 0;
         for (int i = 0; i < XF_NPIXPERCYCLE(NPC); i++) {
 // clang-format off
 #pragma HLS UNROLL
             // clang-format on
             for (int j = 0; j < XF_CHANNELS(IN_TYPE, NPC); j++) {
 // clang-format off
 #pragma HLS UNROLL
                 // clang-format on
                 x_1pix = x_pack.range((j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) + (XF_DTPIXELDEPTH(IN_TYPE, NPC) - 1) +
                                           (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)),
                                       (j * XF_DTPIXELDEPTH(IN_TYPE, NPC)) +
                                           (i * XF_CHANNELS(IN_TYPE, NPC) * XF_DTPIXELDEPTH(IN_TYPE, NPC)));

                 //ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp_val = 0;
                 ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> *out_val = NULL;// = &temp_val;

             if(SELECT & 0x0001) // INT8
             {
                 if(datatype == data_types::INT8){ // Runtime
                     constexpr int BITS_8 = ap_uint<8>::width;
                     ap_ufixed<WIDTH_A, IBITS_A, AP_RND> a_int = (ap_ufixed<WIDTH_A, IBITS_A, AP_RND>)alpha_reg[j];
                     ap_ufixed<WIDTH_A, IBITS_A, AP_RND> b_int = (ap_ufixed<WIDTH_A, IBITS_A, AP_RND>)beta_reg[j];
                     ap_ufixed<WIDTH_A, IBITS_A, AP_RND> out_pix_int = 0;
                     out_pix_int = (ap_ufixed<WIDTH_A, IBITS_A, AP_RND>)((ap_ufixed<WIDTH_A, IBITS_A, AP_RND>)(x_1pix -
 a_int) * b_int);
                     unsigned char* temp = reinterpret_cast<unsigned char*>(&out_pix_int);
                     ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp1 = (*temp)<< (BITDEPTH - 8);
                     out_val = &temp1;

                     //ap_uint8_t out_pix = *(ap_uint8_t *)&out_pix_int;
                     //(*out_val).range(BITDEPTH-1, BITDEPTH-BITS_8) = out_pix_int.range(7,0);
                     //ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp = out_pix_int;
                     //out_val = &temp;
                     //out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>*)&(out_pix_int);
                     //*out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>)((*out_val) << (BITDEPTH - 8));

                 }
             } //
             if(SELECT & 0x0002) // FP16
             { // Compile time
                 if(datatype == data_types::FP16){ // Runtime
                     //constexpr int BITS_16 = ap_uint<16>::width;
                     half a_half = (half)alpha_reg[j];
                     half b_half = (half)beta_reg[j];
                     half out_pix_half = 0;
                     out_pix_half = (half)(((half)x_1pix - a_half) * b_half);
                     unsigned short* temp_fp = reinterpret_cast<unsigned short*>(&out_pix_half);
                     ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp1_fp = (*temp_fp)<< (BITDEPTH - 16);
                     out_val = &temp1_fp;
                     //ap_uint16_t out_pix = *(ap_uint16_t *)&out_pix_half;
                     //(*out_val).range(BITDEPTH-1, BITDEPTH-BITS_16) = out_pix;
                     //out_val.range(BITDEPTH-1, BITDEPTH-BITS_16) = out_pix_half.range(15,0);
                     //out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>*)&(out_pix_half);
                     //*out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>)((*out_val) << (BITDEPTH - 16));

                 }
                 }//
                 if(SELECT & 0x0004) // BF16
                 {
                     if(datatype == data_types::BF16){ // Runtime
                         constexpr int BITS_16 = ap_uint<16>::width;
                         ap_float<16,8> a_bf = (ap_float<16,8>)alpha_reg[j];
                         ap_float<16,8> b_bf = (ap_float<16,8>)beta_reg[j];

                         ap_float<16,8> out_pix_bf = 0;
                         out_pix_bf = (ap_float<16,8>)(((ap_float<16,8>)x_1pix - a_bf) * b_bf);
                         //ap_uint16_t out_pix = *(ap_uint16_t *)&out_pix_bf;
                         unsigned short* temp_bf = reinterpret_cast<unsigned short*>(&out_pix_bf);
                         ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> temp1_bf = (*temp_bf)<< (BITDEPTH - 16);
                         out_val = &temp1_bf;
                         //(*out_val).range(BITDEPTH-1, BITDEPTH-BITS_16) = out_pix;
                         //float k = out_pix_bf.to_float();
                         //short m = (short)k;

                         //out_val = static_cast< ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)> > (k);

                         //unsigned short out_p;
                         //&out_p = reinterpret_cast<unsigned short *>(&out_pix_bf);
                         //out_val.range(BITDEPTH-1, BITDEPTH-BITS_16) = out_p;

                         //out_pix.range(15,15) = out_pix_bf.sign();
                         //out_pix.range(14,7) = out_pix_bf.exp();
                         //out_pix.range(6,0) = out_pix_bf.mantissa();
                         //out_val = out_val << (BITDEPTH-BITS_16);
                         //ap_uint<16> *out_pix = NULL;
                         //out_pix = (ap_uint<16> *)&(out_pix_bf);
                         //memcpy(&out_val, &out_pix_bf, 2);
                         //out_val = out_val << (BITDEPTH-BITS_16);
                         //out_val.range(BITDEPTH-1, BITDEPTH-BITS_16) = *out_pix;//_bf;//(15,0);
                         //out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>*)&(out_pix);
                         //*out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>)((*out_val) << (BITDEPTH - 16));

                     }
                 }//

                 if(SELECT & 0x0008) // FP32
                 {
                     if(datatype == data_types::FP32){
                         float a = alpha_reg[j];
                         float b = beta_reg[j];
                         float out_1pix = 0;
                         out_1pix = (float)(((float)x_1pix - a) * b);
                         out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>*)&out_1pix;
                         //ap_uint32_t out_pix = *(ap_uint32_t *)&out_1pix;
                         //*out_val = (ap_uint<XF_DTPIXELDEPTH(OUT_TYPE, NPC)>)out_pix;
                         //out_val.range(BITDEPTH-1, 0) = out_1pix;
                     }
                 }//
                                 out_pack.range((j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) + (XF_DTPIXELDEPTH(OUT_TYPE, NPC) -
 1) +
                                    (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC)),
                                (j * XF_DTPIXELDEPTH(OUT_TYPE, NPC)) +
                                    (i * XF_CHANNELS(OUT_TYPE, NPC) * XF_DTPIXELDEPTH(OUT_TYPE, NPC))) = *out_val;

             }
         }
         out_mat.write(k, out_pack);
     }
 }

 template <int SELECT,
           int IN_TYPE,
           int OUT_TYPE,
           int HEIGHT,
           int WIDTH,
           int NPC,
           int WIDTH_A,
           int IBITS_A,
           int WIDTH_B,
           int IBITS_B,
           int WIDTH_OUT,
           int IBITS_OUT,
           int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
           int XFCVDEPTH_OUT = _XFCVDEPTH_DEFAULT>
 void preProcess(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN>& in_mat,
                 xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_OUT>& out_mat,
                 float params[2 * XF_CHANNELS(IN_TYPE, NPC)]) {
 #pragma HLS INLINE OFF

 #ifndef __SYNTHESIS__

     if ((SELECT & (1 << datatype)) == 0) {
         assert(false && "Runtime datatype not supported as it was not selected during compile time");
     }
 #endif

     //ap_ufixed<WIDTH_A, IBITS_A, AP_RND> alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
     //ap_fixed<WIDTH_B, IBITS_B, AP_RND> beta_reg[XF_CHANNELS(IN_TYPE, NPC)];
     float alpha_reg[XF_CHANNELS(IN_TYPE, NPC)];
     float beta_reg[XF_CHANNELS(IN_TYPE, NPC)];

 // clang-format off
 #pragma HLS ARRAY_PARTITION variable=alpha_reg dim=0 complete
 #pragma HLS ARRAY_PARTITION variable=beta_reg dim=0 complete

     // clang-format on
     int channels = XF_CHANNELS(IN_TYPE, NPC);
     for (int i = 0; i < 2 * XF_CHANNELS(IN_TYPE, NPC); i++) {
 // clang-format off
 #pragma HLS LOOP_TRIPCOUNT min=1 max=12
 #pragma HLS PIPELINE II=1
         // clang-format on
         float temp = params[i];
         if (i % 2 == 0)
             alpha_reg[i/2] = temp;
         else
             beta_reg[i/2] = temp;
     }

     // clang-format off
 //#pragma HLS DATAFLOW
     // clang-format on

     uint16_t width = in_mat.cols >> XF_BITSHIFT(NPC);
     uint16_t height = in_mat.rows;

     int loop_count = width * height;

     xFpreProcessKernel<SELECT, IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC, XFCVDEPTH_IN, XFCVDEPTH_OUT, WIDTH_A, IBITS_A,
 WIDTH_B,
                        IBITS_B, WIDTH_OUT, IBITS_OUT>(in_mat, out_mat, alpha_reg, beta_reg, loop_count);
 }*/
