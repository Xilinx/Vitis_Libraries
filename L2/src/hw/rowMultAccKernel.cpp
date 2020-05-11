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

#if VITIS_TEST
extern "C" {
#endif

void rowMultAccKernel(const ap_uint<SPARSE_ddrMemBits>* p_memRdPtr,
                      const unsigned int p_memRdBlocks,
                      const ap_uint<SPARSE_hbmMemBits>* p_aNnzIdx,
                      const unsigned int p_nnzBlocks,
                      const unsigned int p_rowBlocks,
                      const unsigned int p_memWrBlocks,
                      ap_uint<SPARSE_ddrMemBits>* p_memWrPtr) {
#pragma HLS INTERFACE m_axi port = p_memRdPtr offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = p_memWrPtr offset = slave bundle = gmem
#pragma HLS INTERFACE m_axi port = p_aNnzIdxPtr offset = slave bundle = hmem
#pragma HLS INTERFACE s_axilite port = p_memRdPtr bundle = control
#pragma HLS INTERFACE s_axilite port = p_memWrPtr bundle = control
#pragma HLS INTERFACE s_axilite port = p_aNnzIdx bundle = control
#pragma HLS INTERFACE s_axilite port = p_memRdBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = p_memWrBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = p_nnzBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = p_rowBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    hls::stream<SPARSE_parDataPktType> l_colDatPktStr;
    hls::stream<SPARSE_parDataPktType> l_rowAggPktStr;

    xf::sparse::loadDat2PktStr<SPARSE_ddrMemBits, SPARSE_parEntries, SPARSE_dataBits, SPARSE_parDataPktType>(
        p_memRdPtr, p_memRdBlocks, l_colDatPktStr);

    xf::sparse::cscRowPkt<SPARSE_maxRowBlocks, SPARSE_logParEntries, SPARSE_logParGroups, SPARSE_dataType,
                          SPARSE_indexType, SPARSE_dataBits, SPARSE_indexBits, SPARSE_hbmMemBits,
                          SPARSE_parDataPktType>(p_aNnzIdx, p_nnzBlocks, p_nnzBlocks, p_rowBlocks, l_colDatPktStr,
                                                 l_rowAggPktStr);

    xf::sparse::storeDatPkt<SPARSE_parEntries, SPARSE_ddrMemBits, SPARSE_dataBits, SPARSE_parDataPktType>(
        l_rowAggPktStr, p_memWrBlocks, p_memWrPtr);
}
#if VITIS_TEST
}
#endif
