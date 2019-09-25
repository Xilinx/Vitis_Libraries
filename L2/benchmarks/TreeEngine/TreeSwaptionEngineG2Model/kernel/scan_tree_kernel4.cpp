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

#include "tree_engine_kernel.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

extern "C" void scanTreeKernel4(int len, ScanInputParam0 inputParam0[1], ScanInputParam1 inputParam1[1], DT NPV[N]) {
#ifndef HLS_TEST
#pragma HLS INTERFACE m_axi port = inputParam0 latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem0 offset = slave
#pragma HLS INTERFACE m_axi port = inputParam1 latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem1 offset = slave
#pragma HLS INTERFACE m_axi port = NPV latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem2 offset = slave

#pragma HLS INTERFACE s_axilite port = len bundle = control
#pragma HLS INTERFACE s_axilite port = inputParam0 bundle = control
#pragma HLS INTERFACE s_axilite port = inputParam1 bundle = control
#pragma HLS INTERFACE s_axilite port = NPV bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#pragma HLS data_pack variable = inputParam0
#pragma HLS data_pack variable = inputParam1

#endif

    scanTreeKernels(len, inputParam0, inputParam1, NPV);
}
