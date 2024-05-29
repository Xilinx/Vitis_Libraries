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
#include "config.h"
#include "imgproc/xf_rgba2gray_aie2.hpp"

void rgba2grey(adf::input_buffer<uint8_t>& in, adf::output_buffer<uint8_t>& out) {
    xf::cv::aie::Rgba2Gray k = xf::cv::aie::Rgba2Gray();
    k.runImpl(in, out);
};
