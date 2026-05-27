/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
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
#include "imgproc/xf_stddev_rgb888.hpp"

void StddevRunner::run(adf::input_buffer<uint8_t>& input,
                       adf::output_buffer<uint8_t>& output,
                       uint32_t reset,
                       float m_r,
                       float m_g,
                       float m_b) {
    xf::cv::aie::Stddev s;
    s.runImpl(input, output, reset, m_r, m_g, m_b, IMAGE_HEIGHT_IN, CHANNELS);
}
