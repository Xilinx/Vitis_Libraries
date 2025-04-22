/*
Copyright (C) 2024-2024, Advanced Micro Devices, Inc.
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
* @brief SVPWM     Modulate the voltages obtained through FOC
* @param s_axis    Formatted Input data
* @param m_axis    Formatted output data
* @param logger_stream_in    Formatted input data of the logger
* @param logger_stream_out    Formatted output data of the logger
* @param mode_    Set mode of control
* @param phase_a_    Value of A phase in case of manual set
* @param phase_b_    Value of B phase in case of manual set
* @param phase_c_    Value of C phase in case of manual set
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */

template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM>
void SVPWM(hls::stream<T_FOC>& s_axis,
           hls::stream<T_FOC>& m_axis,
           hls::stream<T_STREAM_LOG>& logger_stream_in,
           hls::stream<T_STREAM_LOG>& logger_stream_out,
           volatile int& mode_,
           volatile int& phase_a_,
           volatile int& phase_b_,
           volatile int& phase_c_) {
    T_STREAM_LOG logger_pack;
    T_FOC in_data, res;
    WIDTH_DATA Va, Vb, Vc, Theta;
    WIDTH_DATA Va_abs, Vb_abs, Vc_abs;
    WIDTH_DATA Vmin, Vmin_temp, Voff; // SVPWM internals
    WIDTH_DATA Van, Vbn, Vcn;         // Normalized SVPWM data
    WIDTH_ACCUM Vmin_mod;
    WIDTH_DATA zero_ = 0;

    const WIDTH_DATA MIN_LIM = -16777215;
    const WIDTH_DATA MAX_LIM = 1677215;
    const WIDTH_DATA HALF = 32768;

    bool is_a_min = false, is_b_min = false, is_c_min = false;

    // Decode Input stream
    logger_pack = logger_stream_in.read();
    in_data = s_axis.read(); // Read one value from AXI4-Stream
    Vc.range(BIT_WIDTH_DATA - 1, 0) = in_data.range(31, 0);
    Vb.range(BIT_WIDTH_DATA - 1, 0) = in_data.range(63, 32);
    Va.range(BIT_WIDTH_DATA - 1, 0) = in_data.range(95, 64);
    Theta.range(BIT_WIDTH_DATA - 1, 0) = in_data.range(127, 96);

    Van = (Va > MAX_LIM) ? MAX_LIM : Va; // Clip max
    Van = (Va < MIN_LIM) ? MIN_LIM : Va; // Clip min
    Vbn = (Vb > MAX_LIM) ? MAX_LIM : Vb; // Clip max
    Vbn = (Vb < MIN_LIM) ? MIN_LIM : Vb; // Clip min
    Vcn = (Vc > MAX_LIM) ? MAX_LIM : Vc; // Clip max
    Vcn = (Vc < MIN_LIM) ? MIN_LIM : Vc; // Clip min

    if (mode_ == FOC_Mode::MOD_MANUAL_PWM) {
        Van = phase_a_;
        Vbn = phase_b_;
        Vcn = phase_c_;
    }

    res.range(BIT_WIDTH_STEP_STREAM - 1, 0) = Vcn.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) = Vbn.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = Van.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3) = Theta.range(BIT_WIDTH_DATA - 1, 0);

    m_axis.write(res);

    logger_stream_out.write(logger_pack);
}

} // namespace motorcontrol
} // namespace xf