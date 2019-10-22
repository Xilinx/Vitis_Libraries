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

#ifndef _AU_MEDIAN_BLUR_CONFIG_
#define _AU_MEDIAN_BLUR_CONFIG_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "xf_config_params.h"
#include "imgproc/xf_median_blur.hpp"

#if NO
#define NPxPC XF_NPPC1
#else
#if GRAY
#define NPC1 XF_NPPC8
#define NPxPC XF_NPPC8
#else
#define NPC1 XF_NPPC4
#define NPxPC XF_NPPC4
#endif
#endif

#if GRAY
#define TYPE XF_8UC1
#define CHANNELS 1
#else
#define TYPE XF_8UC3
#define CHANNELS 3
#endif

void median_blur_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPxPC>& _src, xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPxPC>& _dst);

#endif // end of _AU_MEDIAN_BLUR_CONFIG_H_
