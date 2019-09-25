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

#include "xf_gaussian_diff_config.h"

void gaussian_diff_accel(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin1,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin2,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin3,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin4,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin5,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput,
                         float sigma) {
    xf::cv::GaussianBlur<FILTER_WIDTH, XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgin1, sigma);
    xf::cv::duplicateMat<XF_8UC1, HEIGHT, WIDTH, NPC1>(imgin1, imgin2, imgin3);
    xf::cv::delayMat<MAXDELAY, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgin3, imgin5);
    xf::cv::GaussianBlur<FILTER_WIDTH, XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgin2, imgin4, sigma);
    xf::cv::subtract<XF_CONVERT_POLICY_SATURATE, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgin5, imgin4, imgOutput);
}
