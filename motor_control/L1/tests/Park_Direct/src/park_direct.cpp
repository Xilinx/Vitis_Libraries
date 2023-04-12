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
#include "park_direct.h"
#include "sin_cos_table.h"
#include "park.hpp"

// hls namespace has to be included in all HLS C source files.
using namespace hls;

// Park Direct
// Id = Ialpha*cos(Theta) + Ibeta*sin(Theta)
// Iq = Ibeta*cos(Theta) - Ialpha*sin(Theta)

void Park_Direct_axi(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis) {
#pragma HLS interface axis port = m_axis
#pragma HLS interface axis port = s_axis
    int64_t in_data, res;
    ap_int<in_width> Ialpha, Ibeta, Theta, RPM;
    ap_int<out_width> Id, Iq;

    // Decode Input stream
    in_data = s_axis.read();                            // Read one value from AXI4-Stream
    Ialpha = ap_int<in_width>(in_data & 0xFFFF);        // Extract Ialpha - bits[15..0] from input stream
    Ibeta = ap_int<in_width>((in_data >> 16) & 0xFFFF); // Extract Ibeta - bits[32..16] from input stream
    RPM = ap_int<in_width>((in_data >> 32) & 0xFFFF);   // Extract RPM - bits[47..32] from input stream
    Theta = ap_int<in_width>((in_data >> 48) & 0xFFFF); // Extract Angle - bits[63..48] from input stream

    Park_Direct_T_AP<in_width, mid_width, out_width, max_limit, min_limit, sincos_width, scale_park>(
        Id, Iq, Ialpha, Ibeta, Theta, (ap_int<sincos_width>*)cos_table, (ap_int<sincos_width>*)sin_table);

    // Write output stream
    res = (((int64_t)Theta << 48) & 0xFFFF000000000000) | // Put Angle bits[63:48]
          (((int64_t)RPM << 32) & 0x0000FFFF00000000) |   // Put RPM bits[47:32]
          (((int64_t)Iq << 16) & 0x00000000FFFF0000) |    // Put Iq bits[31:16]
          ((int64_t)Id & 0x000000000000FFFF);             // Put Id bits[15:0]
    m_axis.write(res);                                    // Write result to the output stream
}

void Park_Direct_axi_ap_fixed(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis) {
#pragma HLS interface axis port = m_axis
#pragma HLS interface axis port = s_axis
    int64_t in_data, res;
    ap_int<in_width> Ialpha, Ibeta, Theta, RPM;
    ap_int<out_width> Id, Iq;

    // Decode Input stream
    in_data = s_axis.read();                            // Read one value from AXI4-Stream
    Ialpha = ap_int<in_width>(in_data & 0xFFFF);        // Extract Ialpha - bits[15..0] from input stream
    Ibeta = ap_int<in_width>((in_data >> 16) & 0xFFFF); // Extract Ibeta - bits[32..16] from input stream
    RPM = ap_int<in_width>((in_data >> 32) & 0xFFFF);   // Extract RPM - bits[47..32] from input stream
    Theta = ap_int<in_width>((in_data >> 48) & 0xFFFF); // Extract Angle - bits[63..48] from input stream

    ap_int<sincos_width> cos_theta_in = cos_table[Theta];
    ap_int<sincos_width> sin_theta_in = sin_table[Theta];
    typedef ap_fixed<16, 8> t_io;
    typedef ap_fixed<9, 1> t_sincos;
    t_io Ialpha_fixed, Ibeta_fixed, Theta_fixed, RPM_fixed, Id_fixed, Iq_fixed;
    t_sincos cos_in = (t_sincos)cos_theta_in >> 15;
    t_sincos sin_in = (t_sincos)sin_theta_in >> 15;
    Park_Direct_ap_fixed<t_io, t_sincos>(Id_fixed, Iq_fixed, Ialpha_fixed, Ibeta_fixed, cos_in, sin_in);

    Id.range(QmW_park + QnW_park, 0) = Id_fixed.range(QmW_park + QnW_park, 0);
    Iq.range(QmW_park + QnW_park, 0) = Iq_fixed.range(QmW_park + QnW_park, 0);
    // Write output stream
    res = (((int64_t)Theta << 48) & 0xFFFF000000000000) | // Put Angle bits[63:48]
          (((int64_t)RPM << 32) & 0x0000FFFF00000000) |   // Put RPM bits[47:32]
          (((int64_t)Iq << 16) & 0x00000000FFFF0000) |    // Put Iq bits[31:16]
          ((int64_t)Id & 0x000000000000FFFF);             // Put Id bits[15:0]
    m_axis.write(res);                                    // Write result to the output stream
}

