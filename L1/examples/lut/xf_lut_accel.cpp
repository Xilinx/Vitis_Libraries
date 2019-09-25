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

#include "xf_lut_config.h"

void lut_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& imgInput,
               xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& imgOutput,
               unsigned char* lut_ptr) {
#pragma HLS INTERFACE m_axi depth = 256 port = lut_ptr offset = direct bundle = in

    xf::cv::LUT<TYPE, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput, lut_ptr);
}
