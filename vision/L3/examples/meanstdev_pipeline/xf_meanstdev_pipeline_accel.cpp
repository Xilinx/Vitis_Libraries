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

#include "xf_meanstdev_pipeline_accel_config.h"

#if YUV_420
void meanstdev_nv122rgb(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<16 * NPC2>* imgInput1,
                        unsigned short* mean,
                        unsigned short* stddev,
                        int height,
                        int width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=imgInput0   offset=slave  bundle=gmem_in0
    #pragma HLS INTERFACE m_axi      port=imgInput1   offset=slave  bundle=gmem_in1
    #pragma HLS INTERFACE m_axi      port=mean          offset=slave  bundle=gmem_out0
    #pragma HLS INTERFACE m_axi      port=stddev        offset=slave  bundle=gmem_out1

    #pragma HLS INTERFACE s_axilite  port=height 		      
    #pragma HLS INTERFACE s_axilite  port=width 		      
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_0> _imgInput0(HEIGHT, WIDTH);
    xf::cv::Mat<XF_8UC2, (HEIGHT / 2), (WIDTH / 2), NPC2, XF_CV_DEPTH_IN_1> _imgInput1((HEIGHT / 2), (WIDTH / 2));
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> _imgOutput(HEIGHT, WIDTH);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<8 * NPC1, XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN_0>(imgInput0, _imgInput0);
    xf::cv::Array2xfMat<16 * NPC2, XF_8UC2, (HEIGHT / 2), (WIDTH / 2), NPC2, XF_CV_DEPTH_IN_1>(imgInput1, _imgInput1);

    xf::cv::nv122rgb<XF_8UC1, XF_8UC2, XF_8UC3, HEIGHT, WIDTH, NPC1, NPC2, XF_CV_DEPTH_IN_0, XF_CV_DEPTH_IN_1,
                     XF_CV_DEPTH_OUT>(_imgInput0, _imgInput1, _imgOutput);

    xf::cv::meanStdDev<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(_imgOutput, mean, stddev);
}
#endif
#if YUV_422
void meanstdev_yuyv2rgb(
    ap_uint<16 * NPC1>* imgInput, unsigned short* mean, unsigned short* stddev, int height, int width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=imgInput    offset=slave  bundle=gmem_in0
    #pragma HLS INTERFACE m_axi      port=mean          offset=slave  bundle=gmem_out0
    #pragma HLS INTERFACE m_axi      port=stddev        offset=slave  bundle=gmem_out1

    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN> _imgInput(HEIGHT, WIDTH);
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> _imgOutput(HEIGHT, WIDTH);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<16 * NPC1, XF_16UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN>(imgInput, _imgInput);

    xf::cv::yuyv2rgb<XF_16UC1, XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(_imgInput, _imgOutput);

    xf::cv::meanStdDev<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_OUT>(_imgOutput, mean, stddev);
}
#endif
#if YUV_400
void meanstdev_yuv400(
    ap_uint<INPUT_PTR_WIDTH>* img_in, unsigned short* mean, unsigned short* stddev, int height, int width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=mean          offset=slave  bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=stddev        offset=slave  bundle=gmem2
    #pragma HLS INTERFACE s_axilite  port=height 		      
    #pragma HLS INTERFACE s_axilite  port=width 		      
    #pragma HLS INTERFACE s_axilite  port=return
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN> imgInput(height, width);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::meanStdDev<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(imgInput, mean, stddev);

    return;
} // End of kernel
#endif
#if YUV_444
void meanstdev_yuv4442rgb(
    ap_uint<32 * NPC1>* imgInput, unsigned short* mean, unsigned short* stddev, int height, int width) {
// clang-format off
    #pragma HLS INTERFACE m_axi      port=imgInput    offset=slave  bundle=gmem_in0
    #pragma HLS INTERFACE m_axi      port=mean          offset=slave  bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=stddev        offset=slave  bundle=gmem2
    #pragma HLS INTERFACE s_axilite  port=return
    #pragma HLS INTERFACE s_axilite  port=height 		      
    #pragma HLS INTERFACE s_axilite  port=width

    // clang-format on

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN> _imgInput(HEIGHT, WIDTH);
    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> _imgOutput(HEIGHT, WIDTH);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<32 * NPC1, XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN>(imgInput, _imgInput);

    xf::cv::yuv4442rgb<XF_8UC3, XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(_imgInput, _imgOutput);

    xf::cv::meanStdDev<OUT_TYPE, HEIGHT, WIDTH, NPPCX, XF_CV_DEPTH_IN>(_imgOutput, mean, stddev);
}
#endif
