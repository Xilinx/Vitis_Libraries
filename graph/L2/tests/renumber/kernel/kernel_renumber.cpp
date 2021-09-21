/*
 * Copyright 2019 Xilinx, Inc.
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
/**
 * @file kernel_renumber.cpp
 *
 * @brief This file contains top function of test case.
 */

#include "kernel_renumber.hpp"

extern "C" void kernel_renumber(int32_t* configs,
                                ap_int<DWIDTHS>* oldCids,
                                ap_int<DWIDTHS>* mapCid0,
                                ap_int<DWIDTHS>* mapCid1,
                                ap_int<DWIDTHS>* newCids) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = configs latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 64 num_write_outstanding = 64 max_write_burst_length = 32 depth = 128

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem1 port = oldCids latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 64 num_write_outstanding = 64 max_write_burst_length = 32 depth = 128

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem2 port = mapCid0 latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 64 num_write_outstanding = 64 max_write_burst_length = 32 depth = 128

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem3 port = mapCid1 latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 64 num_write_outstanding = 64 max_write_burst_length = 32 depth = 128

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem4 port = newCids latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 64 num_write_outstanding = 64 max_write_burst_length = 32 depth = 128

#pragma HLS INTERFACE s_axilite port = configs bundle = control
#pragma HLS INTERFACE s_axilite port = oldCids bundle = control
#pragma HLS INTERFACE s_axilite port = mapCid0 bundle = control
#pragma HLS INTERFACE s_axilite port = mapCid1 bundle = control
#pragma HLS INTERFACE s_axilite port = newCids bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    // clang-format on

    // init map buffer
    xf::graph::internal::initRenumber(configs[0], mapCid0, mapCid1);
    // commit renumber
    xf::graph::renumberCore(configs[0], configs[1], oldCids, mapCid0, mapCid1, newCids);
}
