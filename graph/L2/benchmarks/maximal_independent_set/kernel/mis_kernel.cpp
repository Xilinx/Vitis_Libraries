/*
 * Copyright 2022 Xilinx, Inc.
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
#include "mis_kernel.hpp"

extern "C" void mis_kernel(int m,              // vertex number of current graph, REG: axi-lite
                           const int* offset,  // graph offset, read-only, HBM: Read
                           const int* indices, // graph indices, read-only HBM: Read
                           int* C_group_0,     // status field, C-0, HBM: Read & Write
                           int* C_group_1,     // status field, C-1, HBM: Read & Write
                           int* S_group_0,     // status field, S-0, HBM: Read & Write
                           int* S_group_1,     // status field, S-1, HBM: Read & Write
                           int* res_out        // selected mis result(output), HBM: Write
                           ) {
#pragma HLS INTERFACE mode = m_axi bundle = mm1 depth = M_VERTEX latency = 64 max_read_burst_length = \
    64 max_widen_bitwidth = 64 num_read_outstanding = 64 port = offset
#pragma HLS INTERFACE mode = m_axi bundle = mm2 depth = M_EDGE latency = 64 max_read_burst_length = \
    64 max_widen_bitwidth = 64 num_read_outstanding = 64 port = indices
#pragma HLS INTERFACE mode = m_axi bundle = mm3 depth = M_VERTEX latency = 32 max_read_burst_length =                  \
    32 max_widen_bitwidth = 32 max_write_burst_length = 32 num_read_outstanding = 32 num_write_outstanding = 32 port = \
        C_group_0
#pragma HLS INTERFACE mode = m_axi bundle = mm4 depth = M_VERTEX latency = 32 max_read_burst_length =                  \
    32 max_widen_bitwidth = 32 max_write_burst_length = 32 num_read_outstanding = 32 num_write_outstanding = 32 port = \
        C_group_1
#pragma HLS INTERFACE mode = m_axi bundle = mm5 depth = M_VERTEX latency = 32 max_read_burst_length =                  \
    32 max_widen_bitwidth = 32 max_write_burst_length = 32 num_read_outstanding = 32 num_write_outstanding = 32 port = \
        S_group_0
#pragma HLS INTERFACE mode = m_axi bundle = mm6 depth = M_VERTEX latency = 32 max_read_burst_length =                  \
    32 max_widen_bitwidth = 32 max_write_burst_length = 32 num_read_outstanding = 32 num_write_outstanding = 32 port = \
        S_group_1
#pragma HLS INTERFACE mode = m_axi bundle = mm7 depth = M_VERTEX latency = 32 max_read_burst_length =                  \
    32 max_widen_bitwidth = 32 max_write_burst_length = 32 num_read_outstanding = 32 num_write_outstanding = 32 port = \
        res_out

    xf::graph::mis<MAX_ROUND>(m, offset, indices, C_group_0, C_group_1, S_group_0, S_group_1, res_out);
}
