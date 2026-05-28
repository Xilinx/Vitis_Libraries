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

#include "kernel_qrf.hpp"
#include "xf_solver_L1.hpp"
struct my_qrf_traits : xf::solver::qrfTraits {
    static const int ARCH = SEL_ARCH;
};

extern "C" void kernel_qrf_0(hls::stream<MATRIX_IN_T>& matrixAStrm,
                             hls::stream<MATRIX_OUT_T>& matrixQStrm,
                             hls::stream<MATRIX_OUT_T>& matrixRStrm) {
    xf::solver::qrf<TRANSPOSED_Q, A_ROWS, A_COLS, MATRIX_IN_T, MATRIX_OUT_T, my_qrf_traits>(matrixAStrm, matrixQStrm,
                                                                                            matrixRStrm);
}
