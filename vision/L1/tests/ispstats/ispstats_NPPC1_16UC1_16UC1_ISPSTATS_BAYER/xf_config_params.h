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

#define HEIGHT 1080
#define WIDTH 1920

#define XF_CV_DEPTH_IN 2

#define INPUT_PTR_WIDTH 16
#define NPPCX XF_NPPC1

#define BGR 0
#define BAYER 1
#define GRAY 0

#define T_8U 0
#define T_16U 1

// Resolve input and output pixel type:
#define IN_TYPE XF_16UC1
#define NUM_OUT_CH 3

#define MAX_ZONES 64

#define STATS_SIZE 4096

#define FINAL_BINS_NUM 4
#define MERGE_BINS 0

#define MAX_ROWS 8
#define MAX_COLS 8

void ispstats_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                    unsigned int* stats,
                    ap_uint<13>* max_bins,
                    int rows,
                    int cols,
                    int roi_tlx,
                    int roi_tly,
                    int roi_brx,
                    int roi_bry,
                    int zone_col_num, // N
                    int zone_row_num, // M
                    float inputMin,
                    float inputMax,
                    float outputMin,
                    float outputMax);

#endif
// _XF_ISPSTATS_CONFIG_H_
