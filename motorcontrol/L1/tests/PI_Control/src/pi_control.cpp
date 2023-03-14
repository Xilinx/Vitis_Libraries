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
/*!
 * \file pi_control.cpp
 */

#include "pi_control.h"
#include "pid_control.hpp"

// hls namespace has to be included in all HLS C source files.
using namespace hls;

//--------------------------------------------------------------------------
// PI_Control
// pi = err + GiE_prev;
// p = 1/256 * (kp*err + ki/256 * pi)
//--------------------------------------------------------------------------
void PI_Control_axi(
    hls::stream<int16_t>& s_axis, hls::stream<int16_t>& m_axis, int16_t Sp, int16_t Kp, int16_t Ki, int32_t mode) {
#pragma HLS interface axis port = m_axis
#pragma HLS interface axis port = s_axis
    ap_int<out_width> Res_out;
    ap_int<in_width> sp, kp, ki, in;
    static ap_int<mid_width> GiE_prev;
    bool mode_change = mode;

    int16_t in_data = s_axis.read(); // Read one value from AXI4-Stream
    in = in_data;
    sp = Sp;
    kp = Kp;
    ki = Ki;
    // limit_ap = limit;

    PI_Control_T_AP<in_width, mid_width, out_width, max_limit, min_limit, kp_scale>(Res_out, GiE_prev, in, sp, kp, ki,
                                                                                    mode_change);

    // Write output stream
    m_axis.write(Res_out); // Write result to the output stream
}

void PID_Control_axi(hls::stream<int16_t>& s_axis,
                     hls::stream<int16_t>& m_axis,
                     int16_t Sp,
                     int16_t Kp,
                     int16_t Ki,
                     int16_t Kd,
                     int32_t mode) {
#pragma HLS interface axis port = m_axis
#pragma HLS interface axis port = s_axis
    ap_int<out_width> Res_out;
    ap_int<in_width> sp, kp, ki, kd, in;
    bool mode_change = mode;

    static ap_int<mid_width> GiE_prev, Err_prev;

    int16_t in_data = s_axis.read(); // Read one value from AXI4-Stream
    in = in_data;
    sp = Sp;
    kp = Kp;
    ki = Ki;
    kd = Kd;

    PID_Control_T_AP<in_width, mid_width, out_width, max_limit, min_limit, kp_scale>(Res_out, GiE_prev, Err_prev, in,
                                                                                     sp, kp, ki, kd, mode_change);

    // Write output stream
    m_axis.write(Res_out); // Write result to the output stream
}
