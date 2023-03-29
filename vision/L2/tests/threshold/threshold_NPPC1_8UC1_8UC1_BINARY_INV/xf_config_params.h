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

#define HEIGHT 2160
#define WIDTH 3840

typedef ap_uint<8> ap_uint8_t;
typedef ap_uint<64> ap_uint64_t;

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define THRESH_TYPE XF_THRESHOLD_TYPE_BINARY_INV

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 8

void threshold_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX>& _src,
                     xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX>& _dst,
                     unsigned char thresh,
                     unsigned char maxval);

#endif
// end of _XF_THRESHOLD_CONFIG_H_