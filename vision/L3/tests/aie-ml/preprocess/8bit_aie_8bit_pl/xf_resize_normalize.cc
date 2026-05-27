/*
 * Copyright 2021 Xilinx, Inc.
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

#include "kernels_resize_norm.h"
#include "imgproc/xf_resize_norm.hpp"

template <int FBITS>
uint32_t compute_scalefactor(int M, int N) {
    float x_scale = (float)M / (float)N;
    float scale = x_scale * (1 << FBITS);
    return (uint32_t)(std::roundf(scale));
}

void resize_norm_api(adf::input_buffer<uint8_t>& input, adf::output_buffer<int8_t>& output) {
    uint32_t scale_x = compute_scalefactor<16>(/*IMAGE_WIDTH_IN*/ CROP_WT, IMAGE_WIDTH_OUT);
    uint32_t scale_y = compute_scalefactor<16>(/*IMAGE_HEIGHT_IN*/ CROP_HT, IMAGE_HEIGHT_OUT);
    const short coeff[8] = {104, 107, 123, 0, 8, 8, 8, 0};
    xf::cv::aie::ResizeNorm resize_norm;
    uint8_t FBits[3] = {0, 4, 0};

    resize_norm.runImpl(input, output, scale_x, scale_y, /*IMAGE_HEIGHT_IN*/ CROP_HT, coeff, FBits, CHANNELS);
}
