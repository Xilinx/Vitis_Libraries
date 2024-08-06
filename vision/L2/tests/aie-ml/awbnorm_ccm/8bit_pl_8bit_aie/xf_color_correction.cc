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

#include "imgproc/xf_awb_norm_aie2.hpp"
template <typename T, unsigned int N>
void awbNormCCM(adf::input_buffer<T>& input1, const int16_t (&coeff)[28], adf::output_buffer<T>& output) {
    xf::cv::aie::awbnorm_ccm_api<T, N>(input1, coeff, output);
};
