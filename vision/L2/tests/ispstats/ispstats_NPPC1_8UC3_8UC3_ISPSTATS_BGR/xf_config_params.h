/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef _XF_ISPSTATS_CONFIG_H_
#define _XF_ISPSTATS_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_ispstats.hpp"

#define HEIGHT 128
#define WIDTH 128

#define XF_CV_DEPTH_IN 2

#define INPUT_PTR_WIDTH 32
#define NPPCX XF_NPPC1

#define BGR 1
#define BAYER 0
#define GRAY 0

#define T_8U 1
#define T_16U 0

// Resolve input and output pixel type:
#define IN_TYPE XF_8UC3
#define NUM_OUT_CH 3

#define MAX_ZONES 64

#define STATS_SIZE 256

#define FINAL_BINS_NUM 4
#define MERGE_BINS 0

#define MAX_ROWS 8
#define MAX_COLS 8

#endif
// _XF_ISPSTATS_CONFIG_H_
