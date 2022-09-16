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
 * @file kernel_resize.cpp
 *
 * @brief This file contains top function of test case.
 */

#include "kernel_resize.hpp"
#include "bicubicinterpolator.hpp"

extern "C" void kernel_resize(ap_uint<32>* configs, ap_uint<WDATA>* axi_src, ap_uint<WDATA>* axi_dst) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave bundle = gmem0 port = configs latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 64 num_write_outstanding = 64 max_write_burst_length = 32 depth = 5

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem1 port = axi_src latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 64 num_write_outstanding = 64 max_write_burst_length = 32 depth = 128

#pragma HLS INTERFACE m_axi offset = slave bundle = gmem2 port = axi_dst latency = 32 num_read_outstanding = \
    64 max_read_burst_length = 64 num_write_outstanding = 64 max_write_burst_length = 32 depth = 128

#pragma HLS INTERFACE s_axilite port = configs bundle = control
#pragma HLS INTERFACE s_axilite port = axi_src bundle = control
#pragma HLS INTERFACE s_axilite port = axi_dst bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    // clang-format on

    xf::codec::resizeTop(configs, axi_src, axi_dst);
}
