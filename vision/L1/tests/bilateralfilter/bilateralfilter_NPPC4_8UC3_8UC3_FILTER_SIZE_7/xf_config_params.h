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

#ifndef _XF_BILATERAL_FILTER_CONFIG_H_
#define _XF_BILATERAL_FILTER_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_bilateral_filter.hpp"
#define WIDTH 128
#define HEIGHT 128

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

typedef unsigned short int uint16_t;

#define ERROR_THRESHOLD 0
// acceptable error threshold range 0 to 255

// Resolve optimization type:

#define FILTER_SIZE_3 0
#define FILTER_SIZE_5 0
#define FILTER_SIZE_7 1

#if FILTER_SIZE_3
#define FILTER_WIDTH 3
#elif FILTER_SIZE_5
#define FILTER_WIDTH 5
#elif FILTER_SIZE_7
#define FILTER_WIDTH 7
#endif

#define GRAY 0
#define RGB 1

#define NPPCX XF_NPPC4

#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define CV_IN_TYPE CV_8UC3
#define CV_OUT_TYPE CV_8UC3

#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128

void bilateral_filter_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                            float sigma_color,
                            float sigma_space,
                            int rows,
                            int cols,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_out);

#endif
//_XF_BILATERAL_FILTER_CONFIG_H_
