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

#ifndef _XF_CCM_CONFIG_H_
#define _XF_CCM_CONFIG_H_

#include "ap_int.h"
#include "hls_stream.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_colorcorrectionmatrix.hpp"

/* Input image Dimensions */
#define WIDTH 1024
// Maximum Input image width
#define HEIGHT 676
// Maximum Input image height

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

#define NPPCX XF_NPPC8

#define XF_CCM_TYPE XF_CCM_yuv_rgb_2020

#define T_8U 0
#define T_16U 1

#define IN_TYPE XF_16UC3
#define OUT_TYPE XF_16UC3

#if T_8U
#define SIN_CHANNEL_TYPE XF_8UC1
#endif
#if T_16U
#define SIN_CHANNEL_TYPE XF_16UC1
#endif

#define SIN_CHANNEL_TYPE XF_8UC1

#define CV_OUT_TYPE CV_16UC3

#define INPUT_PTR_WIDTH 512
#define OUTPUT_PTR_WIDTH 512

void ccm_accel(ap_uint<INPUT_PTR_WIDTH>* src,
               ap_uint<OUTPUT_PTR_WIDTH>* dst,
               signed int ccm_config_1[3][3],
               signed int ccm_config_2[3],
               int rows,
               int cols);

#endif
