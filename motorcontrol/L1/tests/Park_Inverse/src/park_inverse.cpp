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

#include "park_inverse.h"
#include "sin_cos_table.h"
#include "park.hpp"

// hls namespace has to be included in all HLS C source files.
using namespace hls;

// Park Inverse
// Valpha = Vd*cos(Theta) - Vq*sin(Theta)
// Vbeta = Vq*cos(Theta) + Vd*sin(Theta)
void Park_Inverse_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis) {
#pragma HLS interface axis port = m_axis
#pragma HLS interface axis port = s_axis
    int64_t in_data, res;
    ap_int<in_width> Vd, Vq, Theta;
    ap_int<out_width> Valpha, Vbeta;

    // Decode Input stream
    in_data = s_axis.read();                            // Read one value from AXI4-Stream
    Vd = ap_int<in_width>(in_data & 0xFFFF);            // Extract Vd - bits[15..0] from input stream
    Vq = ap_int<in_width>((in_data >> 16) & 0xFFFF);    // Extract Vq - bits[32..16] from input stream
    Theta = ap_int<in_width>((in_data >> 32) & 0xFFFF); // Extract Theta - bits[47..32] from input stream

    Park_Inverse_T_AP<in_width, mid_width, out_width, max_limit, min_limit, sincos_width, sincos_scale>(
        Valpha, Vbeta, Vd, Vq, Theta, (ap_int<sincos_width>*)cos_table, (ap_int<sincos_width>*)sin_table);

    // Write output stream
    res = (((int64_t)Theta << 32) & 0x0000FFFF00000000) | // Put Theta bits[47:32]
          (((int64_t)Vbeta << 16) & 0x00000000FFFF0000) | // Put Vbeta bits[31:16]
          ((int64_t)Valpha & 0x000000000000FFFF);         // Put Valpha bits[15:0]
    m_axis.write(res);                                    // Write result to the output stream
}
