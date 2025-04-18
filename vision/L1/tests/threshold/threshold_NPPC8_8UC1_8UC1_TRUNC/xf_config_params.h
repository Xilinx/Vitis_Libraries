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

#ifndef _XF_THRESHOLD_CONFIG_H_
#define _XF_THRESHOLD_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_threshold.hpp"

#define HEIGHT 128
#define WIDTH 128

typedef ap_uint<8> ap_uint8_t;
typedef ap_uint<64> ap_uint64_t;

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define NPPCX XF_NPPC8

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define THRESH_TYPE XF_THRESHOLD_TYPE_TRUNC

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64

void threshold_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                     ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                     unsigned char thresh,
                     unsigned char maxval,
                     int rows,
                     int cols);

#endif
// end of _XF_THRESHOLD_CONFIG_H_