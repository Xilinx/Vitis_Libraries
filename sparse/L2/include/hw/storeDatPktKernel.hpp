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
#ifndef XF_SPARSE_STOREDATPKT_HPP
#define XF_SPARSE_STOREDATPKT_HPP
/**
 * @file storeDatKernel.hpp
 * @brief storeDatKernel definition.
 *
 * This file is part of Vitis SPARSE Library.
 */

#include "cscKernel.hpp"

typedef ap_axiu<SPARSE_dataBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parDataPktType;
/**
 * @brief storeDataPkt Kernel
 * @param in the input axi stream of row vector entries of cscmv operation results
 * @param p_memPtr the device memory pointer for writing the row vector entries
 * @param p_memBlocks the number of vector entries in each memory write
 */
extern "C" void storeDatPktKernel(hls::stream<SPARSE_parDataPktType>& in,
                                  ap_uint<SPARSE_ddrMemBits>* p_memPtr,
                                  unsigned int p_memBlocks);
#endif
