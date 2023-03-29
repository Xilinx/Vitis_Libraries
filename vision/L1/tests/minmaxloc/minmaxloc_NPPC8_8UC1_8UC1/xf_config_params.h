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

#ifndef _XF_MIN_MAX_LOC_CONFIG_H_
#define _XF_MIN_MAX_LOC_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_utility.hpp"
#include "common/xf_common.hpp"
#include "core/xf_min_max_loc.hpp"

#define HEIGHT 128
#define WIDTH 128

#define XF_CV_DEPTH_IN 2

#define T_8U 1
#define T_16U 0
#define T_16S 0
#define T_32S 0

#define NPPCX XF_NPPC8

#define IN_TYPE XF_8UC1

#define INPUT_PTR_WIDTH 64

void min_max_loc_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                       int32_t& min_value,
                       int32_t& max_value,
                       uint16_t& min_loc_x,
                       uint16_t& min_loc_y,
                       uint16_t& max_loc_x,
                       uint16_t& max_loc_y,
                       int height,
                       int width);

#endif
