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

#include "xf_paintmask_accel_config.h"
extern "C" {

void paintmask_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                     ap_uint<MASK_PTR_WIDTH>* mask_in,
                     unsigned char* color,
                     ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                     int height,
                     int width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=mask_in       offset=slave  bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=color   	offset=slave  bundle=gmem2
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem3
    #pragma HLS INTERFACE s_axilite  port=height 		      
    #pragma HLS INTERFACE s_axilite  port=width		 	      
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput(height, width);
    xf::cv::Mat<MASK_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_2> maskInput(height, width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> imgOutput(height, width);

// clang-format off
// clang-format on

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Copy the color data to local memory:
    unsigned char color_local[XF_CHANNELS(IN_TYPE, NPPCX)];
    for (unsigned int i = 0; i < XF_CHANNELS(IN_TYPE, NPPCX); ++i) {
        color_local[i] = color[i];
    }

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(img_in, imgInput);
    xf::cv::Array2xfMat<MASK_PTR_WIDTH, MASK_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_2>(mask_in, maskInput);

    // Run xfOpenCV kernel:
    xf::cv::paintmask<IN_TYPE, MASK_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_2, XF_CV_DEPTH_OUT>(
        imgInput, maskInput, imgOutput, color_local);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C
