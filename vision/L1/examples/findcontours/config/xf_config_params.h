/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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

#ifndef _XF_FINDCONTOURS_CONFIG_H_
#define _XF_FINDCONTOURS_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_findcontours.hpp"

#define MAX_W 1200 
#define MAX_H 800
#define MAX_CONTOURS 4096
#define MAX_TOTAL_POINTS 10000

#define XF_CV_DEPTH_IN (MAX_W * MAX_H)
#define XF_CV_DEPTH_OUT_1 MAX_TOTAL_POINTS
#define XF_CV_DEPTH_OUT_2 MAX_CONTOURS + 1
#define XF_CV_DEPTH_IN_MAT 2

#define NPPCX XF_NPPC1
// Set the pixel depth:
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_32UC1

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 32

void findcontours_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                        int imgwidth,
                        int imgheight,
                        ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                        ap_uint<OUTPUT_PTR_WIDTH>* img_out1,
                        ap_uint<OUTPUT_PTR_WIDTH>* img_out2);

#endif
