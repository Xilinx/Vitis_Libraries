/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2024 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kernels.h"
#include "imgproc/xf_sbm_impl.hpp"

template <int TILE_HEIGHT,
          int NO_DISPARITIES,
          int TILE_WINSIZE,
          int UNIQUENESS_RATIO,
          int TEXTURE_THRESHOLD,
          int FILTERED,
          int TILE_IN_HEIGHT>
void SbmRunner<TILE_HEIGHT,
               NO_DISPARITIES,
               TILE_WINSIZE,
               UNIQUENESS_RATIO,
               TEXTURE_THRESHOLD,
               FILTERED,
               TILE_IN_HEIGHT>::run(adf::input_buffer<uint8_t>& in_1,
                                    adf::input_buffer<uint8_t>& in_2,
                                    adf::input_buffer<uint8_t>& in_3,
                                    adf::input_buffer<uint8_t>& in_4,
                                    adf::input_buffer<uint8_t>& in_5,
                                    adf::output_buffer<int16_t>& out) {
    xf::cv::aie::SbmBaseImpl<TILE_HEIGHT, NO_DISPARITIES, TILE_WINSIZE, UNIQUENESS_RATIO, TEXTURE_THRESHOLD, FILTERED,
                             TILE_IN_HEIGHT>
        rgba2yuv;
    rgba2yuv.runImpl(in_1, in_2, in_3, in_4, in_5, out);
}
