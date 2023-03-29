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

#ifndef _XF_EROSION_CONFIG_H_
#define _XF_EROSION_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_erosion.hpp"

/* config width and height */
#define WIDTH 128
#define HEIGHT 128

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2
#define FILTER_SIZE 3

// Resolve optimization type:

#define KERNEL_SHAPE XF_SHAPE_RECT

#define ITERATIONS 2
#define GRAY 0
#define RGB 1

#define NPPCX XF_NPPC8

#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define CV_IN_TYPE CV_8UC3
#define CV_OUT_TYPE CV_8UC3

#define INPUT_PTR_WIDTH 256
#define OUTPUT_PTR_WIDTH 256

void erosion_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                   unsigned char* process_shape,
                   ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                   int height,
                   int width);
#endif
// _XF_EROSION_CONFIG_H_
