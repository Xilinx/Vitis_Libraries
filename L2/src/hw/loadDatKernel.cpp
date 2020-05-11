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
 * @file loadDatKernel.cpp
 * @brief loadDatKernel definition.
 *
 * This file is part of Vitis SPARSE Library.
 */

#include "cscKernel.hpp"

typedef ap_axiu<SPARSE_dataBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parDataPktType;

extern "C" {

void loadDatKernel(const ap_uint<SPARSE_ddrMemBits>* p_memPtr,
                   const unsigned int p_memBlocks,
                   hls::stream<SPARSE_parDataPktType>& out) {
#pragma HLS INTERFACE m_axi port = p_memPtr offset = slave bundle = gmem
#pragma HLS INTERFACE axis port = out
#pragma HLS INTERFACE s_axilite port = p_memPtr bundle = control
#pragma HLS INTERFACE s_axilite port = p_memBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    xf::sparse::loadDat2PktStr<SPARSE_ddrMemBits, SPARSE_parEntries, SPARSE_dataBits, SPARSE_parDataPktType>(
        p_memPtr, p_memBlocks, out);
}
}
