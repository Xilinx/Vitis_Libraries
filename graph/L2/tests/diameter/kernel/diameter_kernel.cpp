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

#include "diameter_kernel.hpp"
#include <queue>
#include <stdlib.h>
#include <limits>
#include <ctime>
#include <hls_stream.h>
#include <cfloat>
#include <ap_int.h>

extern "C" void diameter_kernel(unsigned numVert,
                                unsigned numEdge,
                                unsigned* offset,
                                unsigned* column,
                                float* weight,
                                float* max_dist,
                                unsigned* src,
                                unsigned* des) {
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_read_outstanding = 32 max_read_burst_length = 8 bundle = \
    gmem0_0 port = column
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_read_outstanding = 32 max_read_burst_length = 8 bundle = \
    gmem0_1 port = offset
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_read_outstanding = 32 max_read_burst_length = 8 bundle = \
    gmem0_2 port = weight

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 64 max_write_burst_length = 2 bundle = \
    gmem1_0 port = max_dist
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 64 max_write_burst_length = 2 bundle = \
    gmem1_1 port = src
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 64 max_write_burst_length = 2 bundle = \
    gmem1_2 port = des

#pragma HLS INTERFACE s_axilite port = numVert bundle = control
#pragma HLS INTERFACE s_axilite port = numEdge bundle = control
#pragma HLS INTERFACE s_axilite port = column bundle = control
#pragma HLS INTERFACE s_axilite port = offset bundle = control
#pragma HLS INTERFACE s_axilite port = weight bundle = control
#pragma HLS INTERFACE s_axilite port = max_dist bundle = control
#pragma HLS INTERFACE s_axilite port = src bundle = control
#pragma HLS INTERFACE s_axilite port = des bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    xf::graph::estimated_diameter(numVert, numEdge, offset, column, weight, max_dist, src, des);
}
