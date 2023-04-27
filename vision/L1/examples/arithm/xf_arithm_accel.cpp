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

#include "xf_arithm_accel_config.h"

static constexpr int __XF_DEPTH = (HEIGHT * WIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);

#if ARRAY
#if defined(FUNCT_BITWISENOT) || defined(FUNCT_ZERO)
void arithm_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                  ap_uint<INPUT_PTR_WIDTH>* img_in2,
                  ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                  int height,
                  int width) {
// clang-format off
#pragma HLS INTERFACE m_axi      port=img_in1       offset=slave  bundle=gmem0 depth=__XF_DEPTH
#pragma HLS INTERFACE m_axi      port=img_in2       offset=slave  bundle=gmem1 depth=__XF_DEPTH
#pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem2 depth=__XF_DEPTH
// clang-format on
#ifdef FUNCT_MULTIPLY
// clang-format off
#pragma HLS INTERFACE s_axilite  port=scale 			          bundle=control
// clang-format on
#endif
// clang-format off
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput1(height, width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(height, width);

// clang-format off

// clang-format on

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(img_in1, imgInput1);

    // Run xfOpenCV kernel:
    xf::cv::FUNCT_NAME<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(imgInput1, imgOutput);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, img_out);

    return;
} // End of kernel
#else
void arithm_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                  ap_uint<INPUT_PTR_WIDTH>* img_in2,
#ifdef FUNCT_MULTIPLY
                  float scale,
#endif
                  ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                  int height,
                  int width) {
// clang-format off
#pragma HLS INTERFACE m_axi      port=img_in1       offset=slave  bundle=gmem0 depth=__XF_DEPTH

#pragma HLS INTERFACE m_axi      port=img_in2       offset=slave  bundle=gmem1 depth=__XF_DEPTH

#pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem2 depth=__XF_DEPTH
// clang-format on
#ifdef FUNCT_MULTIPLY
// clang-format off
#pragma HLS INTERFACE s_axilite  port=scale 			          bundle=control
// clang-format on
#endif
// clang-format off
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput1(height, width);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_2> imgInput2(height, width);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(height, width);

// clang-format off

// clang-format on

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(img_in1, imgInput1);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_2>(img_in2, imgInput2);

// Run xfOpenCV kernel:
#ifdef EXTRA_PARM
    xf::cv::FUNCT_NAME<EXTRA_PARM, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_2,
                       XF_CV_DEPTH_OUT_1>(imgInput1, imgInput2, imgOutput
#ifdef FUNCT_MULTIPLY
                                          ,
                                          scale
#endif
                                          );
#else
    xf::cv::FUNCT_NAME<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_2, XF_CV_DEPTH_OUT_1>(
        imgInput1, imgInput2, imgOutput);
#endif

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, img_out);

    return;
} // End of kernel
#endif
#endif

#if SCALAR
void arithm_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                  unsigned char* scl_in,
                  ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                  int height,
                  int width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in1       offset=slave  bundle=gmem0 depth=__XF_DEPTH
    #pragma HLS INTERFACE m_axi      port=scl_in        offset=slave  bundle=gmem1 depth=3
    #pragma HLS INTERFACE m_axi      port=img_out      	 offset=slave  bundle=gmem2 depth=__XF_DEPTH
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1> imgInput1(height, width);
    // xf::cv::Scalar<XF_CHANNELS(IN_TYPE, NPPCX), unsigned char> scl;
    // unsigned char scl[XF_CHANNELS(IN_TYPE, NPPCX)];
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1> imgOutput(height, width);

// clang-format off

// clang-format on
// for (unsigned int i = 0; i < XF_CHANNELS(IN_TYPE, NPPCX); ++i) {
//     scl.val[i] = scl_in[i];
// }

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on
    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1>(img_in1, imgInput1);

    // Run xfOpenCV kernel:
    xf::cv::FUNCT_NAME<
#ifdef EXTRA_PARM
        EXTRA_PARM,
#endif
        IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_OUT_1>(imgInput1, scl_in, imgOutput);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT_1>(imgOutput, img_out);

    return;
} // End of kernel
#endif
