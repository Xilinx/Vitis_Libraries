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
#ifndef _XF_LETTERBOX_CONFIG_
#define _XF_LETTERBOX_CONFIG_

#include <hls_stream.h>
#include <ap_int.h>
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "xf_config_params.h"
#include "imgproc/xf_resize.hpp"
#include "dnn/xf_insertBorder.hpp"

#define XF_CV_DEPTH_IN_0 2
#define XF_CV_DEPTH_OUT_1 2
#define XF_CV_DEPTH_OUT_2 2

/* Parameter for Resize kernel */
#define WIDTH 1920    // Maximum Input image width
#define HEIGHT 1080   // Maximum Input image height
#define NEWWIDTH 720  // Maximum output image width
#define NEWHEIGHT 720 // Maximum output image height

#define MAXDOWNSCALE 4
#define RGB 1
#define GRAY 0
/* Interpolation type*/
#define INTERPOLATION 1

#define XF_NPPCX XF_NPPC4

#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define XF_USE_URAM 0

/* Input/output port width */
#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128

#endif
