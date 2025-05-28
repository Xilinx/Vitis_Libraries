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
#include "imgproc/xf_sobel_impl.hpp"

template <int TILE_HEIGHT, int NO_DISPARITIES, int TILE_WINSIZE>
void SobelRunner<TILE_HEIGHT, NO_DISPARITIES, TILE_WINSIZE>::run(adf::input_buffer<uint8_t>& in,
                                                                 adf::output_buffer<uint8_t>& metadata_out,
                                                                 adf::output_buffer<uint8_t>& out) {
    xf::cv::aie::SobelBaseImpl<TILE_HEIGHT, NO_DISPARITIES, TILE_WINSIZE> sobel;
    sobel.runImpl(in, metadata_out, out);
}
