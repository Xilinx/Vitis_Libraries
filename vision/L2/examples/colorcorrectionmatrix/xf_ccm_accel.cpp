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

#include "xf_ccm_accel_config.h"
static constexpr int __XF_DEPTH = (HEIGHT * WIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_OUT = (HEIGHT * WIDTH * (XF_PIXELWIDTH(OUT_TYPE, NPPCX)) / 8) / (OUTPUT_PTR_WIDTH / 8);

extern "C" {

void ccm_accel(ap_uint<INPUT_PTR_WIDTH>* src,
               ap_uint<OUTPUT_PTR_WIDTH>* dst,
               signed int ccm_config_1[3][3],
               signed int ccm_config_2[3],
               int rows,
               int cols) {
// clang-format off
#pragma HLS INTERFACE m_axi      port=src        offset=slave  bundle=gmem0 depth=__XF_DEPTH
#pragma HLS INTERFACE s_axilite  port=rows
#pragma HLS INTERFACE s_axilite  port=cols 
#pragma HLS INTERFACE m_axi port=ccm_config_1   offset=slave bundle=gmem1      
#pragma HLS INTERFACE m_axi port=ccm_config_2   offset=slave bundle=gmem2 
#pragma HLS INTERFACE m_axi      port=dst   offset=slave  bundle=gmem1 depth=__XF_DEPTH_OUT
#pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(rows, cols);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(src, imgInput);

    xf::cv::colorcorrectionmatrix<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(
        imgInput, imgOutput, ccm_config_1, ccm_config_2);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, dst);
}
}