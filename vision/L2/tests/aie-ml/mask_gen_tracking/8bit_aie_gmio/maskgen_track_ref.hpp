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

#ifndef __XF_MASKGEN_TRACK_REF_HPP_
#define __XF_MASKGEN_TRACK_REF_HPP_

#include <cstdint>
#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;

static float pre_f_bar_value_r = 1.0f;
static float pre_b_bar_value_r = 1.0f;
static float diff_f_r = 0.0f;
static float diff_b_r = 0.0f;

uint8 scalar_tracking_comp_utility_ref(uint8_t foreground_thold,
                                       uint8_t background_thold,
                                       uint8_t depth_min,
                                       uint8_t depth_max,
                                       uint32 non_zero_count,
                                       uint32 sum,
                                       uint8& thresh_f_new,
                                       uint8& thresh_b_new) {
    uint8 depth_threshold_or = (uint8)((float)sum / (float)non_zero_count);

    const uint8 max_8 = 255;
    const int one_by_100_Q16_R = 655;
    uint8 dm_max = max_8;
    uint8 dm_min = 0;
    uint8 dm_max_min_diff = (dm_max - dm_min);

    uint8 pd_min = depth_min;
    uint8 pd_max = depth_max;
    uint8 pd_max_min_diff = (pd_max - pd_min);

    uint8 fg_tmp = (dm_max_min_diff * foreground_thold * one_by_100_Q16_R) >> 16;
    uint8 bg_tmp = (dm_max_min_diff * background_thold * one_by_100_Q16_R) >> 16;
    uint8 depth_threshold_f = (uint8)(dm_max - fg_tmp);
    uint8 depth_threshold_b = bg_tmp; //(T)(dm_min + bg_tmp); // dm_min is always '0'

    float foreground_thold_cmp = (foreground_thold / 100.0f);
    float background_thold_cmp = (background_thold / 100.0f);
    if (pre_f_bar_value_r != foreground_thold_cmp) {
        pre_f_bar_value_r = foreground_thold_cmp;
        diff_f_r = depth_threshold_f - depth_threshold_or;
    }
    if (pre_b_bar_value_r != background_thold_cmp) {
        pre_b_bar_value_r = background_thold_cmp;
        diff_b_r = depth_threshold_or - depth_threshold_b;
    }

    if (foreground_thold_cmp != 0) depth_threshold_f = depth_threshold_or + (int)diff_f_r;
    if (background_thold_cmp != 0) depth_threshold_b = depth_threshold_or - (int)diff_b_r;

    thresh_f_new = depth_threshold_f;
    thresh_b_new = depth_threshold_b;
    return depth_threshold_or;
}

void maskgen_track_param_ref(uint8* pd_ptr,
                             uint8* ps_ptr,
                             uint8 pd_min,
                             uint8 pd_max,
                             uint8 foreground_thold,
                             uint8 background_thold,
                             uint8 pred_seg_thresh,
                             int rows,
                             int cols,
                             uint8& depth_or,
                             uint32_t& sum_ret,
                             uint32_t& non_zero_ret,
                             uint8& fg_thresh_track,
                             uint8& bg_thresh_track) {
    uint8 max_val = 255;
    uint8 pd_max_min_diff = (pd_max - pd_min);
    unsigned int non_zero_num = 0, sum = 0;
    for (int i = 0; i < rows; i++) {
        unsigned int sum_l = 0;
        for (int j = 0; j < cols; j++) {
            uint8 ps_val = ps_ptr[i * cols + j];
            ps_val = (ps_val > pred_seg_thresh);
            int val = (max_val * (pd_ptr[i * cols + j] - pd_min));
            int person_depth_mask = ps_val * val;
            non_zero_num += (person_depth_mask > 0 ? 1 : 0);
            sum_l += person_depth_mask;
        }
        sum += (sum_l / pd_max_min_diff);
    }
    sum_ret = sum;
    non_zero_ret = non_zero_num;
    depth_or = scalar_tracking_comp_utility_ref(foreground_thold, background_thold, pd_min, pd_max, non_zero_num, sum,
                                                fg_thresh_track, bg_thresh_track);
}

#endif //__XF_MASKGEN_TRACK_REF_HPP_
