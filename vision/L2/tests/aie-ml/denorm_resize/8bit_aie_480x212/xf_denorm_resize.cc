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

#include "kernels.h"
#include "dnn/xf_resize_norm.hpp"

void DenormResizeRunner::run(input_window<int8_t>* input,
                             output_window<uint8_t>* output,
                             int row,
                             uint32_t scale_x,
                             uint32_t scale_y,
                             int img_height_in,
                             int tile_width_in,
                             int line_stride_in,
                             int tile_width_out,
                             const int16_t (&coeff)[8]) {
    int8_t* img_in_ptr = (int8_t*)input->ptr;
    uint8_t* img_out_ptr = (uint8_t*)output->ptr;

    xf::cv::aie::ResizeNorm denorm_resize(mnFBitsIn, mnFBitsAlpha, mnFBitsBeta, mnFBitsOut);
    denorm_resize.runImpl(img_in_ptr, img_out_ptr, row, scale_x, scale_y, img_height_in, tile_width_in, line_stride_in,
                          tile_width_out, coeff);
}
