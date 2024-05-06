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
#include "imgproc/xf_yuv2rgba_aie2.hpp"

void yuv2rgba_api(adf::input_buffer<uint8_t>& input1,
                  adf::input_buffer<uint8_t>& input2,
                  adf::output_buffer<uint8_t>& output,
                  uint16_t tile_width,
                  uint16_t tile_height) {
    uint8_t* img_in_ptr_y = (uint8_t*)::aie::begin(input1);
    uint8_t* img_in_ptr_uv = (uint8_t*)::aie::begin(input2);
    uint8_t* img_out_ptr = (uint8_t*)::aie::begin(output);
    xf::cv::aie::Yuv2Rgba yuv2rgba_obj;
    yuv2rgba_obj.runImpl(img_in_ptr_y, img_in_ptr_uv, img_out_ptr, tile_width, tile_height);
}
