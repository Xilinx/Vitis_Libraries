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

#include "strtree_kernel.hpp"
#define NC 16

void strtree_acc::compute(int sz,
                          double* inX,
                          double* inY,
                          double* inZone,
                          PT* extPointBuf0,
                          PT* extPointBuf1,
                          NT* extNodeBuf0,
                          NT* extNodeBuf1,
                          NT* extNodeBuf2) {
    STRTree_Kernel(sz, inX, inY, inZone, extPointBuf0, extPointBuf1, extNodeBuf0, extNodeBuf1, extNodeBuf2);
}

void strtree_acc::STRTree_Kernel(int sz,
                                 double* inX,
                                 double* inY,
                                 double* inZone,
                                 PT* extPointBuf0,
                                 PT* extPointBuf1,
                                 NT* extNodeBuf0,
                                 NT* extNodeBuf1,
                                 NT* extNodeBuf2) {
#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem0_0 port = inX
#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem0_1 port = inY
#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem1 port = inZone

#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem2 port = extPointBuf0
#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 32 max_read_burst_length = 2 bundle = gmem3 port = extPointBuf1

#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 32 max_read_burst_length = 2 bundle = gmem4 port = extNodeBuf0
#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 32 bundle = gmem5 port = extNodeBuf1
#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 32 max_read_burst_length = 2 bundle = gmem6 port = extNodeBuf2

    xf::data_analytics::geospatial::strtreeTop<KT, VT, PT, NT, NC, ISN, BSN, MTCN, MSN>(
        sz, inX, inY, inZone, extPointBuf0, extPointBuf1, extNodeBuf0, extNodeBuf1, extNodeBuf2);
}
