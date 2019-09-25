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

#include "xf_phase_config.h"

void phase_accel(xf::cv::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1>& _src1,
                 xf::cv::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1>& _src2,
                 xf::cv::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1>& _dst) {
    xf::cv::phase<DEG_TYPE, XF_16SC1, XF_16SC1, HEIGHT, WIDTH, NPC1>(_src1, _src2, _dst);
}
