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

#include "kernel_pagerank.hpp"

#ifdef KERNEL0
extern "C" void kernel_pagerank_0(int nrows,
                                  int nnz,
                                  DT alpha,
                                  DT tolerance,
                                  int maxIter,
                                  buffType* pagerank,
                                  buffType* degreeCSR,
                                  buffType* offsetCSC,
                                  buffType* indiceCSC,
                                  buffType* cntValFull,
                                  buffType* buffPing,
                                  buffType* buffPong,
                                  ap_uint<widthOr>* orderUnroll) {
    const int depthRow = depNrow;
    const int depthRow0 = depNrow0;
    const int depthEdge = depNNZ;
    const int depthDgr = depDegree;
    const int depthOr = depOrder;
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = pagerank latency = 125 num_read_outstanding = \
    32 max_read_burst_length = 32 num_write_outstanding = 32 max_write_burst_length = 32 depth = depthRow0
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem1 port = degreeCSR latency = 32 num_read_outstanding = \
    32 max_read_burst_length = 2 num_write_outstanding = 32 max_write_burst_length = 2 depth = depthDgr
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = offsetCSC latency = 125 num_read_outstanding = \
    32 max_read_burst_length = 32 num_write_outstanding = 32 max_write_burst_length = 32 depth = depthRow
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem3 port = indiceCSC latency = 125 num_read_outstanding = \
    32 max_read_burst_length = 32 depth = depthEdge
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem4 port = cntValFull latency = 125 num_read_outstanding = \
    32 max_read_burst_length = 2 num_write_outstanding = 2 max_write_burst_length = 32 depth = depthRow0
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem5 port = buffPing latency = 125 num_read_outstanding = \
    32 max_read_burst_length = 2 num_write_outstanding = 2 max_write_burst_length = 32 depth = depthRow0
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem6 port = buffPong latency = 125 num_read_outstanding = \
    32 max_read_burst_length = 2 num_write_outstanding = 2 max_write_burst_length = 32 depth = depthRow0
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem7 port = orderUnroll latency = 125 num_read_outstanding = \
    32 max_read_burst_length = 32 num_write_outstanding = 32 max_write_burst_length = 32 depth = depthOr
#pragma HLS INTERFACE s_axilite port = degreeCSR bundle = control
#pragma HLS INTERFACE s_axilite port = indiceCSC bundle = control
#pragma HLS INTERFACE s_axilite port = offsetCSC bundle = control
#pragma HLS INTERFACE s_axilite port = pagerank bundle = control
#pragma HLS INTERFACE s_axilite port = cntValFull bundle = control
#pragma HLS INTERFACE s_axilite port = buffPing bundle = control
#pragma HLS INTERFACE s_axilite port = buffPong bundle = control
#pragma HLS INTERFACE s_axilite port = orderUnroll bundle = control
#pragma HLS INTERFACE s_axilite port = nnz bundle = control
#pragma HLS INTERFACE s_axilite port = nrows bundle = control
#pragma HLS INTERFACE s_axilite port = alpha bundle = control
#pragma HLS INTERFACE s_axilite port = tolerance bundle = control
#pragma HLS INTERFACE s_axilite port = maxIter bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    const int cacheDepthBin = 0;                          // cache line depth in Binary
    const int dataOneLineBin = (sizeof(DT) == 4) ? 4 : 3; // data numbers in Binary of one buffType
    const int dataOneLineBin2 = 4;                        // data numbers in Binary of one buffType
    const int usURAM = 0; // 0 represents use LUTRAM, 1 represents use URAM, 2 represents use BRAM

    xf::graph::pageRankTop<DT, maxVertex, maxEdge, unrollBin, widthOr, cacheDepthBin, dataOneLineBin, dataOneLineBin2,
                           usURAM>(nrows, nnz, pagerank, degreeCSR, offsetCSC, indiceCSC, cntValFull, buffPing,
                                   buffPong, orderUnroll, alpha, tolerance, maxIter);
}
#endif
