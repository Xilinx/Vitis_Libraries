/*
 * Copyright 2019 Xilinx, Inc.
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

#include "xf_channel_combine_config.h"

extern "C" {

void channel_combine(ap_uint<PTR_IN_WIDTH>* img_in1,
                     ap_uint<PTR_IN_WIDTH>* img_in2,
#if !TWO_INPUT
                     ap_uint<PTR_IN_WIDTH>* img_in3,
#endif
#if FOUR_INPUT
                     ap_uint<PTR_IN_WIDTH>* img_in4,
#endif
                     ap_uint<PTR_OUT_WIDTH>* img_out,
                     int height,
                     int width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in1       offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=img_in2       offset=slave  bundle=gmem1
#if !TWO_INPUT
    #pragma HLS INTERFACE m_axi      port=img_in3       offset=slave  bundle=gmem2
#endif
#if FOUR_INPUT
    #pragma HLS INTERFACE m_axi      port=img_in4       offset=slave  bundle=gmem3
#endif
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem4
    #pragma HLS interface s_axilite  port=height	              
    #pragma HLS interface s_axilite  port=width 	              
    #pragma HLS interface s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_1> imgInput1(height, width);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_2> imgInput2(height, width);
#if !TWO_INPUT
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_3> imgInput3(height, width);
#if FOUR_INPUT
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_4> imgInput4(height, width);
#endif
#endif

    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> imgOutput(height, width);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_IN_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_1>(img_in1, imgInput1);
    xf::cv::Array2xfMat<PTR_IN_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_2>(img_in2, imgInput2);
#if !TWO_INPUT
    xf::cv::Array2xfMat<PTR_IN_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_3>(img_in3, imgInput3);
#if FOUR_INPUT
    xf::cv::Array2xfMat<PTR_IN_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_4>(img_in4, imgInput4);
#endif
#endif

// Run xfOpenCV kernel:
#if TWO_INPUT
    xf::cv::merge<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_2, XF_CV_DEPTH_OUT>(
        imgInput1, imgInput2, imgOutput);
#else
#if !FOUR_INPUT
    xf::cv::merge<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_2, XF_CV_DEPTH_IN_3,
                  XF_CV_DEPTH_OUT>(imgInput1, imgInput2, imgInput3, imgOutput);
#endif

#if FOUR_INPUT
    xf::cv::merge<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_1, XF_CV_DEPTH_IN_2, XF_CV_DEPTH_IN_3,
                  XF_CV_DEPTH_IN_4, XF_CV_DEPTH_OUT>(imgInput1, imgInput2, imgInput3, imgInput4, imgOutput);
#endif
#endif

    // Convert imgOutput xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_OUT_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT>(imgOutput, img_out);

    return;

} // End of kernel

} // End of extern C
