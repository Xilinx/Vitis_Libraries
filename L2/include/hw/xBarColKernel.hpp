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
#ifndef XF_SPARSE_XBARCOL_HPP
#define XF_SPARSE_XBARCOL_HPP
/**
 * @file xBarColKernel.hpp
 * @brief xBarColKernel definition.
 *
 * This file is part of Vitis SPARSE Library.
 */

#include "cscKernel.hpp"

typedef ap_axiu<SPARSE_dataBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parDataPktType;
typedef ap_axiu<SPARSE_indexBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parIndexPktType;

/**
 * @brief xBarCol Kernel
 * @param p_colPtrBlocks number of parallel column pointer entries in the axi stream input in2
 * @param p_nnzBlocks number of parallel NNZ entries in the input axi stream in1 and output axi stream out
 * @param in1 input axi stream of parallel column vector entries
 * @param in2 input axi stream of parallel column pointer entries
 * @param out output axi stream of parallel column vector entries selected for the NNZs
 */
extern "C" void xBarColKernel(const unsigned int p_colPtrBlocks,
                              const unsigned int p_nnzBlocks,
                              hls::stream<SPARSE_parDataPktType>& in1,
                              hls::stream<SPARSE_parIndexPktType>& in2,
                              hls::stream<SPARSE_parDataPktType>& out);
#endif
