/*
 * Copyright 2019 Xilinx, Inc.
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

#include "xf_inrange_config.h"

void inrange_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPIX>& _src,
                   unsigned char* lower_thresh,
                   unsigned char* upper_thresh,
                   xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPIX>& _dst) {
#pragma HLS INTERFACE m_axi depth = 3 port = lower_thresh offset = direct bundle = in
#pragma HLS INTERFACE m_axi depth = 3 port = upper_thresh offset = direct bundle = in
    xf::cv::inRange<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPIX>(_src, lower_thresh, upper_thresh, _dst);
}
