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

#include "xf_crop_config.h"

void crop_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC>& _src,
                xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> _dst[NUM_ROI],
                xf::cv::Rect_<unsigned int> roi[NUM_ROI]) {
#if MEMORYMAPPED_ARCH

    for (int i = 0; i < NUM_ROI; i++) {
        xf::cv::crop<TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPC>(_src, _dst[i], roi[i]);
    }
#else
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> _src1(_src.rows, _src.cols);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> _src2(_src.rows, _src.cols);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> _src3(_src.rows, _src.cols);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> _src4(_src.rows, _src.cols);
    // clang-format off
    #pragma HLS dataflow
    // clang-format on
    xf::cv::duplicateMat<TYPE, HEIGHT, WIDTH, NPC>(_src, _src1, _src2);
    xf::cv::duplicateMat<TYPE, HEIGHT, WIDTH, NPC>(_src1, _src3, _src4);
    xf::cv::crop<TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPC>(_src1, _dst[0], roi[0]);
    xf::cv::crop<TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPC>(_src2, _dst[1], roi[1]);
    xf::cv::crop<TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPC>(_src4, _dst[2], roi[2]);

#endif
}
