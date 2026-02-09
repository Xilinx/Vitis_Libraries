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

#ifndef _XF_QUANTIZATIONDITHERING_CONFIG_
#define _XF_QUANTIZATIONDITHERING_CONFIG_

#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "hls_stream.h"
#include "imgproc/xf_quantizationdithering.hpp"

#define WIDTH 1024
// Maximum Input image width
#define HEIGHT 676
// Maximum Input image height

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

/* SCALEFACTOR & MAXREPRESENTEDVALUE should power of 2 */
#define SCALEFACTOR 256
#define MAXREPRESENTEDVALUE 65536

#define NPPCX XF_NPPC1
#define XF_USE_URAM 0
#define IN_TYPE XF_16UC3
#define OUT_TYPE XF_8UC3

#define RGB 1
#define GRAY 0

#define T_16U 1
#define T_8U 0

#define CV_IN_TYPE CV_16UC3
#define CV_OUT_TYPE CV_8UC3

// port widths
#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128

#if GRAY
#define CH_TYPE XF_GRAY
#else
#define CH_TYPE XF_RGB
#endif

#endif
