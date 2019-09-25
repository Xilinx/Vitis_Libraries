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

#include "xf_hog_descriptor_config.h"

void hog_descriptor_accel(xf::cv::Mat<XF_INPUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& inMat,
                          xf::cv::Mat<XF_32UC1, 1, XF_DESC_SIZE, XF_NPPC1>& outMat) {
    xf::cv::HOGDescriptor<XF_WIN_HEIGHT, XF_WIN_WIDTH, XF_WIN_STRIDE, XF_BLOCK_HEIGHT, XF_BLOCK_WIDTH, XF_CELL_HEIGHT,
                          XF_CELL_WIDTH, XF_NO_OF_BINS, XF_DESC_SIZE, XF_INPUT_COLOR, XF_OUTPUT_MODE, XF_INPUT_TYPE,
                          XF_32UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1, XF_USE_URAM>(inMat, outMat);
}
