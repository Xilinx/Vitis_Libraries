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

#ifndef _XF_STEREOBM_CONFIG_H_
#define _XF_STEREOBM_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_stereolbm.hpp"

/* config width and height */
#define WIDTH 1280
#define HEIGHT 720

#define XF_CV_DEPTH_IN_L 2
#define XF_CV_DEPTH_IN_R 2
#define XF_CV_DEPTH_OUT 2

#define SAD_WINDOW_SIZE 5

/* NO_OF_DISPARITIES must be greater than '0' and less than the image width */
#define NO_OF_DISPARITIES 16

/* NO_OF_DISPARITIES must not be lesser than PARALLEL_UNITS and NO_OF_DISPARITIES/PARALLEL_UNITS must be a
 * non-fractional number */
#define PARALLEL_UNITS 2

#define XF_USE_URAM 0

// Set the input and output pixel depth:
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_16UC1

// Set the optimization type:
#define NPPCX XF_NPPC1

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 16

void stereolbm_accel(ap_uint<INPUT_PTR_WIDTH>* img_in_l,
                     ap_uint<INPUT_PTR_WIDTH>* img_in_r,
                     unsigned char* bm_state_in,
                     ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                     int rows,
                     int cols);

#endif
// _XF_STEREOBM_CONFIG_H_