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

#ifndef _XF_LENSSHDING_CONFIG_H_
#define _XF_LENSSHDING_CONFIG_H_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_lensshadingcorrection.hpp"

/* Input image Dimensions */
#define WIDTH 1024
// Maximum Input image width
#define HEIGHT 676
// Maximum Input image height

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define T_8U 0
#define T_16U 1

#define BPATTERN XF_BAYER_RG

#if T_8U
#define pxltype unsigned char
#else
#define pxltype unsigned short
#endif

#define NPPCX XF_NPPC8

#define IN_TYPE XF_16UC1
#define OUT_TYPE XF_16UC1

#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128

#endif