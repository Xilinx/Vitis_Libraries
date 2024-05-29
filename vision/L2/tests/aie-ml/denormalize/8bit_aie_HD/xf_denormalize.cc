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
#include "xf_denormalize.hpp"

void Denormalize(adf::input_buffer<int8_t>& input, adf::output_buffer<uint8_t>& output, const int16_t (&coeff)[8]) {
    uint8_t FBits[3] = {7, 8, 5};

    xf::cv::aie::Denormalize_runImpl(input, output, coeff, FBits, CHANNELS);
}
