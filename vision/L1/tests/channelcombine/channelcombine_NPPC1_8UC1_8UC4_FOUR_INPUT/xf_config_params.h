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

#ifndef _XF_CHANNEL_COMBINE_CONFIG_H_
#define _XF_CHANNEL_COMBINE_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_channel_combine.hpp"

#define HEIGHT 128
#define WIDTH 128

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_IN_3 2
#define XF_CV_DEPTH_IN_4 2
#define XF_CV_DEPTH_OUT_1 2

#define TWO_INPUT 0
#define THREE_INPUT 0
#define FOUR_INPUT 1

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC4

#define CV_IN_TYPE CV_8UC1
#define CV_OUT_TYPE CV_8UC4

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 32

// Set the input and output pixel depth:
#if FOUR_INPUT
void channel_combine_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                           ap_uint<INPUT_PTR_WIDTH>* img_in2,
                           ap_uint<INPUT_PTR_WIDTH>* img_in3,
                           ap_uint<INPUT_PTR_WIDTH>* img_in4,
                           ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                           int height,
                           int width);
#endif

#if THREE_INPUT
void channel_combine_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                           ap_uint<INPUT_PTR_WIDTH>* img_in2,
                           ap_uint<INPUT_PTR_WIDTH>* img_in3,
                           ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                           int height,
                           int width);

#endif

#if TWO_INPUT
void channel_combine_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                           ap_uint<INPUT_PTR_WIDTH>* img_in2,
                           ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                           int height,
                           int width);

#endif

#endif
//_XF_CHANNEL_COMBINE_CONFIG_H_
