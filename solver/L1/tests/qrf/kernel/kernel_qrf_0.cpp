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
struct my_qrf_traits : xf::solver::qrf_traits<A_ROWS, A_COLS, MATRIX_IN_T, MATRIX_OUT_T> {
    static const int ARCH = SEL_ARCH;
};

extern "C" void kernel_qrf_0(const MATRIX_IN_T A[A_ROWS][A_COLS],
                             MATRIX_OUT_T Q[A_ROWS][A_ROWS],
                             MATRIX_OUT_T R[A_ROWS][A_COLS]) {
    if (SEL_ARCH == 1) {
        xf::solver::qrf<TRANSPOSED_Q, A_ROWS, A_COLS, MATRIX_IN_T, MATRIX_OUT_T>(A, Q, R);
    } else {
        xf::solver::qrf_top<TRANSPOSED_Q, A_ROWS, A_COLS, my_qrf_traits, MATRIX_IN_T, MATRIX_OUT_T>(A, Q, R);
    }
}
