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
 * @file xBarColKernel.cpp
 * @brief xBarColKernel definition.
 *
 * This file is part of Vitis SPARSE Library.
 */

#include "cscKernel.hpp"

typedef ap_axiu<SPARSE_dataBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parDataPktType;
typedef ap_axiu<SPARSE_indexBits * SPARSE_parEntries, 0, 0, 0> SPARSE_parIndexPktType;

extern "C" {

void xBarColKernel(const unsigned int p_colPtrBlocks,
                   const unsigned int p_nnzBlocks,
                   hls::stream<SPARSE_parDataPktType>& in1,
                   hls::stream<SPARSE_parIndexPktType>& in2,
                   hls::stream<SPARSE_parDataPktType>& out) {
#pragma HLS INTERFACE axis port = in1
#pragma HLS INTERFACE axis port = in2
#pragma HLS INTERFACE axis port = out
#pragma HLS INTERFACE s_axilite port = p_colPtrBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = p_nnzBlocks bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    xf::sparse::xBarColPkt<SPARSE_logParEntries, SPARSE_dataType, SPARSE_indexType, SPARSE_dataBits, SPARSE_indexBits,
                           SPARSE_parDataPktType, SPARSE_parIndexPktType>(p_colPtrBlocks, p_nnzBlocks, in1, in2, out);
}
}
