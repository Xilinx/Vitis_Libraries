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

#ifndef _XF_REDUCE_CONFIG_H_
#define _XF_REDUCE_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_reduce.hpp"

#define HEIGHT 1080
#define WIDTH 1920

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_32SC1

#define CV_IN_TYPE CV_8UC1
#define CV_OUT_TYPE CV_32SC1

#define DIM 0

#define XF_REDUCE_SUM 0
#define XF_REDUCE_AVG 1
#define XF_REDUCE_MAX 2
#define XF_REDUCE_MIN 3

// Set the output image size:
#if DIM
#define ONE_D_HEIGHT 1080
#define ONE_D_WIDTH 1
#else
#define ONE_D_HEIGHT 1
#define ONE_D_WIDTH 1920
#endif

#define REDUCTION_OP XF_REDUCE_SUM

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 32

#endif
// end of _XF_REDUCE_CONFIG_H_
