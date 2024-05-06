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

#ifndef _XF_DEMOSIACING_CONFIG_H_
#define _XF_DEMOSIACING_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_demosaicing.hpp"

#define WIDTH 128
// 7680//1920//
#define HEIGHT 128
// 4320//1080//

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

#define NPPCX XF_NPPC1

#define T_8U 0
#define T_16U 1

#define BPATTERN XF_BAYER_BG

#define XF_USE_URAM 0

#define INPUT_PTR_WIDTH 16
#define OUTPUT_PTR_WIDTH 64

#define IN_TYPE XF_16UC1
#define OUT_TYPE XF_16UC3

#define CV_IN_TYPE CV_16UC1
#define CV_OUT_TYPE CV_16UC3

#define ERROR_THRESHOLD 1

void demosaicing_accel(
    ap_uint<INPUT_PTR_WIDTH>* img_in, ap_uint<OUTPUT_PTR_WIDTH>* img_out, uint16_t bformat, int height, int width);

#endif
// _XF_DEMOSAICING_CONFIG_H_
