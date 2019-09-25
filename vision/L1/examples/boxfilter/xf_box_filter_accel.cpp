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

#include "xf_box_filter_config.h"

void boxfilter_accel(xf::cv::Mat<IN_T, HEIGHT, WIDTH, NPIX>& _src, xf::cv::Mat<IN_T, HEIGHT, WIDTH, NPIX>& _dst) {
    xf::cv::boxFilter<XF_BORDER_CONSTANT, FILTER_WIDTH, IN_T, HEIGHT, WIDTH, NPIX, XF_USE_URAM>(_src, _dst);
}
