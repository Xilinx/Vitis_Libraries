/*
 * Copyright 2021 Xilinx, Inc.
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

#include "kernel_cholesky.hpp"
#include "xf_solver_L1.hpp"

struct my_cholesky_traits : xf::solver::choleskyTraits<LOWER_TRIANGULAR, DIM, MATRIX_IN_T, MATRIX_OUT_T> {
    static const int ARCH = SEL_ARCH;
};

extern int kernel_cholesky_0(hls::stream<MATRIX_IN_T>& matrixAStrm, hls::stream<MATRIX_OUT_T>& matrixLStrm) {
    int ret;
    ret = xf::solver::cholesky<LOWER_TRIANGULAR, DIM, MATRIX_IN_T, MATRIX_OUT_T>(matrixAStrm, matrixLStrm);
    return ret;
}
