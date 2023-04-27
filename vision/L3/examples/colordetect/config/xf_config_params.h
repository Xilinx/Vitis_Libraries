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
#ifndef _XF_COLORDETECT_CONFIG_H_
#define _XF_COLORDETECT_CONFIG_H_

#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "hls_stream.h"
#include "imgproc/xf_bgr2hsv.hpp"
#include "imgproc/xf_channel_combine.hpp"
#include "imgproc/xf_colorthresholding.hpp"
#include "imgproc/xf_dilation.hpp"
#include "imgproc/xf_erosion.hpp"

//#define MAXCOLORS 3

/* Set the image height and width */
#define HEIGHT 2160
#define WIDTH 3840

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_RGB2HSV 2
#define XF_CV_DEPTH_HELP_1 2
#define XF_CV_DEPTH_HELP_2 2
#define XF_CV_DEPTH_HELP_3 2
#define XF_CV_DEPTH_HELP_4 2
#define XF_CV_DEPTH_OUT_1 2

/* Color thresholding parameters */
#define MAXCOLORS 3

/* Erode and Dilate parameters */
#define FILTER_SIZE 3
#define KERNEL_SHAPE 0 // 0 - rectangle, 1 - ellipse, 2 - cross
#define ITERATIONS 1

// Set the optimization type:
#define XF_NPPCX XF_NPPC1

// Set the input and output pixel depth:
#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC1
#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 8

// Resolve mask shape:
#if KERNEL_SHAPE == 0
#define XF_KERNEL_SHAPE XF_SHAPE_RECT
#elif KERNEL_SHAPE == 1
#define XF_KERNEL_SHAPE XF_SHAPE_ELLIPSE
#elif KERNEL_SHAPE == 2
#define XF_KERNEL_SHAPE XF_SHAPE_CROSS
#else
#define XF_KERNEL_SHAPE XF_SHAPE_RECT
#endif

#endif
