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

/**
 * @file cholesky_cfloat.hpp
 * @brief This file contains cholesky functions which deals with cfloat data type
 *   - cholesky_cfloat:     Entry point function
 */

#ifndef _XF_SOLVER_CHOLESKY_CFLOAT_HPP_
#define _XF_SOLVER_CHOLESKY_CFLOAT_HPP_

#include "ap_fixed.h"
#include "hls_x_complex.h"
#include <complex>
#include "utils/std_complex_utils.h"
#include "utils/x_matrix_utils.hpp"
#include "hls_stream.h"

namespace xf {
namespace solver {

// ===================================================================================================================
// Helper functions

// Square root
// o Overloaded versions of the sqrt function
// o The square root of a complex number is expensive.  However, the diagonal values of a Cholesky decomposition are
// always real so we don't need a full complex square root.
template <typename T_IN, typename T_OUT>
int cholesky_sqrt_op(T_IN a, T_OUT& b) {
Function_cholesky_sqrt_op_real:;
    const T_IN ZERO = 0;
    if (a < ZERO) {
        b = ZERO;
        return (1);
    }
    b = x_sqrt(a);
    return (0);
}
template <typename T_IN, typename T_OUT>
int cholesky_sqrt_op(hls::x_complex<T_IN> din, hls::x_complex<T_OUT>& dout) {
Function_cholesky_sqrt_op_complex:;
    const T_IN ZERO = 0;
    T_IN a = din.real();
    dout.imag(ZERO);

    if (a < ZERO) {
        dout.real(ZERO);
        return (1);
    }

    dout.real(x_sqrt(a));
    return (0);
}
template <typename T_IN, typename T_OUT>
int cholesky_sqrt_op(std::complex<T_IN> din, std::complex<T_OUT>& dout) {
Function_cholesky_sqrt_op_complex:;
    const T_IN ZERO = 0;
    T_IN a = din.real();
    dout.imag(ZERO);

    if (a < ZERO) {
        dout.real(ZERO);
        return (1);
    }

    dout.real(x_sqrt(a));
    return (0);
}

// ===================================================================================================================
template <int DIM, class InputType, class OutputType>
void cholesky_cfloat_core(InputType A[DIM][DIM]) {
    for (int j = 0; j < DIM; j++) {

        // Calculate the diagonal value for this column
        InputType ajj = A[j][j];
        InputType ljj = {0.0, 0.0};
        float invDiag=1;
        for(int i=0; i<j; i++) {
            A[i][j].real(0.0);
            A[i][j].imag(0.0);
        }
        cholesky_sqrt_op(ajj, ljj);
        invDiag = 1 / ljj.real();
        A[j][j] = ljj;

    // Calculate the off diagonal values for this column
    off_diag_loop:
        for (int i = j+ 1; i < DIM; i++) {
#pragma HLS PIPELINE II = 1
            InputType aij = A[i][j];
            OutputType lij = aij * invDiag;
            A[i][j] = lij;
        }

    update_rest_outer_loop:
        for (int k = j+ 1; k < DIM; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 128
            InputType akj = A[k][j];
        update_rest_zeros_loop:
        update_rest_inner_loop:
            for (int i = k; i < DIM; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 128
                InputType aij = A[i][j];
                InputType aik = A[i][k];
                OutputType lik = aik - aij * hls::x_conj(akj);
                A[i][k] = lik;
            }
        }
    }
}

/**
* @brief cholesky_cfloat
*
* @tparam LowerTriangularL   Defines the output matrix is lower triangular or upper tirangular, When false generates upper triangle
* @tparam DIM                Defines the input Hermitian/symmetric positive matrix dimensions
* @tparam InputType          Input data type
* @tparam OutputType         Output data type
*
* @param matrixAStrm         Stream of Hermitian/symmetric positive definite input matrix
* @param matrixLStrm         Stream of Lower or upper triangular output matrix, defalut is lower triangular.
*
*/
template <bool LowerTriangularL, int DIM, class InputType, class OutputType>
void cholesky_cfloat(hls::stream<InputType>& matrixAStrm, hls::stream<OutputType>& matrixLStrm) {
    InputType matA[DIM][DIM];

    for (int r = 0; r < DIM; r++) {
#pragma HLS PIPELINE
        for(int c=0; c<DIM; c++){
            matrixAStrm.read(matA[r][c]);
        }
    }

    cholesky_cfloat_core<DIM, InputType, OutputType>(matA);

    for(int c=0; c<DIM; c++){
#pragma HLS PIPELINE
        for (int r = 0; r < DIM; r++) {
            matrixLStrm.write(matA[r][c]);
        }
    }
}

} // end namespace solver
} // end namespace xf
#endif
