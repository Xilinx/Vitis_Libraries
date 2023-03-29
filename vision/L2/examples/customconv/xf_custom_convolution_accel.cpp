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

#include "xf_custom_convolution_accel_config.h"
extern "C" {

void customconv(ap_uint<INPUT_PTR_WIDTH>* img_in,
                short int* filter,
                unsigned char shift,
                ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                int rows,
                int cols) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
   
    #pragma HLS INTERFACE m_axi      port=filter        offset=slave  bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem2
   
    #pragma HLS INTERFACE s_axilite  port=shift 			          
	#pragma HLS INTERFACE s_axilite  port=rows 			          
	#pragma HLS INTERFACE s_axilite  port=cols 			          
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(rows, cols);

#pragma HLS DATAFLOW

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::filter2D<XF_BORDER_CONSTANT, FILTER_WIDTH, FILTER_HEIGHT, IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPPCX,
                     XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(imgInput, imgOutput, filter, shift);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C
