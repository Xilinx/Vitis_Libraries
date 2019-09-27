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

#ifndef _XF_CONVERT_SCALE_ABS_CONFIG_H_
#define _XF_CONVERT_SCALE_ABS_CONFIG_H_
#include "common/xf_common.hpp"
#include "hls_stream.h"
#include "imgproc/xf_convertscaleabs.hpp"
#include "xf_config_params.h"
#include <ap_int.h>

// Set the image height and width
#define HEIGHT 128
#define WIDTH 128

#if NO
#define NPC1 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#endif

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

void convertScaleAbs_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1>& imgInput1,
                           xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1>& imgOutput,
                           float scale,
                           float shift);
#endif //_XF_CONVERT_SCALE_ABS_CONFIG_H_
