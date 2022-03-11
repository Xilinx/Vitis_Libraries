/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#include "pairhmm.hpp"

extern "C" {
void pairhmm(ap_uint<512>* input_data, float* output_data, int numRead, int numHap) {
#pragma HLS INTERFACE m_axi port = input_data offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = output_data offset = slave bundle = gmem
#pragma HLS INTERFACE s_axilite port = input_data
#pragma HLS INTERFACE s_axilite port = output_data
#pragma HLS INTERFACE s_axilite port = numRead
#pragma HLS INTERFACE s_axilite port = numHap
#pragma HLS INTERFACE ap_ctrl_chain port = return
    xf::genomics::pmm_core<SLR_PE_NUM / (READ_BLOCK_SIZE * HAP_BLOCK_SIZE)>(input_data, numRead, numHap, output_data);
}
}
