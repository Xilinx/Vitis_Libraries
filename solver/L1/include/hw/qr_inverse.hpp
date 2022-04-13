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
 * @file qr_inverse.hpp
 * @brief This file contains implentation of QR Inverse functions
 *   - qrInverse: Entry point function
 */

#ifndef _XF_SOLVER_QR_INVERSE_HPP_
#define _XF_SOLVER_QR_INVERSE_HPP_

#include "hls_x_complex.h"
#include "utils/x_matrix_utils.hpp"
#include "utils/std_complex_utils.h"
#include "qrf.hpp"
#include "back_substitute.hpp"
#include "matrix_multiply.hpp"
#include "hls_stream.h"

namespace xf {
namespace solver {

template <int RowsColsA, typename InputType, typename OutputType>
struct qrInverseTraits {
    typedef float InternalType;
    // typedef qrfTraits<RowsColsA, RowsColsA, InputType, InternalType> QRF_CONFIG;
    typedef qrfTraits QRF_CONFIG;
    typedef backSubstituteTraits<RowsColsA, InternalType, InternalType> BACK_SUB_CONFIG;
    typedef matrixMultiplyTraits<NoTranspose,
                                 NoTranspose,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 InternalType,
                                 OutputType>
        MULTIPLIER_CONFIG;
};

template <int RowsColsA, typename InputType, typename OutputBaseType>
struct qrInverseTraits<RowsColsA, InputType, hls::x_complex<OutputBaseType> > {
    typedef hls::x_complex<float> InternalType;
    // typedef qrfTraits<RowsColsA, RowsColsA, InputType, InternalType> QRF_CONFIG;
    typedef qrfTraits QRF_CONFIG;
    typedef backSubstituteTraits<RowsColsA, InternalType, InternalType> BACK_SUB_CONFIG;
    typedef matrixMultiplyTraits<NoTranspose,
                                 NoTranspose,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 InternalType,
                                 hls::x_complex<OutputBaseType> >
        MULTIPLIER_CONFIG;
};

template <int RowsColsA, typename InputType, typename OutputBaseType>
struct qrInverseTraits<RowsColsA, InputType, std::complex<OutputBaseType> > {
    typedef std::complex<float> InternalType;
    // typedef qrfTraits<RowsColsA, RowsColsA, InputType, InternalType> QRF_CONFIG;
    typedef qrfTraits QRF_CONFIG;
    typedef backSubstituteTraits<RowsColsA, InternalType, InternalType> BACK_SUB_CONFIG;
    typedef matrixMultiplyTraits<NoTranspose,
                                 NoTranspose,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 RowsColsA,
                                 InternalType,
                                 hls::x_complex<OutputBaseType> >
        MULTIPLIER_CONFIG;
};

/**
 * @brief qrInverse the entry point function.
 *
 * @tparam RowsColsA                 Defines the matrix dimensions
 * @tparam InputType                 Input data type
 * @tparam OutputType                Output data type
 * @tparam QRInverseTraits           QRInverse Traits class
 *
 * @param matrixAStrm                Stream of Input matrix A
 * @param matrixInverseAStrm         Stream of Inverse of input matrix
 * @param A_singular                 Failure, matrix A is singular
 */
template <int RowsColsA,
          typename InputType,
          typename OutputType,
          typename QRInverseTraits = qrInverseTraits<RowsColsA, InputType, OutputType> >
void qrInverse(hls::stream<InputType>& matrixAStrm, hls::stream<OutputType>& matrixInverseAStrm, int& A_singular) {
#pragma HLS DATAFLOW
    // Define intermediate buffers
    hls::stream<typename QRInverseTraits::InternalType> matrixQStrm;
#pragma HLS STREAM variable = matrixQStrm depth = 16
    hls::stream<typename QRInverseTraits::InternalType> matrixRStrm;
#pragma HLS STREAM variable = matrixRStrm depth = 16
    hls::stream<typename QRInverseTraits::InternalType> matrixInverseRStrm;
#pragma HLS STREAM variable = matrixInverseRStrm depth = 16

    // Run QR factorization, get upper-triangular result in R, orthogonal/unitary matrix Q
    const bool TRANSPOSED_Q = true; // Q is produced in transpose form such that Q*A = R
    qrf<TRANSPOSED_Q, RowsColsA, RowsColsA, InputType, typename QRInverseTraits::InternalType,
        typename QRInverseTraits::QRF_CONFIG>(matrixAStrm, matrixQStrm, matrixRStrm);

    // Run back-substitution to compute R^-1
    // This doesn't work in-place, so use an additional array InverseR
    backSubstitute<RowsColsA, typename QRInverseTraits::InternalType, typename QRInverseTraits::InternalType,
                   typename QRInverseTraits::BACK_SUB_CONFIG>(matrixRStrm, matrixInverseRStrm, A_singular);

    // A^-1 = R^-1*Qt
    matrixMultiply<NoTranspose, NoTranspose, RowsColsA, RowsColsA, RowsColsA, RowsColsA, RowsColsA, RowsColsA,
                   typename QRInverseTraits::InternalType, OutputType, typename QRInverseTraits::MULTIPLIER_CONFIG>(
        matrixInverseRStrm, matrixQStrm, matrixInverseAStrm);
}
} // end namespace solver
} // end namespace xf

#endif
