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
#include "imgproc/xf_rgba2yuv_aie2.hpp"

void rgba2yuv_api(adf::input_buffer<uint8_t>& input1,
                  adf::output_buffer<uint8_t>& output1,
                  adf::output_buffer<uint8_t>& output2,
                  uint16_t tile_width,
                  uint16_t tile_height) {
    uint8_t* img_in_ptr_rgba = (uint8_t*)::aie::begin(input1);
    uint8_t* img_out_ptr_y = (uint8_t*)::aie::begin(output1);
    uint8_t* img_out_ptr_uv = (uint8_t*)::aie::begin(output2);
    xf::cv::aie::Rgba2YuvBaseImpl rgba2yuv_obj;
    rgba2yuv_obj.runImpl(img_in_ptr_rgba, img_out_ptr_y, img_out_ptr_uv, tile_width, tile_height);
}
