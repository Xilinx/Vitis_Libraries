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

#include "xf_canny_config.h"

extern "C" {
void canny_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                 ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                 int rows,
                 int cols,
                 int low_threshold,
                 int high_threshold) {
// clang-format off
    #pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem2

// clang-format on

// clang-format off
    #pragma HLS INTERFACE s_axilite port=rows     
    #pragma HLS INTERFACE s_axilite port=cols     
    #pragma HLS INTERFACE s_axilite port=low_threshold     
    #pragma HLS INTERFACE s_axilite port=high_threshold     
    #pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, INTYPE> in_mat(rows, cols);
// clang-format off
    #pragma HLS stream variable=in_mat.data depth=2
    // clang-format on

    xf::cv::Mat<XF_2UC1, HEIGHT, WIDTH, XF_NPPC32> dst_mat(rows, cols);
// clang-format off
    #pragma HLS stream variable=dst_mat.data depth=2
// clang-format on

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC1, HEIGHT, WIDTH, INTYPE>(img_inp, in_mat);
    xf::cv::Canny<FILTER_WIDTH, NORM_TYPE, XF_8UC1, XF_2UC1, HEIGHT, WIDTH, INTYPE, XF_NPPC32, XF_USE_URAM>(
        in_mat, dst_mat, low_threshold, high_threshold);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_2UC1, HEIGHT, WIDTH, XF_NPPC32>(dst_mat, img_out);
}
}