void Park_Direct_axi_Qmn(hls::stream<int64_t>& s_axis, hls::stream<int64_t>& m_axis) {
#pragma HLS interface axis port = m_axis
#pragma HLS interface axis port = s_axis
    int64_t in_data, res;
    ap_int<in_width> Ialpha, Ibeta, Theta, RPM;
    ap_fixed<(QmW_park + QnW_park + 1), (QmW_park + 1)> Ialpha_fixed, Ibeta_fixed, Theta_fixed, RPM_fixed, Id_fixed,
        Iq_fixed;
    ap_int<out_width> Id, Iq;

    // Decode Input stream
    in_data = s_axis.read();                            // Read one value from AXI4-Stream
    Ialpha = ap_int<in_width>(in_data & 0xFFFF);        // Extract Ialpha - bits[15..0] from input stream
    Ibeta = ap_int<in_width>((in_data >> 16) & 0xFFFF); // Extract Ibeta - bits[32..16] from input stream
    RPM = ap_int<in_width>((in_data >> 32) & 0xFFFF);   // Extract RPM - bits[47..32] from input stream
    Theta = ap_int<in_width>((in_data >> 48) & 0xFFFF); // Extract Angle - bits[63..48] from input stream

    // XF_MOTORCONTROL_HW_ASSERT(QmW_park + QnW_park + 1 <= in_width);
    ap_int<sincos_width> cos_theta_in = cos_table[Theta];
    ap_int<sincos_width> sin_theta_in = sin_table[Theta];

    // estimate for resource
    // Ialpha_fixed = Ialpha;
    // Ibeta_fixed = Ibeta;
    // Q4.Q9 convert to ap_fixed
    Ialpha_fixed.range(QmW_park + QnW_park, QnW_park) = Ialpha >> QnW_park;
    if (QnW_park > 0) Ialpha_fixed.range(QnW_park - 1, 0) = Ialpha.range(QnW_park - 1, 0);
    Ibeta_fixed.range(QmW_park + QnW_park, QnW_park) = Ibeta >> QnW_park;
    if (QnW_park > 0) Ibeta_fixed.range(QnW_park - 1, 0) = Ibeta.range(QnW_park - 1, 0);

    RPM_fixed = RPM;
    Theta_fixed = Theta;

    Park_Direct_Qmn<QmW_park, QnW_park, 31, -31, ap_int<sincos_width>, scale_park>(
        Id_fixed, Iq_fixed, Ialpha_fixed, Ibeta_fixed, cos_theta_in, sin_theta_in);
    if (QmW_park + QnW_park + 1 <= out_width) {
        Id.range(QmW_park + QnW_park, 0) = Id_fixed.range(QmW_park + QnW_park, 0);
        Iq.range(QmW_park + QnW_park, 0) = Iq_fixed.range(QmW_park + QnW_park, 0);
    } else {
        Id = Id_fixed.to_int();
        Iq = Iq_fixed.to_int();
    }

    // Write output stream
    res = (((int64_t)Theta << 48) & 0xFFFF000000000000) | // Put Angle bits[63:48]
          (((int64_t)RPM << 32) & 0x0000FFFF00000000) |   // Put RPM bits[47:32]
          (((int64_t)Iq << 16) & 0x00000000FFFF0000) |    // Put Iq bits[31:16]
          ((int64_t)Id & 0x000000000000FFFF);             // Put Id bits[15:0]
    m_axis.write(res);                                    // Write result to the output stream
}
