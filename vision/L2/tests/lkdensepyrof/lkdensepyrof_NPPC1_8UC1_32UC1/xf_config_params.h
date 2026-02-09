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

#ifndef __XF_PYR_DENSE_OPTICAL_FLOW_CONFIG__
#define __XF_PYR_DENSE_OPTICAL_FLOW_CONFIG__

#include "ap_int.h"
#include "hls_stream.h"
#include "assert.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "video/xf_pyr_dense_optical_flow_wrapper.hpp"
#include "imgproc/xf_pyr_down.hpp"

#define HEIGHT 2160
#define WIDTH 3840

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_IN_3 2
#define XF_CV_DEPTH_IN_4 2
#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define NPPCX XF_NPPC1

#define GRAY 1
#define RGB 0

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_32UC1

#define TYPE unsigned char

#define TYPE_FLOW_WIDTH 16
#define TYPE_FLOW_INT 10
#define TYPE_FLOW_TYPE ap_fixed<TYPE_FLOW_WIDTH, TYPE_FLOW_INT>

#define WINSIZE_OFLOW 11

#define NUM_LEVELS 5
#define NUM_ITERATIONS 5

#define NUM_LINES_FINDIT 50

#define XF_USE_URAM 0

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 32

#if GRAY
#define CH_TYPE 1
#else
#define CH_TYPE 0
#endif

#define __XF_BENCHMARK 0

#endif
