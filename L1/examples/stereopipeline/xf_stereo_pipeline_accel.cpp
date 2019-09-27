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

#include "xf_stereo_pipeline_config.h"

void stereopipeline_accel(xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& leftMat,
                          xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& rightMat,
                          xf::cv::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& dispMat,
                          xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& mapxLMat,
                          xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& mapyLMat,
                          xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& mapxRMat,
                          xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& mapyRMat,
                          xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& leftRemappedMat,
                          xf::cv::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& rightRemappedMat,
                          xf::cv::xFSBMState<SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS>& bm_state,
                          ap_fixed<32, 12>* cameraMA_l_fix,
                          ap_fixed<32, 12>* cameraMA_r_fix,
                          ap_fixed<32, 12>* distC_l_fix,
                          ap_fixed<32, 12>* distC_r_fix,
                          ap_fixed<32, 12>* irA_l_fix,
                          ap_fixed<32, 12>* irA_r_fix,
                          int _cm_size,
                          int _dc_size) {
// clang-format off
    #pragma HLS INTERFACE m_axi depth=9 port=cameraMA_l_fix offset=direct bundle=cameraMA
    #pragma HLS INTERFACE m_axi depth=9 port=cameraMA_r_fix offset=direct bundle=cameraMA
    #pragma HLS INTERFACE m_axi depth=9 port=distC_l_fix offset=direct bundle=distC
    #pragma HLS INTERFACE m_axi depth=9 port=distC_r_fix offset=direct bundle=distC
    #pragma HLS INTERFACE m_axi depth=9 port=irA_l_fix offset=direct bundle=irA
    #pragma HLS INTERFACE m_axi depth=9 port=irA_r_fix offset=direct bundle=irA
    // clang-format on

    xf::cv::InitUndistortRectifyMapInverse<XF_CAMERA_MATRIX_SIZE, XF_DIST_COEFF_SIZE, XF_32FC1, XF_HEIGHT, XF_WIDTH,
                                           XF_NPPC1>(cameraMA_l_fix, distC_l_fix, irA_l_fix, mapxLMat, mapyLMat,
                                                     _cm_size, _dc_size);
    xf::cv::remap<XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32FC1, XF_8UC1, XF_HEIGHT, XF_WIDTH,
                  XF_NPPC1, XF_USE_URAM>(leftMat, leftRemappedMat, mapxLMat, mapyLMat);

    xf::cv::InitUndistortRectifyMapInverse<XF_CAMERA_MATRIX_SIZE, XF_DIST_COEFF_SIZE, XF_32FC1, XF_HEIGHT, XF_WIDTH,
                                           XF_NPPC1>(cameraMA_r_fix, distC_r_fix, irA_r_fix, mapxRMat, mapyRMat,
                                                     _cm_size, _dc_size);
    xf::cv::remap<XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32FC1, XF_8UC1, XF_HEIGHT, XF_WIDTH,
                  XF_NPPC1, XF_USE_URAM>(rightMat, rightRemappedMat, mapxRMat, mapyRMat);

    xf::cv::StereoBM<SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS, XF_8UC1, XF_16UC1, XF_HEIGHT, XF_WIDTH,
                     XF_NPPC1, XF_USE_URAM>(leftRemappedMat, rightRemappedMat, dispMat, bm_state);
}
