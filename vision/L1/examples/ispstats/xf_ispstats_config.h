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

#ifndef _XF_ISPSTATS_CONFIG_H_
#define _XF_ISPSTATS_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "xf_config_params.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_ispstats.hpp"

#define MAX_ZONES 64

#define HEIGHT 128
#define WIDTH 128

#define STATS_SIZE 256
#define FINAL_BINS_NUM 4
#define MERGE_BINS 1

#define MAX_ROWS 8
#define MAX_COLS 8

// Resolve optimization type:
#define PTR_WIDTH 32
#define NPC1 XF_NPPC1

// Set the pixel depth:
#if BAYER
#define TYPE XF_8UC1
#endif

#if BGR
#define TYPE XF_8UC3
#endif

void ispstats_accel(ap_uint<PTR_WIDTH>* img_in,
                    unsigned int* stats,
                    unsigned int* max_bins,
                    int rows,
                    int cols,
                    int roi_tlx,
                    int roi_tly,
                    int roi_width,
                    int roi_height,
                    int zone_col_num,
                    int zone_row_num);

#endif // _XF_ISPSTATS_CONFIG_H_
