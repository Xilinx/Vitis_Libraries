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

using namespace xf::blas;

void uut_transpMat(unsigned int p_blocks,
                   hls::stream<WideType<BLAS_dataType, BLAS_memWidth> >& p_in,
                   hls::stream<WideType<BLAS_dataType, BLAS_memWidth> >& p_out) {
#pragma HLS DATA_PACK variable = p_in
#pragma HLS DATA_PACK variable = p_out
    // transpMemWordBlocks<BLAS_dataType, BLAS_memWidth, BLAS_rows, BLAS_cols>(p_blocks, p_in, p_out);
    transpMemBlocks<BLAS_dataType, BLAS_memWidth, BLAS_rows, BLAS_cols>(p_blocks, p_in, p_out);
}

int main() {
    unsigned int l_colWords = BLAS_cols / BLAS_memWidth;
    unsigned int l_size = BLAS_rows * BLAS_cols / BLAS_memWidth;
    hls::stream<WideType<BLAS_dataType, BLAS_memWidth> > l_in;
    hls::stream<WideType<BLAS_dataType, BLAS_memWidth> > l_out;

    unsigned int l_blocks = 5;
    for (unsigned int b = 0; b < l_blocks; ++b) {
        for (unsigned int i = 0; i < BLAS_rows; ++i) {
            for (unsigned int j = 0; j < l_colWords; ++j) {
                WideType<BLAS_dataType, BLAS_memWidth> l_val;
                for (unsigned int k = 0; k < BLAS_memWidth; ++k) {
                    l_val[k] = i * l_colWords * BLAS_memWidth + j * BLAS_memWidth + k;
                }
                l_in.write(l_val);
            }
        }
    }

    uut_transpMat(l_blocks, l_in, l_out);

    for (unsigned int b = 0; b < l_blocks; ++b) {
        for (unsigned int i = 0; i < l_size; ++i) {
            WideType<BLAS_dataType, BLAS_memWidth> l_val;
            l_val = l_out.read();
            std::cout << l_val << std::endl;
        }
        std::cout << std::endl;
    }
    return 0;
}
