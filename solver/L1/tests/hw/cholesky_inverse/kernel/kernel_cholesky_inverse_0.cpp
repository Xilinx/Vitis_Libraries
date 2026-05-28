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

#include "kernel_cholesky_inverse.hpp"
#include "xf_solver_L1.hpp"

typedef xf::solver::choleskyInverseTraits<ROWSCOLSA, MATRIX_IN_T, MATRIX_OUT_T> DEFAULT_CHOL_INV_TRAITS;

struct my_cholesky_inv_traits : DEFAULT_CHOL_INV_TRAITS {
    struct BACK_SUBSTITUTE_TRAITS : xf::solver::backSubstituteTraits<ROWSCOLSA,
                                                                     DEFAULT_CHOL_INV_TRAITS::CHOLESKY_OUT,
                                                                     DEFAULT_CHOL_INV_TRAITS::BACK_SUBSTITUTE_OUT> {
        static const int ARCH = SEL_ARCH;
    };
};

extern "C" int kernel_cholesky_inverse_0(hls::stream<MATRIX_IN_T>& matrixAStrm,
                                         hls::stream<MATRIX_OUT_T>& matrixInverseAStrm) {
    int inverse_OK;
    xf::solver::choleskyInverse<ROWSCOLSA, MATRIX_IN_T, MATRIX_OUT_T, my_cholesky_inv_traits>(
        matrixAStrm, matrixInverseAStrm, inverse_OK);
    return inverse_OK;
}
