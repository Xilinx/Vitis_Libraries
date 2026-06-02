/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

/**
 * @file xf_waitMat.hpp
 * @brief Row-major stream passthrough: copy @p src xf::cv::Mat stream to @p dst.
 */

#ifndef _XF_WAIT_MAT_HPP_
#define _XF_WAIT_MAT_HPP_

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "hls_stream.h"

namespace xf {
namespace cv {

template <int ROWS, int COLS, int XF_WIN_ROWS, int MASK_T, int NPC, int XFCVDEPTH>
void waitMat(xf::cv::Mat<MASK_T, ROWS, COLS, NPC, XFCVDEPTH>& src,
             xf::cv::Mat<MASK_T, ROWS, COLS, NPC, XFCVDEPTH>& dst) {
#pragma HLS INLINE OFF

    int read_pointer_src = 0;
passthrough:
    for (int r = 0; r < src.rows + XF_WIN_ROWS; ++r) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = ROWS + XF_WIN_ROWS
        if (r >= XF_WIN_ROWS) {
            for (int c = 0; c < src.cols; ++c) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = COLS
#pragma HLS PIPELINE II = 1
                dst.write(r * src.cols + c, src.read(read_pointer_src++));
            }
        }
    }
}

} // namespace cv
} // namespace xf

#endif /* _XF_WAIT_MAT_HPP_ */
