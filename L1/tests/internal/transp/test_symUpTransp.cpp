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
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include "ap_int.h"
#include "hls_stream.h"
#include "xf_blas.hpp"

using namespace xf::linear_algebra::blas;

void uut_transpMat(unsigned int p_blocks,
                   hls::stream<WideType<BLAS_dataType, BLAS_parEntries> >& p_in,
                   hls::stream<WideType<BLAS_dataType, BLAS_parEntries> >& p_out) {
#pragma HLS DATA_PACK variable = p_in
#pragma HLS DATA_PACK variable = p_out

    // transpSymUpMatBlocks<BLAS_dataType, BLAS_parEntries>(p_blocks, p_in, p_out);
    // transpMatBlocks<BLAS_dataType, BLAS_parEntries>(p_blocks, p_in, p_out);
    transpSymLoMatBlocks<BLAS_dataType, BLAS_parEntries>(p_blocks, p_in, p_out);
}

int main() {
    unsigned int l_blocks = 4;
    hls::stream<WideType<BLAS_dataType, BLAS_parEntries> > l_in;
    hls::stream<WideType<BLAS_dataType, BLAS_parEntries> > l_out;

    for (unsigned int b = 0; b < l_blocks; ++b) {
        WideType<BLAS_dataType, BLAS_parEntries> l_val;
        for (unsigned int i = 0; i < BLAS_parEntries; ++i) {
            for (unsigned int j = 0; j < BLAS_parEntries; ++j) {
                l_val[j] = b * BLAS_parEntries * BLAS_parEntries + i * BLAS_parEntries + j;
            }
            l_in.write(l_val);
        }
    }

    uut_transpMat(l_blocks, l_in, l_out);

    for (unsigned int b = 0; b < l_blocks; ++b) {
        for (unsigned int i = 0; i < BLAS_parEntries; ++i) {
            WideType<BLAS_dataType, BLAS_parEntries> l_val;
            l_val = l_out.read();
            std::cout << l_val << std::endl;
        }
        std::cout << std::endl;
    }
    return 0;
}
