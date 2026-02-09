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
#ifndef _XF_RESIZE_PIPELINE_CONFIG_H_
#define _XF_RESIZE_PIPELINE_CONFIG_H_

#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "hls_stream.h"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
#include "imgproc/xf_resize.hpp"

#define MEMORYMAPPED_ARCH 0

/* Input image Dimensions */
#define WIDTH 3840
// Maximum Input image width
#define HEIGHT 2160
// Maximum Input image height

/* Output image Dimensions */
#define NEWWIDTH 1920
// Maximum output image width
#define NEWHEIGHT 1080
// Maximum output image height

#define MAXDOWNSCALE 2

/* Interpolation type*/
#define INTERPOLATION 1
// 0 - Nearest Neighbor Interpolation
// 1 - Bilinear Interpolation
// 2 - AREA Interpolation

#define XF_USE_URAM 1

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define XF_CV_DEPTH_IN_0 2
#define XF_CV_DEPTH_OUT_0 2

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_OUT_2 2

/*  set the optimisation type  */
#define SPC 0
// Single Pixel per Clock operation
#define MPC 1
// Multiple Pixels per Clock operation

// input types supported
#define YUV_420 0
#define YUV_422 1
#define YUV_400 0
#define YUV_444 0

#if YUV_420
#if SPC
static constexpr int NPC1 = XF_NPPC1;
static constexpr int NPC2 = XF_NPPC1;
#endif
#if MPC
static constexpr int NPC1 = XF_NPPC8;
static constexpr int NPC2 = XF_NPPC4;
#endif
#else
#if SPC
static constexpr int NPC1 = XF_NPPC1;
static constexpr int NPC2 = XF_NPPC1;
#endif
#if MPC
static constexpr int NPC1 = XF_NPPC8;
static constexpr int NPC2 = XF_NPPC8;
#endif
#endif

#define NPPCX XF_NPPC8

// Set the input and output pixel depth:
#if YUV_400
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64
#else
#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define INPUT_PTR_WIDTH 256
#define OUTPUT_PTR_WIDTH 256
#endif

#endif
