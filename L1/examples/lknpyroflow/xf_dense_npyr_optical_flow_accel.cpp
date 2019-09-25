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

#include "xf_dense_npyr_optical_flow_config.h"

void dense_non_pyr_of_accel(xf::cv::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>& buf0,
                            xf::cv::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>& buf1,
                            xf::cv::Mat<XF_32FC1, MAX_HEIGHT, MAX_WIDTH, NPPC>& flowx,
                            xf::cv::Mat<XF_32FC1, MAX_HEIGHT, MAX_WIDTH, NPPC>& flowy) {
    xf::cv::DenseNonPyrLKOpticalFlow<KMED, XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC, XF_USE_URAM>(buf0, buf1, flowx, flowy);
}
