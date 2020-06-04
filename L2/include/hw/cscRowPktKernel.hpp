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
#ifndef XF_SPARSE_CSCROWPKT_HPP
#define XF_SPARSE_CSCROWPKT_HPP
/**
 * @file cscRowPktKernel.hpp
 * @brief cscRowPktKernel definition.
 *
 * This file is part of Vitis SPARSE Library.
 */

#include "cscKernel.hpp"

typedef ap_axiu<SPARSE_dataBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parDataPktType;

/**
 * @brief cscRowPkt Kernel
 * @param p_aNnzIdx the device memory pointer for read the NNZ values and row indices
 * @param p_memBlocks the number of device memory accesses to read the NNZ values androw indices
 * @param p_nnzBlocks the number of parallel NNZ entries
 * @param p_rowBlocks the number of parallel row vector entries
 * @param in the input axi stream of column vector entries selected for the NNZs
 * @param out the output axi stream of result row vector entries
 */
extern "C" void cscRowPktKernel(const ap_uint<SPARSE_hbmMemBits>* p_aNnzIdx,
                                const unsigned int p_memBlocks,
                                const unsigned int p_nnzBlocks,
                                const unsigned int p_rowBlocks,
                                hls::stream<SPARSE_parDataPktType>& in,
                                hls::stream<SPARSE_parDataPktType>& out);
#endif
