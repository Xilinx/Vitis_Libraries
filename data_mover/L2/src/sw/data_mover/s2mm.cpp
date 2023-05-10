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

#include <stdint.h>
#include "xf_data_mover/store_stream_to_master.hpp"

extern "C" void s2mm(
    // 0
    hls::stream<ap_axiu<32, 0, 0, 0> >& s,
    ap_uint<32>* DataOut1,
    uint64_t sz0

    ) {
    using namespace xf::data_mover;

    ; // clang-format off
#pragma HLS interface axis port=s
#pragma HLS interface m_axi offset=slave bundle=gmem0 port=DataOut1 \
    max_write_burst_length=32 num_write_outstanding=4 latency=128
#pragma HLS interface s_axilite bundle=control port=DataOut1
#pragma HLS interface s_axilite bundle=control port=sz0

#pragma HLS interface s_axilite bundle=control port=return
    ; // clang-format on

#pragma HLS dataflow

    storeStreamToMaster(s, DataOut1, sz0);
}
