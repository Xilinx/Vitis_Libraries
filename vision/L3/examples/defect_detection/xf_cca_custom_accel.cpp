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

#include "xf_cca_custom_accel_config.h"

extern "C" {
void cca_custom_accel(
    uint8_t* in_ptr, uint8_t* fwd_ptr, uint8_t* out_ptr, int* def_pix, int height, int width, int stride) {
// clang-format off
    #pragma HLS INTERFACE m_axi     port=in_ptr  offset=slave bundle=gmem1 
    #pragma HLS INTERFACE m_axi     port=fwd_ptr  offset=slave bundle=gmem2 
    #pragma HLS INTERFACE m_axi     port=out_ptr  offset=slave bundle=gmem3 
    #pragma HLS INTERFACE m_axi port=def_pix offset=slave bundle=gmem4 
    #pragma HLS INTERFACE s_axilite port=height
    #pragma HLS INTERFACE s_axilite port=width
#pragma HLS INTERFACE s_axilite port=stride
    #pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> rev_out_mat(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    int tmp_def;

    xf::cv::rev_cca<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT>(in_ptr, rev_out_mat, height, width);
    xf::cv::pass_2<OUT_TYPE, HEIGHT, WIDTH, STRIDE, XF_NPPCX, XF_CV_DEPTH_OUT>(fwd_ptr, rev_out_mat, out_ptr, tmp_def,
                                                                               height, width, stride);

    // xf::cv::ccaCustom<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX>(fwd_ptr, in_ptr, rev_out_mat, out_ptr, tmp_def, height,
    //                               width);
    //*obj_pix = tmp_obj;
    *def_pix = tmp_def;
}
}
