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
#ifndef _XF_HARRIS_CONFIG_H_
#define _XF_HARRIS_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "features/xf_harris.hpp"

#define WIDTH 3840
#define HEIGHT 2160

// Set the function for reference
#define __XF_BENCHMARK 1

#define IMGSIZE WIDTH* HEIGHT
#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2
#define MAXCORNERS 1024

#define FILTER_WIDTH 3
#define BLOCK_WIDTH 5
#define NMS_RADIUS 2
#define XF_USE_URAM 0

#define NPPCX XF_NPPC8

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define CV_IN_TYPE CV_8UC1
#define CV_OUT_TYPE CV_8UC1

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64

#endif
