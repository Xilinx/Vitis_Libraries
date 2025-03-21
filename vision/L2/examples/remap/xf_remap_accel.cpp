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

#include "xf_remap_accel_config.h"
extern "C" {

void remap_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                 ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x,
                 ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y,
                 ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                 int rows,
                 int cols) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=map_x         offset=slave  bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=map_y         offset=slave  bundle=gmem2
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem3
    #pragma HLS INTERFACE s_axilite  port=rows 	
    #pragma HLS INTERFACE s_axilite  port=cols 	
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput(rows, cols);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_2> mapX(rows, cols);
    xf::cv::Mat<MAPXY_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_3> mapY(rows, cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT> imgOutput(rows, cols);

    const int HEIGHT_WIDTH_LOOPCOUNT = HEIGHT * WIDTH / XF_NPIXPERCYCLE(NPPCX);

#pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(img_in, imgInput);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_2>(map_x, mapX);
    xf::cv::Array2xfMat<MAPXY_TYPE_PTR_WIDTH, MAPXY_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_3>(map_y, mapY);

    // Run xfOpenCV kernel:
    xf::cv::remap<XF_WIN_ROWS, XF_REMAP_INTERPOLATION_TYPE, IN_TYPE, MAPXY_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPPCX,
                  XF_USE_URAM, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_2, XF_CV_DEPTH_IN_3, XF_CV_DEPTH_OUT>(
        imgInput, imgOutput, mapX, mapY);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C
