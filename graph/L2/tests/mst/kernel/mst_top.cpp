/*
 * Copyright 2021 Xilinx, Inc.
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

#include "mst_top.hpp"

extern "C" void mst_top(unsigned int numVert,
                        unsigned int numEdge,
                        unsigned int source,
                        unsigned int* offset,
                        unsigned int* column,
                        float* weight,
                        unsigned* mst) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 2 max_read_burst_length = 8 bundle = gmem0 port = offset
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem1 port = column
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem2 port = weight
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 2 max_read_burst_length = 8 bundle = gmem3 port = mst

// clang-format on
#pragma HLS INTERFACE s_axilite port = numVert bundle = control
#pragma HLS INTERFACE s_axilite port = numEdge bundle = control
#pragma HLS INTERFACE s_axilite port = source bundle = control
#pragma HLS INTERFACE s_axilite port = offset bundle = control
#pragma HLS INTERFACE s_axilite port = column bundle = control
#pragma HLS INTERFACE s_axilite port = weight bundle = control
#pragma HLS INTERFACE s_axilite port = mst bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    xf::graph::mst(numVert, numEdge, source, offset, column, weight, mst);
}
