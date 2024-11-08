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
#include "imgproc/xf_topk.hpp"

void topk_api(adf::input_buffer<uint16_t>& input_data,
              adf::output_buffer<uint16_t>& output_scores,
              adf::output_buffer<uint16_t>& output_indices,
              const int& num_elem,
              const int& ktop,
              const int& start_idx) {
    xf::cv::aie::TopK topK = xf::cv::aie::TopK();
    topK.runImpl(input_data, output_scores, output_indices, num_elem, ktop, start_idx);
}
