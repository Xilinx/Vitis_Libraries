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

#include "xf_stereo_pipeline_accel_config.h"

extern "C" {
void stereopipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_L,
                          ap_uint<INPUT_PTR_WIDTH>* img_R,
                          ap_uint<OUTPUT_PTR_WIDTH>* img_disp,
                          float* cameraMA_l,
                          float* cameraMA_r,
                          float* distC_l,
                          float* distC_r,
                          float* irA_l,
                          float* irA_r,
                          int* bm_state_arr,
                          int rows,
                          int cols) {
// clang-format off
    #pragma HLS INTERFACE m_axi     port=img_L  offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi     port=img_R  offset=slave bundle=gmem5
    #pragma HLS INTERFACE m_axi     port=img_disp  offset=slave bundle=gmem6
    #pragma HLS INTERFACE m_axi     port=cameraMA_l  offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi     port=cameraMA_r  offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi     port=distC_l  offset=slave bundle=gmem3
    #pragma HLS INTERFACE m_axi     port=distC_r  offset=slave bundle=gmem3
    #pragma HLS INTERFACE m_axi     port=irA_l  offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi     port=irA_r  offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi     port=bm_state_arr  offset=slave bundle=gmem4
    #pragma HLS INTERFACE s_axilite port=rows               
    #pragma HLS INTERFACE s_axilite port=cols               
    #pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    ap_fixed<32, 12> cameraMA_l_fix[XF_CAMERA_MATRIX_SIZE], cameraMA_r_fix[XF_CAMERA_MATRIX_SIZE],
        distC_l_fix[XF_DIST_COEFF_SIZE], distC_r_fix[XF_DIST_COEFF_SIZE], irA_l_fix[XF_CAMERA_MATRIX_SIZE],
        irA_r_fix[XF_CAMERA_MATRIX_SIZE];

    for (int i = 0; i < XF_CAMERA_MATRIX_SIZE; i++) {
// clang-format off
        #pragma HLS PIPELINE II=1
        // clang-format on
        cameraMA_l_fix[i] = (ap_fixed<32, 12>)cameraMA_l[i];
        cameraMA_r_fix[i] = (ap_fixed<32, 12>)cameraMA_r[i];
        irA_l_fix[i] = (ap_fixed<32, 12>)irA_l[i];
        irA_r_fix[i] = (ap_fixed<32, 12>)irA_r[i];
    }
    for (int i = 0; i < XF_DIST_COEFF_SIZE; i++) {
// clang-format off
        #pragma HLS PIPELINE II=1
        // clang-format on
        distC_l_fix[i] = (ap_fixed<32, 12>)distC_l[i];
        distC_r_fix[i] = (ap_fixed<32, 12>)distC_r[i];
    }

    xf::cv::xFSBMState<SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS> bm_state;
    bm_state.preFilterType = bm_state_arr[0];
    bm_state.preFilterSize = bm_state_arr[1];
    bm_state.preFilterCap = bm_state_arr[2];
    bm_state.SADWindowSize = bm_state_arr[3];
    bm_state.minDisparity = bm_state_arr[4];
    bm_state.numberOfDisparities = bm_state_arr[5];
    bm_state.textureThreshold = bm_state_arr[6];
    bm_state.uniquenessRatio = bm_state_arr[7];
    bm_state.ndisp_unit = bm_state_arr[8];
    bm_state.sweepFactor = bm_state_arr[9];
    bm_state.remainder = bm_state_arr[10];

    int _cm_size = 9, _dc_size = 5;

    xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAT_L> mat_L(rows, cols);

    xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAT_R> mat_R(rows, cols);

    xf::cv::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAT_DISP> mat_disp(rows, cols);

    xf::cv::Mat<XF_32SC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAP_XL> mapxLMat(rows, cols);

    xf::cv::Mat<XF_32SC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAP_YL> mapyLMat(rows, cols);

    xf::cv::Mat<XF_32SC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAP_XR> mapxRMat(rows, cols);

    xf::cv::Mat<XF_32SC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAP_YR> mapyRMat(rows, cols);

    xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_LEFT_REMAPPED> leftRemappedMat(rows, cols);

    xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_RIGHT_REMAPPED> rightRemappedMat(rows, cols);

// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAT_L>(img_L, mat_L);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAT_R>(img_R, mat_R);

    xf::cv::InitUndistortRectifyMapInverse<XF_CAMERA_MATRIX_SIZE, XF_DIST_COEFF_SIZE, XF_32SC1, XF_HEIGHT, XF_WIDTH,
                                           XF_NPPCX, XF_CV_DEPTH_MAP_XL, XF_CV_DEPTH_MAP_YL>(
        cameraMA_l_fix, distC_l_fix, irA_l_fix, mapxLMat, mapyLMat, _cm_size, _dc_size);

    xf::cv::remap<XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32SC1, XF_8UC1, XF_HEIGHT, XF_WIDTH,
                  XF_NPPCX, XF_USE_URAM, XF_CV_DEPTH_MAT_L, XF_CV_DEPTH_LEFT_REMAPPED, XF_CV_DEPTH_MAP_XL,
                  XF_CV_DEPTH_MAP_YL>(mat_L, leftRemappedMat, mapxLMat, mapyLMat);

    xf::cv::InitUndistortRectifyMapInverse<XF_CAMERA_MATRIX_SIZE, XF_DIST_COEFF_SIZE, XF_32SC1, XF_HEIGHT, XF_WIDTH,
                                           XF_NPPCX, XF_CV_DEPTH_MAP_XR, XF_CV_DEPTH_MAP_YR>(
        cameraMA_r_fix, distC_r_fix, irA_r_fix, mapxRMat, mapyRMat, _cm_size, _dc_size);
    xf::cv::remap<XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32SC1, XF_8UC1, XF_HEIGHT, XF_WIDTH,
                  XF_NPPCX, XF_USE_URAM, XF_CV_DEPTH_MAT_R, XF_CV_DEPTH_RIGHT_REMAPPED, XF_CV_DEPTH_MAP_XR,
                  XF_CV_DEPTH_MAP_YR>(mat_R, rightRemappedMat, mapxRMat, mapyRMat);

    xf::cv::StereoBM<SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS, XF_8UC1, XF_16UC1, XF_HEIGHT, XF_WIDTH,
                     XF_NPPCX, XF_USE_URAM, XF_CV_DEPTH_LEFT_REMAPPED, XF_CV_DEPTH_RIGHT_REMAPPED,
                     XF_CV_DEPTH_MAT_DISP>(leftRemappedMat, rightRemappedMat, mat_disp, bm_state);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_MAT_DISP>(mat_disp,
                                                                                                         img_disp);
}
}
