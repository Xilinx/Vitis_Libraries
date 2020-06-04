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
#ifndef XF_SPARSE_LOADCOLPTRVALKERNEL_HPP
#define XF_SPARSE_LOADCOLPTRVALKERNEL_HPP

/**
 * @file loadColPtrValKernel.hpp
 * @brief loadColPtrValKernel definition.
 *
 * This file is part of Vitis SPARSE Library.
 */

#include "cscKernel.hpp"

typedef ap_axiu<SPARSE_dataBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parDataPktType;
typedef ap_axiu<SPARSE_indexBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parIndexPktType;

/**
 * @brief loadColPtrVal Kernel
 * @param p_memColVal device memory pointer for reading column vector
 * @param p_memColVal device memory pointer for read column pointers of NNZ entries
 * @param p_memBlocks number of blocks of vector entries in the memory read operation
 * @param p_numTrans number of times to trigger this kernel. Currently only support 1
 * @param out1 the axi stream of output column vector entries
 * @param out2 the axi stream of output column pointer entries
 */

extern "C" void loadColPtrValKernel(const ap_uint<SPARSE_ddrMemBits>* p_memColVal,
                                    const ap_uint<SPARSE_ddrMemBits>* p_memColPtr,
                                    const unsigned int p_memBlocks,
                                    const unsigned int p_numTrans,
                                    hls::stream<SPARSE_parDataPktType>& out1,
                                    hls::stream<SPARSE_parIndexPktType>& out2);

#endif
