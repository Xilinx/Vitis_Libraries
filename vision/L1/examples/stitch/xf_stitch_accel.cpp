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

static constexpr int __XF_DEPTH_IN_1 =
    (HEIGHT_1 * WIDTH_1 * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_IN_2 =
    (HEIGHT_2 * WIDTH_2 * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_IN_3 =
    (HEIGHT_3 * WIDTH_3 * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_IN_4 =
    (HEIGHT_4 * WIDTH_4 * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_IN_MASK_1 =
    (HEIGHT_1 * WIDTH_1 * (XF_PIXELWIDTH(IN_MASK_TYPE, NPPCX)) / 8) / (MASK_INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_IN_MASK_2 =
    (HEIGHT_2 * WIDTH_2 * (XF_PIXELWIDTH(IN_MASK_TYPE, NPPCX)) / 8) / (MASK_INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_IN_MASK_3 =
    (HEIGHT_3 * WIDTH_3 * (XF_PIXELWIDTH(IN_MASK_TYPE, NPPCX)) / 8) / (MASK_INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_IN_MASK_4 =
    (HEIGHT_4 * WIDTH_4 * (XF_PIXELWIDTH(IN_MASK_TYPE, NPPCX)) / 8) / (MASK_INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_OUT_1 =
    (HEIGHT_DST * WIDTH_DST * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);

void stitch_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                  ap_uint<INPUT_PTR_WIDTH>* img_in2,
                  ap_uint<INPUT_PTR_WIDTH>* img_in3,
                  ap_uint<INPUT_PTR_WIDTH>* img_in4,
                  ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img1,
                  ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img2,
                  ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img3,
                  ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img4,
                  ap_uint<INPUT_PTR_WIDTH>* img_out,
                  int img_sizes[8],
                  int mask_corners[8],
                  int dst_height,
                  int dst_width) {
// clang-format off
#pragma HLS INTERFACE m_axi port=img_in1 offset=slave bundle=gmem0 depth=__XF_DEPTH_IN_1
#pragma HLS INTERFACE m_axi port=img_in2 offset=slave bundle=gmem1 depth=__XF_DEPTH_IN_2
#pragma HLS INTERFACE m_axi port=img_in3 offset=slave bundle=gmem2 depth=__XF_DEPTH_IN_3
#pragma HLS INTERFACE m_axi port=img_in4 offset=slave bundle=gmem3 depth=__XF_DEPTH_IN_4
#pragma HLS INTERFACE m_axi port=mask_img1 offset=slave bundle=gmem5 depth=__XF_DEPTH_IN_MASK_1
#pragma HLS INTERFACE m_axi port=mask_img2 offset=slave bundle=gmem6 depth=__XF_DEPTH_IN_MASK_2
#pragma HLS INTERFACE m_axi port=mask_img3 offset=slave bundle=gmem7 depth=__XF_DEPTH_IN_MASK_3
#pragma HLS INTERFACE m_axi port=mask_img4 offset=slave bundle=gmem8 depth=__XF_DEPTH_IN_MASK_4
#pragma HLS INTERFACE m_axi port=img_out offset=slave bundle=gmem9 depth=__XF_DEPTH_OUT_1
#pragma HLS INTERFACE s_axilite port=return bundle=control
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT_1, WIDTH_1, NPPCX, XF_CV_DEPTH_IN_1> imgInput1(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<IN_TYPE, HEIGHT_2, WIDTH_2, NPPCX, XF_CV_DEPTH_IN_1> imgInput2(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<IN_TYPE, HEIGHT_3, WIDTH_3, NPPCX, XF_CV_DEPTH_IN_1> imgInput3(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<IN_TYPE, HEIGHT_4, WIDTH_4, NPPCX, XF_CV_DEPTH_IN_1> imgInput4(img_sizes[6], img_sizes[7]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_1, WIDTH_1, NPPCX, XF_CV_DEPTH_IN_1> maskInput1(img_sizes[0], img_sizes[1]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_2, WIDTH_2, NPPCX, XF_CV_DEPTH_IN_1> maskInput2(img_sizes[2], img_sizes[3]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_3, WIDTH_3, NPPCX, XF_CV_DEPTH_IN_1> maskInput3(img_sizes[4], img_sizes[5]);
    xf::cv::Mat<IN_MASK_TYPE, HEIGHT_4, WIDTH_4, NPPCX, XF_CV_DEPTH_IN_1> maskInput4(img_sizes[6], img_sizes[7]);
#pragma HLS DATAFLOW

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT_1, WIDTH_1, NPPCX, XF_CV_DEPTH_IN_1>(img_in1, imgInput1);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT_2, WIDTH_2, NPPCX, XF_CV_DEPTH_IN_1>(img_in2, imgInput2);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT_3, WIDTH_3, NPPCX, XF_CV_DEPTH_IN_1>(img_in3, imgInput3);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT_4, WIDTH_4, NPPCX, XF_CV_DEPTH_IN_1>(img_in4, imgInput4);
    xf::cv::Array2xfMat<MASK_INPUT_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_1, WIDTH_1, NPPCX, XF_CV_DEPTH_IN_1>(mask_img1,
                                                                                                        maskInput1);
    xf::cv::Array2xfMat<MASK_INPUT_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_2, WIDTH_2, NPPCX, XF_CV_DEPTH_IN_1>(mask_img2,
                                                                                                        maskInput2);
    xf::cv::Array2xfMat<MASK_INPUT_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_3, WIDTH_3, NPPCX, XF_CV_DEPTH_IN_1>(mask_img3,
                                                                                                        maskInput3);
    xf::cv::Array2xfMat<MASK_INPUT_PTR_WIDTH, IN_MASK_TYPE, HEIGHT_4, WIDTH_4, NPPCX, XF_CV_DEPTH_IN_1>(mask_img4,
                                                                                                        maskInput4);

    xf::cv::Mat<IN_TYPE, HEIGHT_DST, WIDTH_DST, NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(dst_height, dst_width);

    // Run xfOpenCV kernel:
    xf::cv::stitch<IN_TYPE, IN_MASK_TYPE, HEIGHT_1, WIDTH_1, HEIGHT_2, WIDTH_2, HEIGHT_3, WIDTH_3, HEIGHT_4, WIDTH_4,
                   HEIGHT_DST, WIDTH_DST, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(
        imgInput1, imgInput2, imgInput3, imgInput4, maskInput1, maskInput2, maskInput3, maskInput4, mask_corners,
        imgOutput);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT_DST, WIDTH_DST, NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, img_out);

    return;
} // End of kernel
