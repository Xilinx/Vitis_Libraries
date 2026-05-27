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

#ifndef _XF_SOBEL_CONFIG_H_
#define _XF_SOBEL_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_sobel.hpp"

typedef unsigned int uint32_t;

//////////////  To set the parameters in Top and Test bench
//////////////////

/* config width and height */
#define WIDTH 3840
#define HEIGHT 2160

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT_0 2
#define XF_CV_DEPTH_OUT_1 2

/*  Set Filter size  */

#define FILTER_SIZE_3 0
#define FILTER_SIZE_5 1
#define FILTER_SIZE_7 0

//#define DDEPTH XF_8UC1

#if FILTER_SIZE_3
#define FILTER_WIDTH 3
#elif FILTER_SIZE_5
#define FILTER_WIDTH 5
#elif FILTER_SIZE_7
#define FILTER_WIDTH 7
#endif

#define XF_USE_URAM 1

#define GRAY 0
#define RGB 1

#define T_8U 1
#define T_16S 0

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define CV_IN_TYPE CV_8UC3
#define CV_OUT_TYPE CV_8UC3

#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 32

#define XF_INPUT_COLOR XF_RGB

void sobel_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX>& _src,
                 xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX>& _dstgx,
                 xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX>& _dstgy);
#endif
//  _XF_SOBEL_CONFIG_H_
