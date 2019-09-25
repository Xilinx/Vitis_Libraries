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

#ifndef _XF_BILATERAL_FILTER_CONFIG_H_
#define _XF_BILATERAL_FILTER_CONFIG_H_
#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "xf_config_params.h"
#include "imgproc/xf_bilateral_filter.hpp"

typedef unsigned short int uint16_t;

#define ERROR_THRESHOLD 0 // acceptable error threshold range 0 to 255

#if RO
#define IN_TYPE ap_uint<64>
#define OUT_TYPE IN_TYPE
#define NPC1 XF_NPPC8
#endif
#if NO
#define IN_TYPE ap_uint<8>
#define OUT_TYPE IN_TYPE
#define NPC1 XF_NPPC1
#endif

#if FILTER_SIZE_3
#define FILTER_WIDTH 3
#elif FILTER_SIZE_5
#define FILTER_WIDTH 5
#elif FILTER_SIZE_7
#define FILTER_WIDTH 7
#endif

#if GRAY
#define TYPE XF_8UC1
#else
#define TYPE XF_8UC3
#endif

void bilateral_filter_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& _src,
                            xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& _dst,
                            float sigma_color,
                            float sigma_space);
#endif //_AU_BILATERAL_FILTER_CONFIG_H_
