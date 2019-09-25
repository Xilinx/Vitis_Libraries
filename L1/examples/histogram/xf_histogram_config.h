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

#ifndef _XF_HISTOGRAM_CONFIG_H_
#define _XF_HISTOGRAM_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "xf_config_params.h"
#include "common/xf_common.hpp"
#include "imgproc/xf_histogram.hpp"

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;

/* config width and height */
#define WIDTH 128
#define HEIGHT 128

#if NO
#define _NPPC XF_NPPC1
#endif

#if RO
#define _NPPC XF_NPPC8
#endif

#if GRAY
#define TYPE XF_8UC1
#else
#define TYPE XF_8UC3
#endif

void histogram_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, _NPPC>& imgInput, unsigned int* histogram);

#endif // _XF_HISTOGRAM_CONFIG_H_
