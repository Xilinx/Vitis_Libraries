/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef __XF_MASKGEN_UTIL_H_
#define __XF_MASKGEN_UTIL_H_

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

// ########### SCALAR COMPUTATION FOR TRACKING AND NON-TRACKING MODES ###########
static float pre_f_bar_value = 1.0f;
static float pre_b_bar_value = 1.0f;
static float diff_f = 0.0f;
static float diff_b = 0.0f;

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

#define __1_by_100_Q16__ 655

void scalar_comp_utility(uint8_t foreground_thold,
                         uint8_t background_thold,
                         uint8_t depth_min,
                         uint8_t depth_max,
                         uint16_t& thresh_f_new,
                         uint16_t& thresh_b_new) {
    const uint8 max_8 = 255;
    uint8 dm_max = max_8;
    uint8 dm_min = 0;
    uint8 dm_max_min_diff = (dm_max - dm_min);

    uint8 pd_min = depth_min;
    uint8 pd_max = depth_max;
    uint8 pd_max_min_diff = (pd_max - pd_min);

    uint8 fg_tmp = (dm_max_min_diff * foreground_thold * __1_by_100_Q16__) >> 16;
    uint8 bg_tmp = (dm_max_min_diff * background_thold * __1_by_100_Q16__) >> 16;
    uint8 depth_threshold_f = (uint8)(dm_max - fg_tmp);
    uint8 depth_threshold_b = bg_tmp; //(T)(dm_min + bg_tmp); // dm_min is always '0'
    thresh_f_new = depth_threshold_f * pd_max_min_diff;
    thresh_b_new = depth_threshold_b * pd_max_min_diff;
}

uint8 scalar_tracking_comp_utility(uint8_t foreground_thold,
                                   uint8_t background_thold,
                                   uint8_t depth_min,
                                   uint8_t depth_max,
                                   uint32 non_zero_count,
                                   uint32 sum,
                                   uint16_t& thresh_f_new,
                                   uint16_t& thresh_b_new) {
    uint8 depth_threshold_or = (uint8)((float)sum / (float)non_zero_count);

    const uint8 max_8 = 255;
    uint8 dm_max = max_8;
    uint8 dm_min = 0;
    uint8 dm_max_min_diff = (dm_max - dm_min);

    uint8 pd_min = depth_min;
    uint8 pd_max = depth_max;
    uint8 pd_max_min_diff = (pd_max - pd_min);

    uint8 fg_tmp = (dm_max_min_diff * foreground_thold * __1_by_100_Q16__) >> 16;
    uint8 bg_tmp = (dm_max_min_diff * background_thold * __1_by_100_Q16__) >> 16;
    uint8 depth_threshold_f = (uint8)(dm_max - fg_tmp);
    uint8 depth_threshold_b = bg_tmp; //(T)(dm_min + bg_tmp); // dm_min is always '0'

    float foreground_thold_cmp = (foreground_thold / 100.0f);
    float background_thold_cmp = (background_thold / 100.0f);
    if (pre_f_bar_value != foreground_thold_cmp) {
        pre_f_bar_value = foreground_thold_cmp;
        diff_f = depth_threshold_f - depth_threshold_or;
    }
    if (pre_b_bar_value != background_thold_cmp) {
        pre_b_bar_value = background_thold_cmp;
        diff_b = depth_threshold_or - depth_threshold_b;
    }

    if (foreground_thold_cmp != 0) depth_threshold_f = depth_threshold_or + (int)diff_f;
    if (background_thold_cmp != 0) depth_threshold_b = depth_threshold_or - (int)diff_b;

    thresh_f_new = depth_threshold_f * pd_max_min_diff;
    thresh_b_new = depth_threshold_b * pd_max_min_diff;
    return depth_threshold_or;
}

#endif // __XF_MASKGEN_UTIL_H_