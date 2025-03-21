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

#include "xf_autogaincontrol_accel_config.h"
static bool flag = 0;

void autogaincontrol_accel(unsigned int histogram[XF_CHANNELS(IN_TYPE, NPPCX)][HIST_SIZE],
                           uint16_t gain[XF_CHANNELS(IN_TYPE, NPPCX)]) {
// clang-format off
  
	#pragma HLS INTERFACE s_axilite port=histogram     bundle=control
	#pragma HLS INTERFACE s_axilite port=gain     bundle=control
    #pragma HLS INTERFACE s_axilite port=return   bundle=control
    // clang-format on
    xf::cv::autogain<IN_TYPE, NPPCX, HIST_SIZE>(histogram, gain);
}
