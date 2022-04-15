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
 * @file cholesky_inverse.hpp
 * @brief This file contains Cholesky Inverse functions
 * choleskyInverse: Entry point function
 */

#ifndef _XF_SOLVER_CHOLESKY_INVERSE_HPP_
#define _XF_SOLVER_CHOLESKY_INVERSE_HPP_

#include "ap_fixed.h"
#include "hls_x_complex.h"
#include "utils/std_complex_utils.h"
#include "utils/x_matrix_utils.hpp"
#include "hls_stream.h"
#include <complex>

#include "cholesky.hpp"
#include "back_substitute.hpp"
#include "matrix_multiply.hpp"

namespace xf {
namespace solver {

// ===================================================================================================================
// Default traits struct defining the internal variable types for the Cholesky Inverse function
template <int RowsColsA, typename InputType, typename OutputType>
struct choleskyInverseTraits {
    typedef InputType CHOLESKY_OUT;
    typedef choleskyTraits<false, RowsColsA, InputType, InputType> CHOLESKY_TRAITS;
    typedef InputType BACK_SUBSTITUTE_OUT;
    typedef backSubstituteTraits<RowsColsA, InputType, InputType> BACK_SUBSTITUTE_TRAITS;
    typedef matrixMultiplyTraits<NoTranspose,
                                 ConjugateTranspose,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 InputType,
                                 OutputType>
        MATRIX_MULTIPLY_TRAITS;
};

// Specialization for ap_fixed
template <int RowsColsA,
          int W1,
          int I1,
          ap_q_mode Q1,
          ap_o_mode O1,
          int N1,
          int W2,
          int I2,
          ap_q_mode Q2,
          ap_o_mode O2,
          int N2>
struct choleskyInverseTraits<RowsColsA, ap_fixed<W1, I1, Q1, O1, N1>, ap_fixed<W2, I2, Q2, O2, N2> > {
    // Cholesky decomposition output precision
    static const int CholeskyOutputW = W1;
    static const int CholeskyOutputI = I1;
    static const ap_q_mode CholeskyOutputQ = Q1;
    static const ap_o_mode CholeskyOutputO = O1;
    static const int CholeskyOutputN = N1;
    typedef ap_fixed<CholeskyOutputW, CholeskyOutputI, CholeskyOutputQ, CholeskyOutputO, CholeskyOutputN> CHOLESKY_OUT;
    typedef choleskyTraits<false, RowsColsA, ap_fixed<W1, I1, Q1, O1, N1>, CHOLESKY_OUT> CHOLESKY_TRAITS;
    // Back substitution output precision
    static const int BackSubstitutionOutW = W2;
    static const int BackSubstitutionOutI = I2;
    static const ap_q_mode BackSubstitutionOutQ = Q2;
    static const ap_o_mode BackSubstitutionOutO = O2;
    static const int BackSubstitutionOutN = N2;
    typedef ap_fixed<BackSubstitutionOutW,
                     BackSubstitutionOutI,
                     BackSubstitutionOutQ,
                     BackSubstitutionOutO,
                     BackSubstitutionOutN>
        BACK_SUBSTITUTE_OUT;
    typedef backSubstituteTraits<RowsColsA, CHOLESKY_OUT, BACK_SUBSTITUTE_OUT> BACK_SUBSTITUTE_TRAITS;
    typedef matrixMultiplyTraits<NoTranspose,
                                 ConjugateTranspose,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 BACK_SUBSTITUTE_OUT,
                                 ap_fixed<W2, I2, Q2, O2, N2> >
        MATRIX_MULTIPLY_TRAITS;
};

// Further specialization for hls::complex<ap_fixed>
template <int RowsColsA,
          int W1,
          int I1,
          ap_q_mode Q1,
          ap_o_mode O1,
          int N1,
          int W2,
          int I2,
          ap_q_mode Q2,
          ap_o_mode O2,
          int N2>
struct choleskyInverseTraits<RowsColsA,
                             hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                             hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
    // Cholesky decomposition output precision
    static const int CholeskyOutputW = W1;
    static const int CholeskyOutputI = I1;
    static const ap_q_mode CholeskyOutputQ = Q1;
    static const ap_o_mode CholeskyOutputO = O1;
    static const int CholeskyOutputN = N1;
    typedef hls::x_complex<
        ap_fixed<CholeskyOutputW, CholeskyOutputI, CholeskyOutputQ, CholeskyOutputO, CholeskyOutputN> >
        CHOLESKY_OUT;
    typedef choleskyTraits<false, RowsColsA, hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> >, CHOLESKY_OUT>
        CHOLESKY_TRAITS;
    // Back substitution output precision
    static const int BackSubstitutionOutW = W2;
    static const int BackSubstitutionOutI = I2;
    static const ap_q_mode BackSubstitutionOutQ = Q2;
    static const ap_o_mode BackSubstitutionOutO = O2;
    static const int BackSubstitutionOutN = N2;
    typedef hls::x_complex<ap_fixed<BackSubstitutionOutW,
                                    BackSubstitutionOutI,
                                    BackSubstitutionOutQ,
                                    BackSubstitutionOutO,
                                    BackSubstitutionOutN> >
        BACK_SUBSTITUTE_OUT;
    typedef backSubstituteTraits<RowsColsA, CHOLESKY_OUT, BACK_SUBSTITUTE_OUT> BACK_SUBSTITUTE_TRAITS;
    typedef matrixMultiplyTraits<NoTranspose,
                                 ConjugateTranspose,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 BACK_SUBSTITUTE_OUT,
                                 hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > >
        MATRIX_MULTIPLY_TRAITS;
};

// Further specialization for std::complex<ap_fixed>
template <int RowsColsA,
          int W1,
          int I1,
          ap_q_mode Q1,
          ap_o_mode O1,
          int N1,
          int W2,
          int I2,
          ap_q_mode Q2,
          ap_o_mode O2,
          int N2>
struct choleskyInverseTraits<RowsColsA,
                             std::complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                             std::complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
    // Cholesky decomposition output precision
    static const int CholeskyOutputW = W1;
    static const int CholeskyOutputI = I1;
    static const ap_q_mode CholeskyOutputQ = Q1;
    static const ap_o_mode CholeskyOutputO = O1;
    static const int CholeskyOutputN = N1;
    typedef std::complex<ap_fixed<CholeskyOutputW, CholeskyOutputI, CholeskyOutputQ, CholeskyOutputO, CholeskyOutputN> >
        CHOLESKY_OUT;
    typedef choleskyTraits<false, RowsColsA, std::complex<ap_fixed<W1, I1, Q1, O1, N1> >, CHOLESKY_OUT> CHOLESKY_TRAITS;
    // Back substitution output precision
    static const int BackSubstitutionOutW = W2;
    static const int BackSubstitutionOutI = I2;
    static const ap_q_mode BackSubstitutionOutQ = Q2;
    static const ap_o_mode BackSubstitutionOutO = O2;
    static const int BackSubstitutionOutN = N2;
    typedef std::complex<ap_fixed<BackSubstitutionOutW,
                                  BackSubstitutionOutI,
                                  BackSubstitutionOutQ,
                                  BackSubstitutionOutO,
                                  BackSubstitutionOutN> >
        BACK_SUBSTITUTE_OUT;
    typedef backSubstituteTraits<RowsColsA, CHOLESKY_OUT, BACK_SUBSTITUTE_OUT> BACK_SUBSTITUTE_TRAITS;
    typedef matrixMultiplyTraits<NoTranspose,
                                 ConjugateTranspose,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 BACK_SUBSTITUTE_OUT,
                                 std::complex<ap_fixed<W2, I2, Q2, O2, N2> > >
        MATRIX_MULTIPLY_TRAITS;
};

