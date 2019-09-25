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

#include "xf_convert_bitdepth_config.h"

void convert_bitdepth_accel(xf::cv::Mat<_SRC_T, HEIGHT, WIDTH, _NPC>& imgInput,
                            xf::cv::Mat<_DST_T, HEIGHT, WIDTH, _NPC>& imgOutput,
                            ap_int<4> _convert_type,
                            int shift) {
    xf::cv::convertTo<_SRC_T, _DST_T, HEIGHT, WIDTH, _NPC>(imgInput, imgOutput, _convert_type, shift);
}
