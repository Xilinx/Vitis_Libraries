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

#include "imgproc/xf_yuy2_filter2d_aieml.hpp"
#include "kernels.h"

void yuy2_filter2D(adf::input_buffer<int16>& input, const int16_t (&coeff)[16], adf::output_buffer<int16>& output) {
    xf::cv::aie::yuy2_filter2D(input, coeff, output);
};
