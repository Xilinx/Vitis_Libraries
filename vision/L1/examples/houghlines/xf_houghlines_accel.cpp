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

#include "xf_houghlines_accel_config.h"

static constexpr int __XF_DEPTH = (HEIGHT * WIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_Y = LINESMAX;
static constexpr int __XF_DEPTH_X = LINESMAX;

void houghlines_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                      short threshold,
                      short maxlines,
                      float* arrayy,
                      float* arrayx,
                      int rows,
                      int cols) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in    offset=slave  bundle=gmem0 depth=__XF_DEPTH
   
    #pragma HLS INTERFACE m_axi      port=arrayy    offset=slave  bundle=gmem1 depth=__XF_DEPTH_Y
    #pragma HLS INTERFACE s_axilite  port=arrayy 			      
    #pragma HLS INTERFACE m_axi      port=arrayx    offset=slave  bundle=gmem2 depth=__XF_DEPTH_X
    #pragma HLS INTERFACE s_axilite  port=arrayx 			      
    #pragma HLS INTERFACE s_axilite  port=threshold 			  
    #pragma HLS INTERFACE s_axilite  port=maxlines 			      
	 #pragma HLS INTERFACE s_axilite port=rows     
    #pragma HLS INTERFACE s_axilite port=cols     
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput(rows, cols);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::HoughLines<RHOSTEP, THETASTEP, LINESMAX, DIAGVAL, MINTHETA, MAXTHETA, IN_TYPE, HEIGHT, WIDTH, NPPCX,
                       XF_CV_DEPTH_IN_1>(imgInput, arrayy, arrayx, threshold, maxlines);

    return;
} // End of kernel
