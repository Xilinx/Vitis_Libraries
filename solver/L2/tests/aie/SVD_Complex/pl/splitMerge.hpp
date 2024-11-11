/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#include "config.h"

template <int W>
void axiu2fifo(hls::stream<ap_axiu<W, 0, 0, 0> >& i_strm, hls::stream<ap_uint<W> >& fifo_strm, int data_cnt) {
    int cnt = data_cnt;
AXIU_TO_FIFO_LOOP:
    while (cnt--) {
#pragma HLS pipeline II = 1 style = flp
        ap_axiu<W, 0, 0, 0> in = i_strm.read();
        fifo_strm.write(in.data);
    }
}

void split_core(int data_cnt,
                hls::stream<ap_uint<WDATA> >& o_axis_strm,
                hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& to_aie_strm0,
                hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& to_aie_strm1) {
    int cnt = data_cnt;
    ap_axiu<WDATA / 2, 0, 0, 0> dataInstrm0;
    ap_axiu<WDATA / 2, 0, 0, 0> dataInstrm1;
    while (cnt--) {
#pragma HLS pipeline II = 1 style = flp
        ap_uint<WDATA> dataIn = o_axis_strm.read();
        ap_uint<WDATA / 2> dataIn0 = dataIn(WDATA / 2 - 1, 0);
        ap_uint<WDATA / 2> dataIn1 = dataIn(WDATA - 1, WDATA / 2);
        dataInstrm0.data = dataIn0;
        dataInstrm1.data = dataIn1;
        to_aie_strm0.write(dataInstrm0);
        to_aie_strm1.write(dataInstrm1);
    }
}

void split(int DATANUM,
           int LOOP,
           hls::stream<ap_axiu<WDATA, 0, 0, 0> >& o_axis_strm,
           hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& to_aie_strm0,
           hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& to_aie_strm1) {
#pragma HLS dataflow
    const int cnt = DATANUM * LOOP;
    hls::stream<ap_uint<WDATA>, 32> axiu_strm;

    axiu2fifo<WDATA>(o_axis_strm, axiu_strm, cnt);
    split_core(cnt, axiu_strm, to_aie_strm0, to_aie_strm1);
}

void merge_core(int data_cnt,
                hls::stream<ap_axiu<WDATA, 0, 0, 0> >& i_axis_strm,
                hls::stream<ap_uint<WDATA / 2> >& from_aie_strm0,
                hls::stream<ap_uint<WDATA / 2> >& from_aie_strm1) {
    int cnt = data_cnt;
    ap_axiu<WDATA, 0, 0, 0> dataOutstrm;
    while (cnt > 0) {
#pragma HLS pipeline II = 1
        if (!from_aie_strm0.empty() && !from_aie_strm1.empty()) {
            ap_uint<WDATA / 2> dataOut0 = from_aie_strm0.read();
            ap_uint<WDATA / 2> dataOut1 = from_aie_strm1.read();
            ap_uint<WDATA> dataOut = (static_cast<ap_uint<WDATA> >(dataOut0) << 64) | dataOut1;

            dataOutstrm.data = dataOut;
            i_axis_strm.write(dataOutstrm);
            cnt--;
        }
    }
}

void merge(int DATANUM,
           int LOOP,
           hls::stream<ap_axiu<WDATA, 0, 0, 0> >& i_axis_strm,
           hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& from_aie_strm0,
           hls::stream<ap_axiu<WDATA / 2, 0, 0, 0> >& from_aie_strm1) {
#pragma HLS dataflow
    const int cnt = DATANUM * LOOP;
    hls::stream<ap_uint<WDATA / 2>, 32> axiu_strm0;
    hls::stream<ap_uint<WDATA / 2>, 32> axiu_strm1;

    axiu2fifo<WDATA / 2>(from_aie_strm0, axiu_strm0, cnt);
    axiu2fifo<WDATA / 2>(from_aie_strm1, axiu_strm1, cnt);

    merge_core(cnt, i_axis_strm, axiu_strm0, axiu_strm1);
}
