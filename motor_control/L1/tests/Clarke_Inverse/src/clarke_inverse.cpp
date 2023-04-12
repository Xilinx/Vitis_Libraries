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
#include "clarke_inverse.h"
#include "clarke_2p.hpp"

using namespace hls;

//--------------------------------------------------------------------------
// Clarke Inverse
// Va = Valpha
// Vb = [-Valpha + sqrt(3)*Vbeta]/2
// Vc = [-Valpha - sqrt(3)*Vbeta]/2
// Va, Vb, Vc;							// Clarke Inverse -> SVPWM
//--------------------------------------------------------------------------

void Clarke_Inverse_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis) {
#pragma HLS interface axis port = m_axis
#pragma HLS interface axis port = s_axis
    int64_t in_data, res;
    int16_t Valpha, Vbeta, Theta;
    int32_t s3vb;                             // Clarke Inverse
    ap_int<out_width> va_out, vb_out, vc_out; // Clarke Inverse -> SVPWM

    // Decode Input stream
    in_data = s_axis.read();                   // Read one value from AXI4-Stream
    Valpha = int16_t(in_data & 0xFFFF);        // Extract Valpha - bits[15..0] from input stream
    Vbeta = int16_t((in_data >> 16) & 0xFFFF); // Extract Vbeta - bits[31..16] from input stream
    Theta = int16_t((in_data >> 32) & 0xFFFF); // Extract Theta - bits[47..32] from input stream

    // Process data
    ap_int<in_width> va_in = Valpha;
    ap_int<in_width> vb_in = Vbeta;
    Clarke_Inverse_2p_T_AP<in_width, mid_width, out_width, max_limit, min_limit, sqrt3c, sqrt3c_shif>(
        va_out, vb_out, vc_out, va_in, vb_in);

    // Write output stream
    res = (((int64_t)Theta << 48) & 0xFFFF000000000000) |  // Put Theta bits[63:48]
          (((int64_t)vc_out << 32) & 0x0000FFFF00000000) | // Put Vc bits[47:32]
          (((int64_t)vb_out << 16) & 0x00000000FFFF0000) | // Put Vb bits[31:16]
          ((int64_t)va_out & 0x000000000000FFFF);          // Put Va bits[15:0]
    m_axis.write(res);                                     // Write result to the output stream
}
