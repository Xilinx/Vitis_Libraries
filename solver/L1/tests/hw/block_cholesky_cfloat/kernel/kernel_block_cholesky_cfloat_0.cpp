/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
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

#include "kernel_block_cholesky_cfloat.hpp"

#include "block_cholesky_cfloat.hpp"

#include <cstdint>

extern "C" void kernel_block_cholesky_cfloat_0(hls::stream<MATRIX_IN_T>& matrixAStrm,
                                               hls::stream<MATRIX_OUT_T>& matrixLStrm) {
#pragma HLS INTERFACE axis port = matrixAStrm
#pragma HLS INTERFACE axis port = matrixLStrm
#pragma HLS INTERFACE s_axilite port = return bundle = control

    MATRIX_IN_T A[DIM][DIM];

read_in:
    for (unsigned r = 0; r < DIM; ++r) {
        for (unsigned c = 0; c < DIM; ++c) {
#pragma HLS PIPELINE II = 1
            A[r][c] = matrixAStrm.read();
        }
    }

    // MATRIX_IN_T is std::complex<float>, same as xf::solver::xf_block_cholesky_cfloat in block_cholesky_cfloat.hpp.
    xf::solver::xf_block_cholesky_inplace<MATRIX_DIM, MATRIX_BLOCK_B>(A);

write_out:
    for (unsigned r = 0; r < DIM; ++r) {
        for (unsigned c = 0; c < DIM; ++c) {
#pragma HLS PIPELINE II = 1
            matrixLStrm.write(A[r][c]);
        }
    }
}
