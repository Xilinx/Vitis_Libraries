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

#include "xf_houghlines_config.h"

void houghlines_accel(
    xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& _src, float* arrayy, float* arrayx, short threshold, short maxlines) {
#pragma HLS interface m_axi port = arrayy depth = 512
#pragma HLS interface m_axi port = arrayx depth = 512
#pragma HLS INTERFACE s_axilite port = return bundle = lite

    xf::cv::HoughLines<RHOSTEP, THETASTEP, LINESMAX, DIAGVAL, MINTHETA, MAXTHETA, XF_8UC1, HEIGHT, WIDTH, NPC1>(
        _src, arrayy, arrayx, threshold, maxlines);
}
