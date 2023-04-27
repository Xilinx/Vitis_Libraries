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

#include "xf_gaussian_diff_accel_config.h"

extern "C" {

void gaussiandiference(ap_uint<INPUT_PTR_WIDTH>* img_in,
                       float sigma1,
                       float sigma2,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                       int rows,
                       int cols) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem1  
    #pragma HLS INTERFACE s_axilite  port=sigma1 			          
    #pragma HLS INTERFACE s_axilite  port=sigma2 			          
	#pragma HLS INTERFACE s_axilite  port=rows 			          
	#pragma HLS INTERFACE s_axilite  port=cols 			          
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_0> imgInput(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1> imgin1(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_2> imgin2(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_3> imgin3(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_4> imgin4(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(rows, cols);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_0>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::GaussianBlur<FILTER_WIDTH_1, XF_BORDER_CONSTANT, IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_0,
                         XF_CV_DEPTH_IN_1>(imgInput, imgin1, sigma1);

    xf::cv::duplicateMat<IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_2, XF_CV_DEPTH_IN_3>(
        imgin1, imgin2, imgin3);

    xf::cv::GaussianBlur<FILTER_WIDTH_2, XF_BORDER_CONSTANT, IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_2,
                         XF_CV_DEPTH_IN_4>(imgin2, imgin4, sigma2);
    xf::cv::subtract<XF_CONVERT_POLICY_SATURATE, IN_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_IN_3, XF_CV_DEPTH_IN_4,
                     XF_CV_DEPTH_OUT_1>(imgin3, imgin4, imgOutput);

    // Convert output xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C