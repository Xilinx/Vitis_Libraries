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
#include "imgproc/xf_resize_bicubic.hpp"

void ResizeRunner::run(adf::input_buffer<uint8_t>& input,
                       adf::output_buffer<uint8_t>& output,
                       int channels,
                       uint32_t scale_x,
                       uint32_t scale_y,
                       int img_height_in,
                       int img_height_out,
                       int tile_height_out,
                       int tile_width_out,
                       int line_stride_in,
                       int img_width_out,
                       float scale_y_f) {
    uint8_t* img_in_ptr = (uint8_t*)::aie::begin(input);
    uint8_t* img_out_ptr = (uint8_t*)::aie::begin(output);

    xf::cv::aie::Resizebicubic resize(mwtsY);
    resize.runImpl(img_in_ptr, img_out_ptr, channels, scale_x, scale_y, img_height_in, img_height_out, tile_height_out,
                   tile_width_out, line_stride_in, img_width_out, scale_y_f);
}
