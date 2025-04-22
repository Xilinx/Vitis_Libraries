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
#include <iostream>
#include "common_definitions.hpp"

int main(void) {
    hls::stream<ap_uint<BIT_WIDTH_STREAM_FOC> > input_data_stream, output_data_stream;
    hls::stream<ap_int<BIT_WIDTH_DATA> > input_data_w_1, input_data_w_2, input_data_w_3, input_data_w_4, input_data_w_5;
    hls::stream<ap_int<BIT_WIDTH_DATA> > input_data_w_i, output_data_w_i;
    hls::stream<ap_uint<BIT_WIDTH_LOG_STREAM_FOC> > log_in, log_out;

    ap_uint<BIT_WIDTH_STREAM_FOC> input_values_stream, output_values_stream;
    ap_uint<BIT_WIDTH_LOG_STREAM_FOC> input_log_values, output_log_values;
    ap_int<BIT_WIDTH_DATA> input_values_w_u, output_values_w_u;
    int32_t _data_ag = 0;

    volatile int Sp_ = 5, Kp_ = 2, Ki_ = 1, mode_ = 0;
    input_data_w_1.write(64); // last 8 bit enabled
    input_data_w_2.write(32); // last 8 bit enabled
    PI_Control_stream_Inst(input_data_w_1, input_data_w_2, output_data_w_i, Sp_, Kp_, Ki_, mode_);

    int cnt_error = 1;
    while (!output_data_w_i.empty()) {
        cnt_error = 0;
        std::cout << "VAL: " << output_data_w_i.read() << std::endl;
    }

    return cnt_error;
}
