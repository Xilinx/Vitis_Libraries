/*
 * Copyright (C) 2023, Advanced Micro Devices, Inc.
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

#include <stdint.h>
#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"

void axiu2fifo(hls::stream<ap_axiu<64, 0, 0, 0> >& i_strm, hls::stream<ap_uint<64> >& fifo_strm, int data_cnt) {
    int cnt = data_cnt;
AXIU_TO_FIFO_LOOP:
    while (cnt--) {
#pragma HLS pipeline II = 1 style = flp
        ap_axiu<64, 0, 0, 0> in = i_strm.read();
        fifo_strm.write(in.data);
    }
}

void fifo2axiu(hls::stream<ap_uint<64> >& fifo_strm, hls::stream<ap_axiu<64, 0, 0, 0> >& o_strm, int data_cnt) {
    int cnt = data_cnt;
FIFO_TO_AXIU_LOOP:
    while (cnt--) {
#pragma HLS pipeline II = 1 style = flp
        ap_axiu<64, 0, 0, 0> tmp;
        tmp.data = fifo_strm.read();
        tmp.keep = -1;
        tmp.last = 0;
        o_strm.write(tmp);
    }
}

extern "C" void s2s_bypass(hls::stream<ap_axiu<64, 0, 0, 0> >& i_axis_strm0,
                           hls::stream<ap_axiu<64, 0, 0, 0> >& o_axis_strm0,
                           int data_cnt0) {
#pragma HLS interface axis port = i_axis_strm0
#pragma HLS interface axis port = o_axis_strm0
#pragma HLS interface s_axilite bundle = control port = data_cnt0
#pragma HLS interface s_axilite bundle = control port = return
#pragma HLS dataflow

    hls::stream<ap_uint<64>, 64> fifo_strm0;

    axiu2fifo(i_axis_strm0, fifo_strm0, data_cnt0);
    fifo2axiu(fifo_strm0, o_axis_strm0, data_cnt0);
}
