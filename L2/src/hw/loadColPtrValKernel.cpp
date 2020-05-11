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
 * @file loadColPtrValKernel.cpp
 * @brief loadColPtrValKernel definition.
 *
 * This file is part of Vitis SPARSE Library.
 */

#include "cscKernel.hpp"

typedef ap_axiu<SPARSE_dataBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parDataPktType;
typedef ap_axiu<SPARSE_indexBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parIndexPktType;

extern "C" {

void loadColPtrValKernel(const ap_uint<SPARSE_ddrMemBits>* p_memColVal,
                         const ap_uint<SPARSE_ddrMemBits>* p_memColPtr,
                         const unsigned int p_memBlocks,
                         const unsigned int p_numTrans,
                         hls::stream<SPARSE_parDataPktType>& out1,
                         hls::stream<SPARSE_parIndexPktType>& out2) {
#pragma HLS INTERFACE m_axi port = p_memColVal offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = p_memColPtr offset = slave bundle = gmem
#pragma HLS INTERFACE axis port = out1
#pragma HLS INTERFACE axis port = out2
#pragma HLS INTERFACE s_axilite port = p_memColVal bundle = control
#pragma HLS INTERFACE s_axilite port = p_memColPtr bundle = control
#pragma HLS INTERFACE s_axilite port = p_memBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = p_numTrans bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    xf::sparse::loadCol2PktStrStep<SPARSE_maxColMemBlocks, SPARSE_ddrMemBits, SPARSE_parEntries, SPARSE_dataBits,
                                   SPARSE_indexBits, SPARSE_parDataPktType, SPARSE_parIndexPktType>(
        p_memColVal, p_memColPtr, p_memBlocks, p_numTrans, out1, out2);
}
}
