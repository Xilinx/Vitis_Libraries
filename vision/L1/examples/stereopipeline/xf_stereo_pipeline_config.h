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

#ifndef _XF_STEREO_PIPELINE_CONFIG_H_
#define _XF_STEREO_PIPELINE_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_stereo_pipeline.hpp"
#include "imgproc/xf_remap.hpp"
#include "imgproc/xf_stereolbm.hpp"
#include "xf_config_params.h"

/* config width and height */
#define XF_HEIGHT 720
#define XF_WIDTH 1280

#define XF_CAMERA_MATRIX_SIZE 9
#define XF_DIST_COEFF_SIZE 5

#define IN_TYPE ap_uint<8>
#define OUT_TYPE ap_uint<16>

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
                          int _dc_size);

#endif // _XF_STEREO_PIPELINE_CONFIG_H_
