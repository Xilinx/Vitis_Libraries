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

#include "xf_stitch_accel_config.h"

extern "C" {

void stitch_accel(ap_uint<SRC_PTR_WIDTH>* img_in1,
                  ap_uint<SRC_PTR_WIDTH>* img_in2,
                  ap_uint<SRC_PTR_WIDTH>* img_in3,
                  ap_uint<SRC_PTR_WIDTH>* img_in4,
                  ap_uint<MASK_PTR_WIDTH>* mask_in1,
                  ap_uint<MASK_PTR_WIDTH>* mask_in2,
                  ap_uint<MASK_PTR_WIDTH>* mask_in3,
                  ap_uint<MASK_PTR_WIDTH>* mask_in4,
                  ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                  int* img_sizes,
                  int* mask_corners,
                  int dst_height,
                  int dst_width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in1      offset=slave bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=img_in2      offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=img_in3      offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi      port=img_in4      offset=slave bundle=gmem3
    #pragma HLS INTERFACE m_axi      port=mask_in1     offset=slave bundle=gmem4
    #pragma HLS INTERFACE m_axi      port=mask_in2     offset=slave bundle=gmem5
    #pragma HLS INTERFACE m_axi      port=mask_in3     offset=slave bundle=gmem6
    #pragma HLS INTERFACE m_axi      port=mask_in4     offset=slave bundle=gmem7
    #pragma HLS INTERFACE m_axi      port=img_out      offset=slave bundle=gmem8
    #pragma HLS INTERFACE m_axi      port=img_sizes    offset=slave bundle=gmem9
    #pragma HLS INTERFACE m_axi      port=mask_corners offset=slave bundle=gmem10
    #pragma HLS INTERFACE s_axilite  port=dst_height
    #pragma HLS INTERFACE s_axilite  port=dst_width
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT_1, WIDTH_1, NPPCX, XF_CV_DEPTH_IN_1> img1(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<IN_TYPE, HEIGHT_2, WIDTH_2, NPPCX, XF_CV_DEPTH_IN_1> img2(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<IN_TYPE, HEIGHT_3, WIDTH_3, NPPCX, XF_CV_DEPTH_IN_1> img3(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<IN_TYPE, HEIGHT_4, WIDTH_4, NPPCX, XF_CV_DEPTH_IN_1> img4(img_sizes[6], img_sizes[7]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_1, WIDTH_1, NPPCX, XF_CV_DEPTH_IN_1> msk1(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_2, WIDTH_2, NPPCX, XF_CV_DEPTH_IN_1> msk2(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_3, WIDTH_3, NPPCX, XF_CV_DEPTH_IN_1> msk3(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_4, WIDTH_4, NPPCX, XF_CV_DEPTH_IN_1> msk4(img_sizes[6], img_sizes[7]);
    xf::cv::Mat<IN_TYPE, HEIGHT_DST, WIDTH_DST, NPPCX, XF_CV_DEPTH_OUT_1> imgOut(dst_height, dst_width);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<SRC_PTR_WIDTH, IN_TYPE, HEIGHT_1, WIDTH_1, NPPCX, XF_CV_DEPTH_IN_1>(img_in1, img1);
    xf::cv::Array2xfMat<SRC_PTR_WIDTH, IN_TYPE, HEIGHT_2, WIDTH_2, NPPCX, XF_CV_DEPTH_IN_1>(img_in2, img2);
    xf::cv::Array2xfMat<SRC_PTR_WIDTH, IN_TYPE, HEIGHT_3, WIDTH_3, NPPCX, XF_CV_DEPTH_IN_1>(img_in3, img3);
    xf::cv::Array2xfMat<SRC_PTR_WIDTH, IN_TYPE, HEIGHT_4, WIDTH_4, NPPCX, XF_CV_DEPTH_IN_1>(img_in4, img4);
    xf::cv::Array2xfMat<MASK_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_1, WIDTH_1, NPPCX, XF_CV_DEPTH_IN_1>(mask_in1, msk1);
    xf::cv::Array2xfMat<MASK_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_2, WIDTH_2, NPPCX, XF_CV_DEPTH_IN_1>(mask_in2, msk2);
    xf::cv::Array2xfMat<MASK_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_3, WIDTH_3, NPPCX, XF_CV_DEPTH_IN_1>(mask_in3, msk3);
    xf::cv::Array2xfMat<MASK_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_4, WIDTH_4, NPPCX, XF_CV_DEPTH_IN_1>(mask_in4, msk4);

    int corners[8];
    for (int k = 0; k < 8; k++) {
// clang-format off
        #pragma HLS PIPELINE II=1
        // clang-format on
        corners[k] = mask_corners[k];
    }

    xf::cv::stitch<IN_TYPE, IN_MASK_TYPE, HEIGHT_1, WIDTH_1, HEIGHT_2, WIDTH_2, HEIGHT_3, WIDTH_3, HEIGHT_4, WIDTH_4,
                   HEIGHT_DST, WIDTH_DST, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(
        img1, img2, img3, img4, msk1, msk2, msk3, msk4, corners, imgOut);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, IN_TYPE, HEIGHT_DST, WIDTH_DST, NPPCX, XF_CV_DEPTH_OUT_1>(imgOut, img_out);

    return;
}

} // extern "C"
