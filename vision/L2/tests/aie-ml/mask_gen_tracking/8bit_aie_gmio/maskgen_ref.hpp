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

#ifndef __XF_MASKGEN_REF_HPP_
#define __XF_MASKGEN_REF_HPP_

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

void maskgen_ref(uint8* pd_ptr,
                 uint8* mask_ptr,
                 bool tracking,
                 uint8 pd_min,
                 uint8 pd_max,
                 uint8 foreground_thold,
                 uint8 background_thold,
                 uint8 depth_thresh_f_track,
                 uint8 depth_thresh_b_track,
                 int rows,
                 int cols) {
    const int one_by_100_Q16_R = 655;
    uint8 max_val = 255;
    uint8 dm_max = max_val;
    uint8 dm_min = 0;

    uint8 pd_max_min_diff = (pd_max - pd_min);

    uint8 foreground_thold_1 = ((dm_max - dm_min) * foreground_thold * one_by_100_Q16_R) >> 16;
    uint8 background_thold_1 = ((dm_max - dm_min) * background_thold * one_by_100_Q16_R) >> 16;
    uint8 depth_threshold_f = (uint8)(dm_max - foreground_thold_1);
    uint8 depth_threshold_b = (uint8)(dm_min + background_thold_1);

    if (tracking) {
        depth_threshold_f = depth_thresh_f_track;
        depth_threshold_b = depth_thresh_b_track;
    }

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int val = 0;
            if ((pd_max - pd_min) > 0) {
                val = max_val * (pd_ptr[i * cols + j] - pd_min);
            }

            bool mask_remove_f = (val > (depth_threshold_f * pd_max_min_diff));
            bool mask_remove_b = (val < (depth_threshold_b * pd_max_min_diff));
            bool mask_for_editing = (mask_remove_b || mask_remove_f);
            mask_ptr[i * cols + j] = (uint8)mask_for_editing;
        }
    }
    return;
}

#endif //__XF_MASKGEN_REF_HPP_
