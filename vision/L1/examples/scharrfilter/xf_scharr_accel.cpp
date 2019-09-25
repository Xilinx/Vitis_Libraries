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

#include "xf_scharr_config.h"

void scharr_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1>& _src,
                  xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1>& _dstgx,
                  xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1>& _dstgy) {
    xf::cv::Scharr<XF_BORDER_CONSTANT, IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC1>(_src, _dstgx, _dstgy);
}
