/*
 * Copyright 2019 Xilinx, Inc.
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

#include "xf_mean_shift_config.h"

void mean_shift_accel(xf::cv::Mat<XF_8UC4, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& inMat,
                      uint16_t* tlx,
                      uint16_t* tly,
                      uint16_t* obj_height,
                      uint16_t* obj_width,
                      uint16_t* dx,
                      uint16_t* dy,
                      uint16_t* track,
                      uint8_t frame_status,
                      uint8_t no_objects,
                      uint8_t no_of_iterations) {
// clang-format off
    #pragma HLS INTERFACE m_axi depth=10 port=tlx offset=direct bundle=in
    #pragma HLS INTERFACE m_axi depth=10 port=tly offset=direct bundle=in
    #pragma HLS INTERFACE m_axi depth=10 port=obj_height offset=direct bundle=in
    #pragma HLS INTERFACE m_axi depth=10 port=obj_width offset=direct bundle=in
    #pragma HLS INTERFACE m_axi depth=10 port=dx offset=direct bundle=out
    #pragma HLS INTERFACE m_axi depth=10 port=dy offset=direct bundle=out
    #pragma HLS INTERFACE m_axi depth=10 port=track offset=direct bundle=out
    // clang-format on

    xf::cv::MeanShift<XF_MAX_OBJECTS, XF_MAX_ITERS, XF_MAX_OBJ_HEIGHT, XF_MAX_OBJ_WIDTH, XF_8UC4, XF_HEIGHT, XF_WIDTH,
                      XF_NPPC1>(inMat, tlx, tly, obj_height, obj_width, dx, dy, track, frame_status, no_objects,
                                no_of_iterations);
}
