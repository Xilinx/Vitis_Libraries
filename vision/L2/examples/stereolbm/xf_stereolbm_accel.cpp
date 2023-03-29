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

#include "xf_stereolbm_accel_config.h"
extern "C" {

void stereolbm_accel(ap_uint<INPUT_PTR_WIDTH>* img_in_l,
                     ap_uint<INPUT_PTR_WIDTH>* img_in_r,
                     unsigned char* bm_state_in,
                     ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                     int rows,
                     int cols) {
// clang-format off
	#pragma HLS INTERFACE m_axi      port=img_in_l      offset=slave  bundle=gmem0
	#pragma HLS INTERFACE m_axi      port=img_in_r      offset=slave  bundle=gmem1
	#pragma HLS INTERFACE m_axi      port=bm_state_in   offset=slave  bundle=gmem2
	#pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem3
	#pragma HLS INTERFACE s_axilite  port=rows		
	#pragma HLS INTERFACE s_axilite  port=cols		
	#pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_0> imgInputL(rows, cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInputR(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> imgOutput(rows, cols);
    xf::cv::xFSBMState<SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS> bmState;

    // Initialize SBM State:
    bmState.preFilterCap = bm_state_in[0];
    bmState.uniquenessRatio = bm_state_in[1];
    bmState.textureThreshold = bm_state_in[2];
    bmState.minDisparity = bm_state_in[3];

// clang-format off
// clang-format on

// clang-format off
	#pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_0>(img_in_l, imgInputL);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(img_in_r, imgInputR);

    // Run xfOpenCV kernel:
    xf::cv::StereoBM<SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS, IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPPCX,
                     XF_USE_URAM, XF_CV_DEPTH_IN_0, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT>(imgInputL, imgInputR, imgOutput,
                                                                                       bmState);

    // Convert _dst xf::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C
