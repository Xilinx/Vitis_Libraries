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

#ifndef __XF_DENSE_NONPYR_OPTICAL_FLOW_CONFIG__
#define __XF_DENSE_NONPYR_OPTICAL_FLOW_CONFIG__

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "video/xf_dense_npyr_optical_flow.hpp"

#define HEIGHT 512
#define WIDTH 512

#define XF_CV_DEPTH_IN_CURR 2
#define XF_CV_DEPTH_IN_PREV 2
#define XF_CV_DEPTH_OUTX 2
#define XF_CV_DEPTH_OUTY 2

#define MAX_HEIGHT 512
#define MAX_WIDTH 512
#define KMED 25

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_32FC1

#define XF_USE_URAM false

#define OUT_BYTES_PER_CHANNEL 4

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 32

void dense_non_pyr_of_accel(ap_uint<INPUT_PTR_WIDTH>* img_curr,
                            ap_uint<INPUT_PTR_WIDTH>* img_prev,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_outx,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_outy,
                            int rows,
                            int cols);

#endif
// _XF_DILATION_CONFIG_H_
