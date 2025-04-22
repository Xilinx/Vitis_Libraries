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
* @brief Clarke_Inverse Transform to map 2 phase frame into 3 phase
* @param s_axis    Formatted Input data
* @param m_axis    Formatted output data
* @param logger_stream_in    Formatted input data of the logger
* @param logger_stream_out    Formatted output data of the logger
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */

template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM>
void Clarke_Inverse(hls::stream<T_FOC>& s_axis,
                    hls::stream<T_FOC>& m_axis,
                    hls::stream<T_STREAM_LOG>& logger_stream_in,
                    hls::stream<T_STREAM_LOG>& logger_stream_out) {
#pragma HLS INTERFACE mode = axis port = s_axis
#pragma HLS INTERFACE mode = axis port = m_axis
#pragma HLS INTERFACE mode = axis port = logger_stream_in
#pragma HLS INTERFACE mode = axis port = logger_stream_out

#pragma HLS INTERFACE mode = ap_ctrl_none port = return

    T_FOC in_data, res;
    T_STREAM_LOG log_packet;
    WIDTH_DATA Valpha, Vbeta, Theta;
    WIDTH_ACCUM s3vb_, Va_, Vb_, Vc_; // Clarke Inverse
    WIDTH_DATA Va, Vb, Vc, s3vb;      // Clarke Inverse -> SVPWM
    const WIDTH_DATA SQRT3C = 113512;
    const WIDTH_DATA HALF = 32767;
    const WIDTH_DATA MIN_LIM = -16777216;
    const WIDTH_DATA MAX_LIM = 1677216;

//#pragma HLS INLINE off
#pragma HLS PIPELINE II = 1

    // Decode Input stream
    log_packet = logger_stream_in.read();
    in_data = s_axis.read(); // Read one value from AXI4-Stream
    Vbeta.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM);
    Valpha.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2);
    Theta.range(BIT_WIDTH_DATA - 1, 0) = in_data.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3);

    // Process data
    Va.range(BIT_WIDTH_DATA - 1, 0) = Valpha.range(BIT_WIDTH_DATA - 1, 0);
    s3vb_ = Vbeta * SQRT3C; // (sqrt(3)*(2^16))*Vbeta
    s3vb = s3vb_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Vb_ = (s3vb - Va) * HALF;  // (-Valpha + sqrt(3)*Vbeta)/2
    Vc_ = (-Va - s3vb) * HALF; // (-Valpha - sqrt(3)*Vbeta)/2
    Vb = Vb_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Vc = Vc_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);

    Va = (Va > MAX_LIM) ? MAX_LIM : Va; // Clip max
    Va = (Va < MIN_LIM) ? MIN_LIM : Va; // Clip min
    Vb = (Vb > MAX_LIM) ? MAX_LIM : Vb; // Clip max
    Vb = (Vb < MIN_LIM) ? MIN_LIM : Vb; // Clip min
    Vc = (Vc > MAX_LIM) ? MAX_LIM : Vc; // Clip max
    Vc = (Vc < MIN_LIM) ? MIN_LIM : Vc; // Clip min

    log_packet.range(MAX_VA_LOGGER, MIN_VA_LOGGER) = Va.range(BIT_WIDTH_DATA - 1, 0);
    log_packet.range(MAX_VB_LOGGER, MIN_VB_LOGGER) = Vb.range(BIT_WIDTH_DATA - 1, 0);
    log_packet.range(MAX_VC_LOGGER, MIN_VC_LOGGER) = Vc.range(BIT_WIDTH_DATA - 1, 0);

    // Write output stream
    res.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3) = Theta.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = Va.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) = Vb.range(BIT_WIDTH_DATA - 1, 0);
    res.range(BIT_WIDTH_STEP_STREAM - 1, 0) = Vc.range(BIT_WIDTH_DATA - 1, 0);
    m_axis.write(res); // Write result to the output stream
    logger_stream_out.write(log_packet);
}

} // namespace motorcontrol
} // namespace xf