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

#include "xf_remap_config.h"

void remap_accel(xf::cv::Mat<TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& inMat,
                 xf::cv::Mat<TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& remappedMat,
                 xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& mapxMat,
                 xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& mapyMat) {
    xf::cv::remap<XF_WIN_ROWS, XF_REMAP_INTERPOLATION, TYPE, XF_32FC1, TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1,
                  XF_USE_URAM>(inMat, remappedMat, mapxMat, mapyMat);
}
