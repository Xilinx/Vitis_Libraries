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

#include "xf_custom_bgr2y8_accel_config.h"
#include <iostream>
#include <stdlib.h>

static constexpr int __XF_DEPTH_IN = (HEIGHT * WIDTH * XF_PIXELWIDTH(IN_TYPE, NPPCX)) / INPUT_PTR_WIDTH;
static constexpr int __XF_DEPTH_OUT = (HEIGHT * WIDTH * XF_PIXELWIDTH(OUT_TYPE, NPPCX)) / OUTPUT_PTR_WIDTH;

void custom_bgr2y8_accel(
    ap_uint<INPUT_PTR_WIDTH>* src, ap_uint<OUTPUT_PTR_WIDTH>* dst, ap_uint<8>* array_params, int height, int width) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=src  offset=slave bundle=gmem0 depth=__XF_DEPTH_IN
#pragma HLS INTERFACE m_axi     port=dst offset=slave bundle=gmem1 depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE m_axi     port=array_params depth=12
#pragma HLS INTERFACE s_axilite port=height
#pragma HLS INTERFACE s_axilite port=width
#pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput(height, width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(height, width);

    struct bgr2y8_params pxl_val;

    // clang-format off
//#pragma HLS AGGREGATE variable=pxl_val
    // clang-format on

    pxl_val.black_Vmax = array_params[0];
    pxl_val.black_Smax = array_params[1];
    pxl_val.brown_Hmax = array_params[2];
    pxl_val.brown_Vmax = array_params[3];
    pxl_val.Smin = array_params[4];
    pxl_val.Smax = array_params[5];
    pxl_val.darkgreen_Vmax = array_params[6];
    pxl_val.darkgreen_Hmin = array_params[7];
    pxl_val.darkgreen_Hmax = array_params[8];
    pxl_val.green_Hmax = array_params[9];
    pxl_val.green_Hmin = array_params[10];
    pxl_val.green_Vmax = array_params[11];

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(src, imgInput);

    custom_bgr2y8<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(imgInput, imgOutput,
                                                                                                pxl_val);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, dst);

    return;
} // End of kernel
