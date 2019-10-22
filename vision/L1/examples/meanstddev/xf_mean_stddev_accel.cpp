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

#include "xf_mean_stddev_config.h"

void mean_stddev_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, _NPPC>& imgInput,
                       unsigned short mean[XF_CHANNELS(TYPE, _NPPC)],
                       unsigned short stddev[XF_CHANNELS(TYPE, _NPPC)]) {
#pragma HLS interface m_axi port = mean
#pragma HLS interface m_axi port = stddev
    xf::cv::meanStdDev<TYPE, HEIGHT, WIDTH, _NPPC>(imgInput, mean, stddev);
}
