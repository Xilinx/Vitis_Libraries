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

#ifndef _XF_MEAN_STDDEV_CONFIG_H_
#define _XF_MEAN_STDDEV_CONFIG_H_
#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "core/xf_mean_stddev.hpp"

#define HEIGHT 128
#define WIDTH 128
#define XF_CV_DEPTH_IN 2

// Resolve optimization type:

#define GRAY 1
#define RGB 0

#define NPPCX XF_NPPC8
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define INPUT_PTR_WIDTH 64

void mean_stddev_accel(
    ap_uint<INPUT_PTR_WIDTH>* img_in, unsigned short* mean, unsigned short* stddev, int height, int width);

#endif
// _XF_MEAN_STDDEV_CONFIG_H_
