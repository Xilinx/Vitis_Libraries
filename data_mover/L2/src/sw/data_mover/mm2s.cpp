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
#include "xf_data_mover/load_master_to_stream.hpp"

extern "C" void mm2s(
    // 0
    ap_uint<32>* DataIn1,
    hls::stream<ap_axiu<32, 0, 0, 0> >& s,
    uint64_t sz0

    ) {
    using namespace xf::data_mover;

    ; // clang-format off
#pragma HLS interface m_axi offset=slave bundle=gmem0 port=DataIn1 \
    max_read_burst_length=32 num_read_outstanding=4 latency=128
#pragma HLS interface s_axilite bundle=control port=DataIn1
#pragma HLS interface axis port=s
#pragma HLS interface s_axilite bundle=control port=sz0

#pragma HLS interface s_axilite bundle=control port=return
    ; // clang-format on

#pragma HLS dataflow

    loadMasterToStream(DataIn1, s, sz0);
}
