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

#include "xf_threshold_config.h"

extern "C" {
void gaussian_otsu_accel(ap_uint<GAUSSIAN_INPUT_PTR_WIDTH>* img_inp,
                         ap_uint<GAUSSIAN_OUTPUT_PTR_WIDTH>* img_out,
                         int rows,
                         int cols,
                         float sigma,
                         unsigned char* Otsuval,
                         ap_uint<8>* array_params) {
// clang-format off
    #pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem2
	#pragma HLS INTERFACE m_axi     port=Otsuval  offset=slave bundle=gmem3
	#pragma HLS INTERFACE m_axi     port=array_params depth=12
    #pragma HLS INTERFACE s_axilite port=sigma     
    #pragma HLS INTERFACE s_axilite port=rows     
    #pragma HLS INTERFACE s_axilite port=cols     
    #pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN> in_mat(rows, cols);
    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> out_bgr2y8_mat(rows, cols);

    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> out_mat(rows, cols);

    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT> out_mat_otsu(rows, cols);
    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT_1> out_mat_ret(rows, cols);

    struct bgr2y8_params pxl_val;

    pxl_val.black_Vmax = array_params[0];
    pxl_val.black_Smax = array_params[1];
    pxl_val.brown_Hmax = array_params[2];
    pxl_val.brown_Vmax = array_params[3];
    pxl_val.Smin = array_params[4];
    pxl_val.Smax = array_params[5];
    pxl_val.darkgreen_Vmax = array_params[6];
    pxl_val.darkgreen_Hmin = array_params[7];
    pxl_val.darkgreen_Hmax = array_params[8];
    pxl_val.green_Hmax = array_params[9];
    pxl_val.green_Hmin = array_params[10];
    pxl_val.green_Vmax = array_params[11];

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<GAUSSIAN_INPUT_PTR_WIDTH, XF_8UC3, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN>(img_inp, in_mat);

    xf::cv::custom_bgr2y8<XF_8UC3, XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        in_mat, out_bgr2y8_mat, pxl_val);

    xf::cv::GaussianBlur<FILTER_WIDTH, XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN,
                         XF_CV_DEPTH_OUT>(out_bgr2y8_mat, out_mat, sigma);

    xf::cv::duplicateMat<XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT, XF_CV_DEPTH_OUT_1>(
        out_mat, out_mat_otsu, out_mat_ret);

    xf::cv::OtsuThreshold<OTSU_PIXEL_TYPE, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT>(out_mat_otsu, *Otsuval);

    xf::cv::xfMat2Array<GAUSSIAN_OUTPUT_PTR_WIDTH, XF_8UC1, HEIGHT, WIDTH, NPC1, XF_CV_DEPTH_OUT_1>(out_mat_ret,
                                                                                                    img_out);
}
}
