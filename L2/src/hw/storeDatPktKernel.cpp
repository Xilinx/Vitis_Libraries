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
 * @file storeDatKernel.cpp
 * @brief storeDatKernel definition.
 *
 * This file is part of Vitis SPARSE Library.
 */

#include "cscKernel.hpp"

typedef ap_axiu<SPARSE_dataBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parDataPktType;

extern "C" {

void storeDatPktKernel(hls::stream<SPARSE_parDataPktType>& in,
                       ap_uint<SPARSE_ddrMemBits>* p_memPtr,
                       unsigned int p_memBlocks) {
#pragma HLS INTERFACE m_axi port = p_memPtr offset = slave bundle = gmem
#pragma HLS INTERFACE axis port = in
#pragma HLS INTERFACE s_axilite port = p_memPtr bundle = control
#pragma HLS INTERFACE s_axilite port = p_memBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    xf::sparse::storeDatPkt<SPARSE_parEntries, SPARSE_ddrMemBits, SPARSE_dataBits, SPARSE_parDataPktType>(
        in, p_memBlocks, p_memPtr);
}
}
