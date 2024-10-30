/*
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/
#pragma once
#include "common_vars.hpp"

namespace xf {
namespace motorcontrol {

/**
* @brief PI_Control Proportional and Integrative Control
* @param s_axis    Formatted Input data
* @param m_axis    Formatted output data
* @param Sp    Setpoint of the controller
* @param Kp    Gain of proportional
* @param Ki    Gain of Integrative
  @param mode    mode of control, used to reset in case of change
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */

template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM>
void PI_Control(hls::stream<WIDTH_DATA>& s_axis,
                hls::stream<WIDTH_DATA>& m_axis,
                WIDTH_DATA Sp,
                WIDTH_DATA Kp,
                WIDTH_DATA Ki,
                WIDTH_DATA mode) {
//#pragma HLS INTERFACE mode=axis port=s_axis
//#pragma HLS INTERFACE mode=axis port=m_axis
//
//#pragma HLS INTERFACE mode=ap_none port=Sp
//#pragma HLS INTERFACE mode=ap_none port=Kp
//#pragma HLS INTERFACE mode=ap_none port=Ki
//#pragma HLS INTERFACE mode=ap_none port=mode
//
//#pragma HLS INTERFACE mode=ap_ctrl_none port=return

//#pragma HLS INLINE off
#pragma HLS PIPELINE II = 1

    WIDTH_DATA Err;
    WIDTH_ACCUM GpE, GiE;
    WIDTH_DATA Res_Out, P, I;
    WIDTH_DATA in_data;

    const WIDTH_DATA MAX_LIM = 16777215;
    const WIDTH_DATA MIN_LIM = -16777215;

    WIDTH_DATA GiE_prev = 0;
    WIDTH_DATA Mode_prev = 0;
    WIDTH_DATA IErr = 0;

    in_data = s_axis.read();

    // Proportional
    Err = Sp - in_data;
    // Integral
    IErr += Err;
    // Saturation
    IErr = (IErr > MAX_LIM) ? MAX_LIM : IErr;
    IErr = (IErr < MIN_LIM) ? MIN_LIM : IErr;
    // If controller different, tracked error useless
    if (mode != Mode_prev) {
        IErr = 0;
    }
    // Computes control coefficients
    GpE = Kp * Err;
    GiE = Ki * IErr;
    P = GpE.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    I = GiE.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);

    // PI
    Res_Out = P + I;

    Mode_prev = mode;
    // Write output stream
    m_axis.write(Res_Out); // Write result to the output stream
}

/**
* @brief PI_Control Proportional and Integrative Control
* @param s_axis    Formatted Input data for mode MOD_TORQUE_WITHOUT_SPEED
* @param s_axis_2  Formatted Input data for other modes
* @param m_axis    Formatted output data
* @param Sp    Setpoint of the controller
* @param Kp    Gain of proportional
* @param Ki    Gain of Integrative
  @param mode    mode of control, used to reset in case of change
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */
template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM>
void PI_Control_stream(hls::stream<WIDTH_DATA>& s_axis,
                       hls::stream<WIDTH_DATA>& s_axis_2,
                       hls::stream<WIDTH_DATA>& m_axis,
                       WIDTH_DATA Sp,
                       WIDTH_DATA Kp,
                       WIDTH_DATA Ki,
                       WIDTH_DATA mode) {
#pragma HLS INTERFACE mode = axis port = s_axis
#pragma HLS INTERFACE mode = axis port = s_axis_2
#pragma HLS INTERFACE mode = axis port = m_axis

#pragma HLS INTERFACE mode = ap_none port = Sp
#pragma HLS INTERFACE mode = ap_none port = Kp
#pragma HLS INTERFACE mode = ap_none port = Ki
#pragma HLS INTERFACE mode = ap_none port = mode

#pragma HLS INTERFACE mode = ap_ctrl_none port = return

//#pragma HLS INLINE off
#pragma HLS PIPELINE II = 1

    WIDTH_DATA Err;
    WIDTH_ACCUM GpE, GiE;
    WIDTH_DATA Res_Out, P, I;
    WIDTH_DATA in_data, in_data_2, pi_ref;

    const WIDTH_DATA MAX_LIM = 16777215;
    const WIDTH_DATA MIN_LIM = -16777215;

    WIDTH_DATA GiE_prev = 0;
    WIDTH_DATA Mode_prev = 0;
    WIDTH_DATA IErr = 0;

    in_data = s_axis.read(); // Read one value from AXI4-Stream
    in_data_2 = s_axis_2.read();

    if (mode == FOC_Mode::MOD_TORQUE_WITHOUT_SPEED) {
        pi_ref = in_data;
    } else {
        pi_ref = in_data_2;
    }

    // Proportional
    Err = Sp - pi_ref;
    // Integral
    IErr += Err;
    // Saturation
    IErr = (IErr > MAX_LIM) ? MAX_LIM : IErr;
    IErr = (IErr < MIN_LIM) ? MIN_LIM : IErr;
    // If controller different, tracked error useless
    if (mode != Mode_prev) {
        IErr = 0;
    }
    // Computes control coefficients
    GpE = Kp * Err;
    GiE = Ki * IErr;
    P = GpE.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    I = GiE.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);

    // PI
    Res_Out = P + I;

    Mode_prev = mode;
    // Write output stream
    m_axis.write(Res_Out); // Write result to the output stream
}

} // namespace motorcontrol
} // namespace xf