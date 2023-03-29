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

static constexpr int __XF_DEPTH = (HEIGHT * WIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);

void paintmask_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                     ap_uint<INPUT_PTR_WIDTH>* mask_in,
                     unsigned char* color,
                     ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                     int height,
                     int width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0 depth=__XF_DEPTH
    #pragma HLS INTERFACE m_axi      port=mask_in       offset=slave  bundle=gmem1 depth=__XF_DEPTH
    #pragma HLS INTERFACE m_axi      port=color   		offset=slave  bundle=gmem2 depth=1
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem3 depth=__XF_DEPTH
    #pragma HLS INTERFACE s_axilite  port=return 		      bundle=control
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> imgInput(height, width);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_MASK_IN> maskInput(height, width);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> imgOutput(height, width);

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
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(img_in, imgInput);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_MASK_IN>(mask_in, maskInput);

    // Run xfOpenCV kernel:
    xf::cv::paintmask<IN_TYPE, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_MASK_IN, XF_CV_DEPTH_OUT>(
        imgInput, maskInput, imgOutput, color_local);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(imgOutput, img_out);

    return;
} // End of kernel
