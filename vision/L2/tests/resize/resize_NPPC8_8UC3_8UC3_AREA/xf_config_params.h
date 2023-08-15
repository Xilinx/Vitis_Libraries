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

#ifndef _XF_RESIZE_CONFIG_
#define _XF_RESIZE_CONFIG_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "imgproc/xf_resize.hpp"

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

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define NPPCX XF_NPPC8

#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define CV_IN_TYPE CV_8UC3
#define CV_OUT_TYPE CV_8UC3

// For Nearest Neighbor & Bilinear Interpolation, max down scale factor 2 for all 1-pixel modes, and for upscale in x
// direction
#define MAXDOWNSCALE 2

#define RGB 1
#define GRAY 0

/* Interpolation type*/
#define INTERPOLATION 2
// 0 - Nearest Neighbor Interpolation
// 1 - Bilinear Interpolation
// 2 - AREA Interpolation

#define XF_USE_URAM 1

// port widths
#define INPUT_PTR_WIDTH 256
#define OUTPUT_PTR_WIDTH 256

#if GRAY
#define CH_TYPE 0
#else
#define CH_TYPE 1
#endif

#endif
