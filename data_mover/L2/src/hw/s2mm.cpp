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

#include "xf_data_mover/s2mm.hpp"

extern "C" void s2mm(hls::stream<ap_axiu<128, 0, 0, 0> >& s0, ap_uint<128>* mm0, uint64_t nbytes0) {
    using namespace xf::data_mover;

#pragma HLS interface axis port = s0
#pragma HLS interface m_axi offset = slave bundle = gmem0 port = mm0 max_read_burst_length = 32 num_read_outstanding = \
    4 latency = 128
#pragma HLS interface s_axilite bundle = control port = mm0
#pragma HLS interface s_axilite bundle = control port = nbytes0
#pragma HLS interface s_axilite bundle = control port = return
#pragma HLS dataflow
    storeStreamToMaster(s0, mm0, nbytes0);
}
