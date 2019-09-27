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

#include "xf_autowhitebalance_config.h"

extern "C" {
void autowhitebalance_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                            ap_uint<INPUT_PTR_WIDTH>* img_inp1,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                            float thresh,
                            int rows,
                            int cols,
                            float inputMin,
                            float inputMax,
                            float outputMin,
                            float outputMax) {
// clang-format off
#pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_inp1  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem3

#pragma HLS INTERFACE s_axilite port=thresh     bundle=control
#pragma HLS INTERFACE s_axilite port=inputMin     bundle=control
#pragma HLS INTERFACE s_axilite port=inputMax     bundle=control
#pragma HLS INTERFACE s_axilite port=outputMin     bundle=control
#pragma HLS INTERFACE s_axilite port=outputMax     bundle=control
#pragma HLS INTERFACE s_axilite port=rows     bundle=control
#pragma HLS INTERFACE s_axilite port=cols     bundle=control
#pragma HLS INTERFACE s_axilite port=return   bundle=control
    // clang-format on

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> in_mat;
// clang-format off
#pragma HLS stream variable=in_mat.data depth=2
    // clang-format on
    in_mat.rows = rows;
    in_mat.cols = cols;

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> in_mat1;
// clang-format off
#pragma HLS stream variable=in_mat1.data depth=2
    // clang-format on
    in_mat1.rows = rows;
    in_mat1.cols = cols;

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> out_mat;
// clang-format off
#pragma HLS stream variable=out_mat.data depth=2
    // clang-format on
    out_mat.rows = rows;
    out_mat.cols = cols;

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC3, HEIGHT, WIDTH, NPC1>(img_inp, in_mat);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC3, HEIGHT, WIDTH, NPC1>(img_inp1, in_mat1);

    xf::cv::balanceWhite<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC1, WB_TYPE>(in_mat, in_mat1, out_mat, thresh, inputMin,
                                                                          inputMax, outputMin, outputMax);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_8UC3, HEIGHT, WIDTH, NPC1>(out_mat, img_out);
}
}
