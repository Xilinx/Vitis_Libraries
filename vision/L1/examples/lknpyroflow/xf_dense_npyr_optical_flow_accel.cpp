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

#include "xf_dense_npyr_optical_flow_accel_config.h"

static constexpr int _XF_DEPTH_IN =
    (MAX_HEIGHT * MAX_WIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int _XF_DEPTH_OUT =
    (MAX_HEIGHT * MAX_WIDTH * (XF_PIXELWIDTH(OUT_TYPE, NPPCX)) / 32) / (OUTPUT_PTR_WIDTH / 32);

void dense_non_pyr_of_accel(ap_uint<INPUT_PTR_WIDTH>* img_curr,
                            ap_uint<INPUT_PTR_WIDTH>* img_prev,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_outx,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_outy,
                            int rows,
                            int cols) {
// clang-format off
    #pragma HLS INTERFACE m_axi     port=img_curr  offset=slave bundle=gmem1 depth=_XF_DEPTH_IN
    #pragma HLS INTERFACE m_axi     port=img_prev  offset=slave bundle=gmem2 depth=_XF_DEPTH_IN
    #pragma HLS INTERFACE m_axi     port=img_outx  offset=slave bundle=gmem3 depth=_XF_DEPTH_OUT
    #pragma HLS INTERFACE m_axi     port=img_outy  offset=slave bundle=gmem4 depth=_XF_DEPTH_OUT
    #pragma HLS INTERFACE s_axilite port=cols  
    #pragma HLS INTERFACE s_axilite port=rows  
    #pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_CV_DEPTH_IN_CURR> in_curr_mat(rows, cols);
    xf::cv::Mat<IN_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_CV_DEPTH_IN_PREV> in_prev_mat(rows, cols);
    xf::cv::Mat<OUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_CV_DEPTH_OUTX> outx_mat(rows, cols);
    xf::cv::Mat<OUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_CV_DEPTH_OUTY> outy_mat(rows, cols);
// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_CV_DEPTH_IN_CURR>(img_curr,
                                                                                                     in_curr_mat);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_CV_DEPTH_IN_PREV>(img_prev,
                                                                                                     in_prev_mat);

    xf::cv::DenseNonPyrLKOpticalFlow<KMED, IN_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_USE_URAM, XF_CV_DEPTH_IN_CURR,
                                     XF_CV_DEPTH_IN_PREV, XF_CV_DEPTH_OUTX, XF_CV_DEPTH_OUTY>(in_curr_mat, in_prev_mat,
                                                                                              outx_mat, outy_mat);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_CV_DEPTH_OUTX>(outx_mat, img_outx);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, MAX_HEIGHT, MAX_WIDTH, NPPCX, XF_CV_DEPTH_OUTY>(outy_mat, img_outy);
}
