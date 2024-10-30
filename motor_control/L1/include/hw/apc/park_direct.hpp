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
#include "sin_cos_table.hpp"

namespace xf {
namespace motorcontrol {

/**
* @brief Park_Direct Transform to map 2 phase rotating frame into 2 phase fixed frame
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
void Park_Direct(hls::stream<T_FOC>& s_axis,
                 hls::stream<T_FOC>& m_axis,
                 hls::stream<T_STREAM_LOG>& logger_stream_in,
                 hls::stream<T_STREAM_LOG>& logger_stream_out) {
    T_FOC in_data, res;
    WIDTH_DATA Ialpha, Ibeta, Theta, RPM;
    WIDTH_DATA Id, Iq, Ia_cos, Ib_sin, Ib_cos, Ia_sin;
    WIDTH_DATA cos_theta, sin_theta;
    ap_uint<32> Theta_Park;
    WIDTH_ACCUM cos_angle_scaled, sin_angle_scaled;
    WIDTH_ACCUM Ia_cos_, Ib_sin_, Ib_cos_, Ia_sin_, Id_, Iq_;

    const WIDTH_DATA MIN_LIM = -16777215;
    const WIDTH_DATA MAX_LIM = 16777215;
    const WIDTH_DATA POLAR_PAIR = 2; // 2
    const WIDTH_DATA CPR = 1000;

    T_STREAM_LOG log;

//#pragma HLS INLINE off
#pragma HLS PIPELINE II = 1

    // Decode Input stream
    log = logger_stream_in.read();
    in_data = s_axis.read(); // Read one value from AXI4-Stream
    // ialpha -> ialpha / ibeta->ibeta
    Ialpha = in_data.range(BIT_WIDTH_STEP_STREAM - 1, 0);
    Ibeta = in_data.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM);
    // ialpha -> ibeta / ibeta->ialpha
    //	Ialpha = in_data.range(63, 32);
    //	Ibeta = in_data.range(31, 0);
    RPM = in_data.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2);
    Theta = in_data.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3);

    Theta_Park = Theta << 1;
    if (Theta_Park >= CPR) {
        Theta_Park = Theta_Park - CPR;
    }
    cos_theta = cos_table[Theta_Park];
    sin_theta = sin_table[Theta_Park];
    Ia_cos_ = Ialpha * cos_theta;
    Ib_sin_ = Ibeta * sin_theta;
    Ib_cos_ = Ibeta * cos_theta;
    Ia_sin_ = Ialpha * sin_theta;
    Ia_cos = Ia_cos_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Ib_sin = Ib_sin_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Ib_cos = Ib_cos_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Ia_sin = Ia_sin_.range(BIT_WIDTH_ACCUM - 1, BIT_WIDTH_FRACTIONAL);
    Id = Ia_cos + Ib_sin;
    Iq = Ib_cos - Ia_sin;
    Id = (Id > MAX_LIM) ? MAX_LIM : Id; // Clip max
    Id = (Id < MIN_LIM) ? MIN_LIM : Id; // Clip min
    Iq = (Iq > MAX_LIM) ? MAX_LIM : Iq; // Clip max
    Iq = (Iq < MIN_LIM) ? MIN_LIM : Iq; // Clip min

    log.range(MAX_IQ_LOGGER, MIN_IQ_LOGGER) = Iq.range(BIT_WIDTH_DATA - 1, 0);
    log.range(MAX_ID_LOGGER, MIN_ID_LOGGER) = Id.range(BIT_WIDTH_DATA - 1, 0);

    // Write output stream
    res.range((BIT_WIDTH_STEP_STREAM * 4) - 1, BIT_WIDTH_STEP_STREAM * 3) = Theta.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 3) - 1, BIT_WIDTH_STEP_STREAM * 2) = RPM.range(BIT_WIDTH_DATA - 1, 0);
    res.range((BIT_WIDTH_STEP_STREAM * 2) - 1, BIT_WIDTH_STEP_STREAM) = Iq.range(BIT_WIDTH_DATA - 1, 0);
    res.range(BIT_WIDTH_STEP_STREAM - 1, 0) = Id.range(BIT_WIDTH_DATA - 1, 0);
    m_axis.write(res); // Write result to the output stream
    logger_stream_out.write(log);
}

} // namespace motorcontrol
} // namespace xf