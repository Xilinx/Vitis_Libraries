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

#include "xf_harris_config.h"

void harris_accel(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _src,
                  xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _dst,
                  unsigned short Thresh,
                  unsigned short k) {
    xf::cv::cornerHarris<FILTER_WIDTH, BLOCK_WIDTH, NMS_RADIUS, XF_8UC1, HEIGHT, WIDTH, NPIX, XF_USE_URAM>(_src, _dst,
                                                                                                           Thresh, k);
}
