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
* @brief angle_generation_inst artificial angle generation to spin the motor in open loop
* @param period    Number of clock cycles which interleaves between an angle increment and another
* @param increment    Increment of the angle (measure in CPR)
* @param output_stream    Stream with output result
* @param T_FOC    data width of the streaming connection with execution data
* @param T_STREAM_LOG    data width of the streaming connection with log data
* @param WIDTH_DATA    width of the single variable
* @param WIDTH_ACCUM    width of the accumulator resulting from multiplication operation
 */

template <typename T_FOC, typename T_STREAM_LOG, typename WIDTH_DATA, typename WIDTH_ACCUM>
void angle_generation(volatile int& period, volatile int& increment, hls::stream<WIDTH_DATA>& output_stream) {
    static WIDTH_DATA gen_delay = 0;
    static WIDTH_DATA gen_angle = 0;

#pragma HLS PIPELINE II = 1

    if (gen_delay >= period) { // Period loop
        gen_delay = 0;
        if (gen_angle >= (COMM_MACRO_TLB_LENTH - 1)) { // Angle loop
            gen_angle = 0;
        } else {
            gen_angle += increment;
        }
    } else {
        ++gen_delay;
    }

    output_stream.write(gen_angle);
}

} // namespace motorcontrol
} // namespace xf