/**
* @brief CHOLESKY_INVERSE
* @tparam RowsColsA              Defines the matrix dimensions
* @tparam InputType              Input data type
* @tparam OutputType             Output data type
* @tparam CholeskyInverseTraits  Traits class
*
* @param matrixAStrm             Stream of Square Hermitian/symmetric positive definite input matrix
* @param matrixInverseAStrm      Stream of Inverse of input matrix
* @param cholesky_success        Indicates if matrix A was successfully inverted. 0 = Success. 1 = Failure.
*/
template <int RowsColsA,
          typename InputType,
          typename OutputType,
          typename CholeskyInverseTraits = choleskyInverseTraits<RowsColsA, InputType, OutputType> >
void choleskyInverse(hls::stream<InputType>& matrixAStrm,
                     hls::stream<OutputType>& matrixInverseAStrm,
                     int& cholesky_success) {
#pragma HLS DATAFLOW
    hls::stream<typename CholeskyInverseTraits::CHOLESKY_OUT> matrixUStrm;
#pragma HLS STREAM variable = matrixUStrm depth = 16
    hls::stream<typename CholeskyInverseTraits::BACK_SUBSTITUTE_OUT> matrixInverseUStrm;
#pragma HLS STREAM variable = matrixInverseUStrm depth = 16
    int U_singular;

    // Run Cholesky, get upper-triangular result
    const bool LOWER_TRIANGULAR = false;
    cholesky_success = cholesky<LOWER_TRIANGULAR, RowsColsA, InputType, typename CholeskyInverseTraits::CHOLESKY_OUT,
                                typename CholeskyInverseTraits::CHOLESKY_TRAITS>(matrixAStrm, matrixUStrm);

    // Run back-substitution to compute U^-1
    // This doesn't work in-place, so use an additional array InverseU
    backSubstitute<RowsColsA, typename CholeskyInverseTraits::CHOLESKY_OUT,
                   typename CholeskyInverseTraits::BACK_SUBSTITUTE_OUT,
                   typename CholeskyInverseTraits::BACK_SUBSTITUTE_TRAITS>(matrixUStrm, matrixInverseUStrm, U_singular);
    // A^-1 = U^-1*U^-t (equivalent to L-t*L-1)
    matrixMultiply<NoTranspose, ConjugateTranspose, RowsColsA, RowsColsA, RowsColsA, RowsColsA,
                   typename CholeskyInverseTraits::BACK_SUBSTITUTE_OUT, OutputType,
                   typename CholeskyInverseTraits::MATRIX_MULTIPLY_TRAITS>(matrixInverseUStrm, matrixInverseAStrm);
}

} // end namespace solver
} // end namespace xf

#endif
