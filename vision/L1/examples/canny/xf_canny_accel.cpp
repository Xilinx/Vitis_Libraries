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

#include "xf_canny_config.h"

void canny_accel(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, INTYPE>& _src,
                 xf::cv::Mat<XF_2UC1, HEIGHT, WIDTH, XF_NPPC32>& _dst1,
                 xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC8>& _dst2,
                 unsigned char low_threshold,
                 unsigned char high_threshold) {
    // clang-format off
    #pragma SDS async(1)
    // clang-format on

    xf::cv::Canny<FILTER_WIDTH, NORM_TYPE, XF_8UC1, XF_2UC1, HEIGHT, WIDTH, INTYPE, XF_NPPC32, XF_USE_URAM>(
        _src, _dst1, low_threshold, high_threshold);

    // clang-format off
    #pragma SDS wait(1)
    // clang-format on

    xf::cv::EdgeTracing<XF_2UC1, XF_8UC1, HEIGHT, WIDTH, XF_NPPC32, XF_NPPC8, XF_USE_URAM>(_dst1, _dst2);
}
