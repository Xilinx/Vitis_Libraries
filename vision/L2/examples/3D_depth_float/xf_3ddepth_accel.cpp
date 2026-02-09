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

#include "xf_3ddepth_accel_config.h"

static constexpr int __XF_DEPTH_IN = (HEIGHT * WIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_OUT = (HEIGHT * WIDTH * (XF_PIXELWIDTH(OUT_TYPE, NPPCX)) / 8) / (OUTPUT_PTR_WIDTH / 8);

void depth3d_accel(ap_uint<INPUT_PTR_WIDTH>* disp,
                   ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                   float focal_len,
                   float base_dis,
                   uint16_t height,
                   uint16_t width) {
// clang-format off
	#pragma HLS INTERFACE m_axi      port=disp         offset=slave  bundle=gmem0
	#pragma HLS INTERFACE m_axi      port=img_out      offset=slave  bundle=gmem1

	#pragma HLS INTERFACE s_axilite  port=focal_len	
	#pragma HLS INTERFACE s_axilite  port=base_dis	
	#pragma HLS INTERFACE s_axilite  port=height	
	#pragma HLS INTERFACE s_axilite  port=width	
	#pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_L> disp_m(height, width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> imgOutput(height, width);

// clang-format off
	#pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_L>(disp, disp_m);

    // Run xfOpenCV kernel:
    xf::cv::depth3D<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_L, XF_CV_DEPTH_OUT>(disp_m, imgOutput,
                                                                                                focal_len, base_dis);

    // Convert _dst xf::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(imgOutput, img_out);

    return;
} // End of kernel
