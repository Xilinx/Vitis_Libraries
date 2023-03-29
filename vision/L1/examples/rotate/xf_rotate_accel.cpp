/*
 * Copyright 2022 Xilinx, Inc.
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

#include "xf_rotate_accel_config.h"

static constexpr int __XF_DEPTH = (HEIGHT * WIDTH * (XF_PIXELWIDTH(IN_TYPE, NPPCX)) / 8) / (INPUT_PTR_WIDTH / 8);

void rotate_accel(
    ap_uint<INPUT_PTR_WIDTH>* img_in, ap_uint<OUTPUT_PTR_WIDTH>* img_out, int height, int width, int direction) {
// clang-format off
#pragma HLS INTERFACE m_axi port=img_in offset=slave bundle=gmem0 depth=__XF_DEPTH
#pragma HLS INTERFACE m_axi port=img_out offset=slave bundle=gmem1 depth=__XF_DEPTH
#pragma HLS INTERFACE s_axilite port=height
#pragma HLS INTERFACE s_axilite port=width
#pragma HLS INTERFACE s_axilite port=direction
#pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    // Run xfOpenCV kernel:
    xf::cv::rotate<INPUT_PTR_WIDTH, OUTPUT_PTR_WIDTH, IN_TYPE, TILE_SIZE, HEIGHT, WIDTH, NPPCX>(img_in, img_out, height,
                                                                                                width, direction);

    return;
} // End of kernel
