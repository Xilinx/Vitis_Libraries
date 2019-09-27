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

#include "xf_pyr_dense_optical_flow_config.h"

void pyr_dense_optical_flow_pyr_down_accel(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> mat_imagepyr1[NUM_LEVELS],
                                           xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> mat_imagepyr2[NUM_LEVELS]) {
    for (int pyr_comp = 0; pyr_comp < NUM_LEVELS - 1; pyr_comp++) {
// clang-format off
        #pragma SDS async(1)
        #pragma SDS resource(1)
        // clang-format on
        xf::cv::pyrDown<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1, XF_USE_URAM>(mat_imagepyr1[pyr_comp],
                                                                       mat_imagepyr1[pyr_comp + 1]);
// clang-format off
        #pragma SDS async(2)
        #pragma SDS resource(2)
        // clang-format on
        xf::cv::pyrDown<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1, XF_USE_URAM>(mat_imagepyr2[pyr_comp],
                                                                       mat_imagepyr2[pyr_comp + 1]);
// clang-format off
        #pragma SDS wait(1)
        #pragma SDS wait(2)
        // clang-format on
    }
}

void pyr_dense_optical_flow_accel(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>& _current_img,
                                  xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>& _next_image,
                                  xf::cv::Mat<XF_32UC1, HEIGHT, WIDTH, XF_NPPC1>& _streamFlowin,
                                  xf::cv::Mat<XF_32UC1, HEIGHT, WIDTH, XF_NPPC1>& _streamFlowout,
                                  const int level,
                                  const unsigned char scale_up_flag,
                                  float scale_in,
                                  ap_uint<1> init_flag) {
    xf::cv::densePyrOpticalFlow<NUM_LEVELS, NUM_LINES_FINDIT, WINSIZE_OFLOW, TYPE_FLOW_WIDTH, TYPE_FLOW_INT, XF_8UC1,
                                HEIGHT, WIDTH, XF_NPPC1, XF_USE_URAM>(
        _current_img, _next_image, _streamFlowin, _streamFlowout, level, scale_up_flag, scale_in, init_flag);
}
