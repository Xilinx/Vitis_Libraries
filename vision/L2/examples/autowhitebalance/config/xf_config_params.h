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
#ifndef _XF_AWB_CONFIG_H_
#define _XF_AWB_CONFIG_H_

#include "common/xf_common.hpp"
#include "hls_stream.h"
#include "imgproc/xf_autowhitebalance.hpp"
#include "imgproc/xf_duplicateimage.hpp"
#include <ap_int.h>

/* Input image Dimensions */
#define WIDTH 3840
// Maximum Input image width
#define HEIGHT 2160
// Maximum Input image height

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_OUT_1 2

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define WB_TYPE XF_WB_GRAY
#define XF_USE_URAM 0
#define T_8U 1
#define T_16U 0
#define T_12U 0
#define T_10U 0

#if T_8U
#define HIST_SIZE 256
#endif
#if T_10U
#define HIST_SIZE 1024
#endif
#if T_16U || T_12U
#define HIST_SIZE 4096
#endif

#define CV_IN_TYPE CV_8UC3
#define CV_OUT_TYPE CV_8UC3

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64

#endif
//_XF_AWB_CONFIG_H_
