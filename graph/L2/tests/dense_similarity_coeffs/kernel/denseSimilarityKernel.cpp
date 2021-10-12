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

#include "denseSimilarityKernel.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

extern "C" void denseSimilarityKernel(ap_int<32>* config,
                                      ap_int<32>* sourceWeight,
                                      ap_int<32>* sourceCoeffs,

                                      ap_int<32 * CHANNEL_NUMBER>* dataIn00,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn01,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn02,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn03,

                                      ap_int<32 * CHANNEL_NUMBER>* dataIn10,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn11,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn12,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn13,

                                      ap_int<32 * CHANNEL_NUMBER>* dataIn20,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn21,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn22,
                                      ap_int<32 * CHANNEL_NUMBER>* dataIn23,

                                      ap_int<32>* resultID,
                                      float* similarity) {
    const int ext_mem_size = EXT_MEM_SZ;

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem0_0 port = dataIn00 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn00 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem0_1 port = dataIn01 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn01 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem0_2 port = dataIn02 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn02 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem0_3 port = dataIn03 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn03 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem1_0 port = dataIn10 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn10 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem1_1 port = dataIn11 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn11 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem1_2 port = dataIn12 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn12 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem1_3 port = dataIn13 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn13 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem2_0 port = dataIn20 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn20 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem2_1 port = dataIn21 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn21 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem2_2 port = dataIn22 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn22 bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 16 max_read_burst_length = 64 bundle = gmem2_3 port = dataIn23 depth = ext_mem_size
#pragma HLS INTERFACE s_axilite port = dataIn23 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem4_0 port = config depth = 64
#pragma HLS INTERFACE s_axilite port = config bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem4_0 port = sourceWeight depth = 65536
#pragma HLS INTERFACE s_axilite port = sourceWeight bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem4_0 port = sourceCoeffs depth = 65536
#pragma HLS INTERFACE s_axilite port = sourceCoeffs bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem4_0 port = resultID depth = 128
#pragma HLS INTERFACE s_axilite port = resultID bundle = control
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    32 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem4_0 port = similarity depth = 128
#pragma HLS INTERFACE s_axilite port = similarity bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifndef __SYNTHESIS__
    std::cout << "kernel call success" << std::endl;
#endif

#pragma HLS INLINE off

    xf::graph::denseSimilarityCoeffs<CHANNEL_NUMBER, W_DATA, RAM_SIZE, MAX_K>(
        config, sourceWeight, sourceCoeffs, dataIn00, dataIn01, dataIn02, dataIn03, dataIn10, dataIn11, dataIn12,
        dataIn13, dataIn20, dataIn21, dataIn22, dataIn23, resultID, similarity);
}
