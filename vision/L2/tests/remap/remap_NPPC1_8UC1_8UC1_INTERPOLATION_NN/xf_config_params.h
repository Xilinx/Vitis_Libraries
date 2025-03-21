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

#ifndef _XF_REMAP_CONFIG_H_
#define _XF_REMAP_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_remap.hpp"

#define HEIGHT 1080
#define WIDTH 1920

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_IN_3 2
#define XF_CV_DEPTH_OUT 2

// The type of interpolation, define INTERPOLATION as 0 for Nearest Neighbour
// or 1 for Bilinear
#define INTERPOLATION 0

// Resolve interpolation type:
#if INTERPOLATION == 0
#define XF_REMAP_INTERPOLATION_TYPE XF_INTERPOLATION_NN
#else
#define XF_REMAP_INTERPOLATION_TYPE XF_INTERPOLATION_BILINEAR
#endif

// Mat types
#define MAPXY_TYPE XF_32SC1
// Configure this based on the number of rows needed for the remap purpose
// e.g., If its a right to left flip two rows are enough
#define XF_WIN_ROWS 8
#define BARREL 0
#define XF_USE_URAM 0

#define GRAY 1
#define RGB 0

// Set the optimization type:
//  XF_NPPC1,XF_NPPC2 are available for this algorithm currently
#define NPPCX XF_NPPC1
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#if GRAY
#define CHANNELS 1
#else
// RGB
#define CHANNELS 3
#endif

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 8
#define MAPXY_TYPE_PTR_WIDTH 32

#endif