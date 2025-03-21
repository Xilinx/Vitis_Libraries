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

#include "imgproc/xf_nv12resize_aieml.hpp"
#include "kernels.h"

/* void nv12resize(adf::input_buffer<uint8_t>& input, adf::input_buffer<uint8_t>& UV_input, adf::output_buffer<uint8_t>&
output, adf::output_buffer<uint8_t>& UV_output) {
    xf::cv::aie::nv12resize_api(input, UV_input, output, UV_output);
}; */

void nv12resize(adf::input_buffer<uint8_t>& input, adf::output_buffer<uint8_t>& output) {
    xf::cv::aie::nv12resize_y(input, output);
}

void nv12resize_uv(adf::input_buffer<uint8_t>& UV_input, adf::output_buffer<uint8_t>& UV_output) {
    xf::cv::aie::nv12resize_uv(UV_input, UV_output);
}