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

#include "xf_channel_extract_config.h"

void channel_extract_accel(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1>& imgInput,
                           xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>& imgOutput,
                           unsigned short channel) {
    xf::cv::extractChannel<XF_8UC4, XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(imgInput, imgOutput, channel);
}
