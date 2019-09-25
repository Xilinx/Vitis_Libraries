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

#ifndef _XF_MEAN_STDDEV_CONFIG_H_
#define _XF_MEAN_STDDEV_CONFIG_H_
#include "hls_stream.h"
#include "ap_int.h"
#include "xf_config_params.h"
#include "common/xf_common.hpp"
#include "core/xf_mean_stddev.hpp"

#if NO
#define _NPPC XF_NPPC1
#endif

#if RO
#define _NPPC XF_NPPC8
#endif

/* config width and height */
#define WIDTH 128
#define HEIGHT 128

#if GRAY
#define TYPE XF_8UC1
#else
#define TYPE XF_8UC3
#endif

void mean_stddev_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, _NPPC>& imgInput,
                       unsigned short _mean[XF_CHANNELS(TYPE, _NPPC)],
                       unsigned short _stddev[XF_CHANNELS(TYPE, _NPPC)]);

#endif // _XF_MEAN_STDDEV_CONFIG_H_
