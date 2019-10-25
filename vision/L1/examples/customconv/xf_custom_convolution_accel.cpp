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

#include "xf_custom_convolution_config.h"

void Filter2d_accel(xf::cv::Mat<INTYPE, HEIGHT, WIDTH, NPC_T>& _src,
                    xf::cv::Mat<OUTTYPE, HEIGHT, WIDTH, NPC_T>& _dst,
                    short int* filter_ptr,
                    unsigned char shift) {
    const int depth_filter = FILTER_HEIGHT * FILTER_WIDTH;
// clang-format off
    #pragma HLS INTERFACE m_axi depth=depth_filter port=filter_ptr offset=direct bundle=filter_ptr
    // clang-format on

    xf::cv::filter2D<XF_BORDER_CONSTANT, FILTER_WIDTH, FILTER_HEIGHT, INTYPE, OUTTYPE, HEIGHT, WIDTH, NPC_T>(
        _src, _dst, filter_ptr, shift);
}
