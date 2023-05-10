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

#ifndef DATAMOVER_STORE_STREAM_TO_MASTER_HPP
#define DATAMOVER_STORE_STREAM_TO_MASTER_HPP

#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"

namespace xf {
namespace data_mover {

/**
 * Read from AXI stream, and send data to AXI master.
 *
 * It is expected that the size is aligned with (WS/8) and (WM/8),
 * otherwise out-of-boundary data would be read/written.
 *
 * @tparam WM width of AXI master.
 * @tparam WS width of AXI stream.
 * @tparam BLEN burst length when writing to AXI master.
 *
 * @param mm master port.
 * @param s stream port.
 * @param size size of data to be stored, in byte.
 */
template <int WM, int WS, int BLEN = 32>
void storeStreamToMaster(hls::stream<ap_axiu<WS, 0, 0, 0> >& s, ap_uint<WM>* mm, uint64_t size) {
#ifndef __SYNTHESIS__
    static_assert(WM == WS, "WM should be equal to WS in the current implementation");
#endif
    const int bytePerData = WM / 8;
    const int nBlks = size / bytePerData + ((size % bytePerData) > 0);
    const int nBurst = nBlks / BLEN;
    int cnt = 0;
    for (int i = 0; i < nBurst; i++) {
        for (int j = 0; j < BLEN; j++) {
#pragma HLS pipeline II = 1
            ap_axiu<WS, 0, 0, 0> tmp = s.read();
            mm[cnt] = tmp.data;
            cnt++;
        }
    }
    int leftBlks = nBlks % BLEN;
    if (leftBlks > 0) {
        for (int i = 0; i < leftBlks; i++) {
#pragma HLS pipeline II = 1
            ap_axiu<WS, 0, 0, 0> tmp = s.read();
            mm[cnt] = tmp.data;
            cnt++;
        }
    }
}

/**
 * Read from AXI stream, and send data to AXI master.
 *
 * It is expected that the size is aligned with (WS/8) and (WM/8),
 * otherwise out-of-boundary data would be read/written.
 *
 * @tparam WM width of AXI master.
 * @tparam WS width of AXI stream.
 * @tparam BLEN burst length when writing to AXI master.
 *
 * @param mm master port.
 * @param s stream port.
 * @param size size of data to be stored, in byte.
 * @param vld_str stream port, to record the start and end position of sending data to AXI master.
 * @param cntData count data number that have been sent to AXI master, in byte.
 */
template <int WM, int WS, int BLEN = 32>
void storeStreamToMaster(hls::stream<ap_axiu<WS, 0, 0, 0> >& s,
                         ap_uint<WM>* mm,
                         uint64_t size,
                         hls::stream<ap_uint<1> >& vld_str,
                         uint64_t* cntData) {
#ifndef __SYNTHESIS__
    static_assert(WM == WS, "WM should be equal to WS in the current implementation");
#endif
    const int bytePerData = WM / 8;
    const int nBlks = size / bytePerData + ((size % bytePerData) > 0);
    const int nBurst = nBlks / BLEN;
    int cnt = 0;
    vld_str.write_nb(1);
    for (int i = 0; i < nBurst; i++) {
        for (int j = 0; j < BLEN; j++) {
#pragma HLS pipeline II = 1
            ap_axiu<WS, 0, 0, 0> tmp = s.read();
            mm[cnt] = tmp.data;
            cnt++;
        }
        *cntData = cnt * bytePerData;
    }
    int leftBlks = nBlks % BLEN;
    if (leftBlks > 0) {
        for (int i = 0; i < leftBlks; i++) {
#pragma HLS pipeline II = 1
            ap_axiu<WS, 0, 0, 0> tmp = s.read();
            mm[cnt] = tmp.data;
            cnt++;
        }
    }
    *cntData = cnt * bytePerData;
    vld_str.write_nb(1);
}
} /* data_mover */
} /* xf */
#endif
