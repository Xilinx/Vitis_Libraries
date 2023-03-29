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
#ifndef _XF_CHANNEL_EXTRACT_CONFIG_H_
#define _XF_CHANNEL_EXTRACT_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_channel_extract.hpp"

// Image width and height
#define HEIGHT 2160
#define WIDTH 3840

#define XF_CV_DEPTH_IN_0 2
#define XF_CV_DEPTH_OUT_0 2

#define T_8U 0
#define T_16U 1

#define RGB 0
#define RGBA 1

#define NPPCX XF_NPPC8
#define IN_TYPE XF_16UC4
#define OUT_TYPE XF_16UC1

#define CV_IN_TYPE CV_16UC4
#define CV_OUT_TYPE CV_16UC1

#define INPUT_PTR_WIDTH 512
#define OUTPUT_PTR_WIDTH 128

#endif
//_XF_CHANNEL_EXTRACT_CONFIG_H_
