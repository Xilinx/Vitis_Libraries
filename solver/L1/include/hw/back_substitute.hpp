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
 * @file back_substitute.hpp
 * @brief This file contains back substitute functions
 *   - backSubstitute      : Entry point function
 *   - backSubstituteTop   : Top level function that selects implementation architecture and internal types based on a
 * traits class
 *   - backSubstituteBasic : Implementation requiring lower resources
 *   - backSubstituteAlt   : Re-arranged loops to improve dependencies and throughput with increased resources
 */

#ifndef _XF_SOLVER_BACK_SUBSTITUTE_HPP_
#define _XF_SOLVER_BACK_SUBSTITUTE_HPP_

#include "ap_fixed.h"
#include "hls_x_complex.h"
#include "hls_stream.h"
#include <complex>

namespace xf {
namespace solver {

// ===================================================================================================================
// Default traits struct defining the internal variable types for the Back Substitution function
template <int RowsColsA, typename InputType, typename OutputType>
struct backSubstituteTraits {
    typedef InputType RECIP_T;
    typedef InputType MULT_T;
    typedef InputType ADD_T;
    typedef InputType MULTNEG_T;
    static const int ARCH = 1;     // Select implementation. 0=Basic. 1=Improved throughput.
    static const int INNER_II = 1; // Specify the pipelining target for the main inner loop
    static const int DIAG_II = 1;  // Specify the pipelining target for the diag loop in backSubstituteAlt
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
struct backSubstituteTraits<RowsColsA, ap_fixed<W1, I1, Q1, O1, N1>, ap_fixed<W2, I2, Q2, O2, N2> > {
    static const int W =
        W1 + (W1 - I1) + (W1 - I1);      // Fractional growth is denominator fraction width + numerator full width
    static const int I = I1 + (W1 - I1); // Integer growth is denominator int width + numerator fraction width
    typedef ap_fixed<W, I, AP_TRN, AP_WRAP, 0> RECIP_T;
    typedef ap_fixed<2 * W, 2 * I, AP_TRN, AP_WRAP, 0> MULT_T;
    typedef ap_fixed<(2 * W) + 1, (2 * I) + 1, AP_TRN, AP_WRAP, 0> ADD_T;
    typedef ap_fixed<2 * W2, 2 * I2, AP_TRN, AP_WRAP, 0> MULTNEG_T;
    static const int ARCH = 1;
    static const int INNER_II = 1;
    static const int DIAG_II = 1;
};

// Further specialization for hls::x_complex<ap_fixed>
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
struct backSubstituteTraits<RowsColsA,
                            hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                            hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
    static const int W =
        W1 + (W1 - I1) + (W1 - I1);      // Fractional growth is denominator fraction width + numerator full width
    static const int I = I1 + (W1 - I1); // Integer growth is denominator int width + numerator fraction width
    typedef hls::x_complex<ap_fixed<W, I, AP_TRN, AP_WRAP, 0> > RECIP_T;
    typedef hls::x_complex<ap_fixed<(2 * W) + 1, (2 * I) + 1, AP_TRN, AP_WRAP, 0> > MULT_T;
    typedef hls::x_complex<ap_fixed<(2 * W) + 2, (2 * I) + 2, AP_TRN, AP_WRAP, 0> > ADD_T;
    typedef hls::x_complex<ap_fixed<(2 * W2) + 1, (2 * I2) + 1, AP_TRN, AP_WRAP, 0> > MULTNEG_T;
    static const int ARCH = 1;
    static const int INNER_II = 1;
    static const int DIAG_II = 1;
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
struct backSubstituteTraits<RowsColsA,
                            std::complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                            std::complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
    static const int W =
        W1 + (W1 - I1) + (W1 - I1);      // Fractional growth is denominator fraction width + numerator full width
    static const int I = I1 + (W1 - I1); // Integer growth is denominator int width + numerator fraction width
    typedef std::complex<ap_fixed<W, I, AP_TRN, AP_WRAP, 0> > RECIP_T;
    typedef std::complex<ap_fixed<(2 * W) + 1, (2 * I) + 1, AP_TRN, AP_WRAP, 0> > MULT_T;
    typedef std::complex<ap_fixed<(2 * W) + 2, (2 * I) + 2, AP_TRN, AP_WRAP, 0> > ADD_T;
    typedef std::complex<ap_fixed<(2 * W2) + 1, (2 * I2) + 1, AP_TRN, AP_WRAP, 0> > MULTNEG_T;
    static const int ARCH = 1;
    static const int INNER_II = 1;
    static const int DIAG_II = 1;
};

// ===================================================================================================================
// Helper functions

// IMPLEMENTATION TIP: Force the reciprocal to be implemented using a single-precision divider with the resource
// directive.
// This permits sharing of operators with the factorization function preceding the back-substitution
template <typename T>
void back_substitute_recip(T x, T& one_over_x) {
    // #pragma HLS BIND_OP variable=one_over_x  op=fdiv impl=fabric
    const T ONE = 1.0;
    one_over_x = ONE / x;
}

// All diagonal elements of the factorization (Cholesky or QR) should be real.
// We can therefore use a real-valued divider (or reciprocal operator) to compute the diagonal inverse values.
//
// IMPLEMENTATION TIP: Force the reciprocal to be implemented using a single-precision divider with the resource
// directive.
// This permits sharing of operators with the factorization function preceding the back-substitution.
template <typename T>
void back_substitute_recip(hls::x_complex<T> x, hls::x_complex<T>& one_over_x) {
    // #pragma HLS BIND_OP variable=recip  op=fdiv impl=fabric
    T recip; // intermediate variable to allow directive to be applied
    const hls::x_complex<T> ONE = 1.0;
    recip = ONE.real() / x.real();
    one_over_x.real() = recip;
    one_over_x.imag() = 0.0;
}

template <typename T>
void back_substitute_recip(std::complex<T> x, std::complex<T>& one_over_x) {
    // #pragma HLS BIND_OP variable=recip  op=fdiv impl=fabric
    T recip; // intermediate variable to allow directive to be applied
    const std::complex<T> ONE(1.0);
    recip = ONE.real() / x.real();
    one_over_x.real(recip);
    one_over_x.imag(0.0);
}

template <int W1, int I1, ap_q_mode Q1, ap_o_mode O1, int N1, int W2, int I2, ap_q_mode Q2, ap_o_mode O2, int N2>
void back_substitute_recip(ap_fixed<W1, I1, Q1, O1, N1> x, ap_fixed<W2, I2, Q2, O2, N2>& one_over_x) {
    ap_fixed<W2, I2, Q2, O2, N2> ONE = 1; // Size to the output precision
    one_over_x = ONE / x;                 // Infers a divider
}

template <int W1, int I1, ap_q_mode Q1, ap_o_mode O1, int N1, int W2, int I2, ap_q_mode Q2, ap_o_mode O2, int N2>
void back_substitute_recip(hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> > x,
                           hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> >& one_over_x) {
    hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > ONE; // Size to the output precision
    ONE.real() = 1;
    ONE.imag() = 0;
    one_over_x.real() = ONE.real() / x.real(); // Infers a real-valued divider
    one_over_x.imag() = 0;
}

template <int W1, int I1, ap_q_mode Q1, ap_o_mode O1, int N1, int W2, int I2, ap_q_mode Q2, ap_o_mode O2, int N2>
void back_substitute_recip(std::complex<ap_fixed<W1, I1, Q1, O1, N1> > x,
                           std::complex<ap_fixed<W2, I2, Q2, O2, N2> >& one_over_x) {
    std::complex<ap_fixed<W2, I2, Q2, O2, N2> > ONE; // Size to the output precision
    ONE.real(1);
    ONE.imag(0);
    one_over_x.real(ONE.real() / x.real()); // Infers a real-valued divider
    one_over_x.imag(0);
}

// ===================================================================================================================
// backSubstituteBasic
// o It is assumed that argument B is an internal buffer created in the calling function
//   and not an external interface, since the back substitution process reads and writes
//   from/to that buffer.
template <int RowsColsA, typename BackSubstituteTraits, typename InputType, typename OutputType>
void backSubstituteBasic(const InputType A[RowsColsA][RowsColsA],
                         OutputType B[RowsColsA][RowsColsA],
                         int& is_singular) {
    typename BackSubstituteTraits::RECIP_T diag_recip, diag_recip_calc;
    OutputType subst_prod_m1;
    OutputType subst_prod_m2;
    typename BackSubstituteTraits::MULT_T subst_prod;
    typename BackSubstituteTraits::ADD_T subst_prod_sum;
    typename BackSubstituteTraits::ADD_T subst_sum;
    typename BackSubstituteTraits::ADD_T diag_recip_low;
    typename BackSubstituteTraits::MULTNEG_T neg_diag_prod;

    is_singular = 0;
back_substitute_j:
    for (int j = 0; j < RowsColsA; j++) {
        back_substitute_recip(A[j][j], diag_recip);
        B[j][j] = diag_recip;
        if (hls::x_real(diag_recip) == 0 && hls::x_imag(diag_recip) == 0) {
            is_singular = 1;
        }

    back_substitute_i:
        for (int i = 0; i < RowsColsA; i++) {
            if (i >= j) {
                if (i == j) {
                    continue;
                } else {
                    B[i][j] = 0; // Sets the lower triangle to zero for the final matrix mult to work
                }
            } else {
                subst_sum = 0; // Equivalent to setting zeros in the upper triangle of the identity matrix we're
                               // "inverting against"
            back_substitute_k:
                for (int k = 0; k < RowsColsA; k++) {
#pragma HLS PIPELINE II = BackSubstituteTraits::INNER_II

                    if (k >= j) {
                        continue;
                    } else {
                        if (i > k) {
                            continue;
                        } else {
                            subst_prod_m1 = A[k][j]; // For fixed-point, re-size to higher precision of B
                            subst_prod_m2 = B[i][k];
                            subst_prod = subst_prod_m1 * subst_prod_m2;
                            subst_prod_sum = subst_prod; // Resize
                            subst_sum += subst_prod_sum;
                        }
                    }
                    diag_recip_low =
                        diag_recip; // For fixed-point, reduce precision to match subst_sum for multiplication
                    // IMPLEMENTATION TIP: Use last subst_sum value below rather than reading from B to reduce error for
                    // fixed-point implementations.
                    // Implementing as "neg_diag_prod = -B[i][j] * B[j][j];" for fixed-point increases error by ~10%,
                    // but halves
                    // the DSP48 usage.
                    neg_diag_prod = -subst_sum * diag_recip_low;
                    B[i][j] = neg_diag_prod;
                } // end i>=j
            }
        }
    }
} // end template backSubstituteBasic

// ===================================================================================================================
// backSubstituteAlt: Re-arrange loops to improve dependencies and throughput, utilizes additional internal buffers
// for the diagonal and row accumulations. Arguement B is only written to.
template <int RowsColsA, typename BackSubstituteTraits, typename InputType, typename OutputType>
void backSubstituteAlt(const InputType A[RowsColsA][RowsColsA], OutputType B[RowsColsA][RowsColsA], int& is_singular) {
    typename BackSubstituteTraits::RECIP_T diag_recip, diag_recip_calc;
    typename BackSubstituteTraits::RECIP_T diag[RowsColsA];

    typename BackSubstituteTraits::MULT_T subst_prod;
    typename BackSubstituteTraits::ADD_T subst_prod_sum;
    typename BackSubstituteTraits::ADD_T final_sum;
    typename BackSubstituteTraits::ADD_T subst_sum;
    typename BackSubstituteTraits::ADD_T row_sum[RowsColsA][RowsColsA];

    typename BackSubstituteTraits::ADD_T diag_recip_low;
    typename BackSubstituteTraits::MULTNEG_T neg_diag_prod;
    OutputType select_column_multiplier;
    OutputType column_multiplier[RowsColsA];
    OutputType subst_prod_m1;

    is_singular = 0;
diag_loop:
    for (int i = 0; i < RowsColsA; i++) {
#pragma HLS PIPELINE II = BackSubstituteTraits::DIAG_II
        back_substitute_recip(A[i][i], diag_recip_calc);
        if (hls::x_real(diag_recip_calc) == 0 && hls::x_imag(diag_recip_calc) == 0) {
            is_singular = 1;
        }
        diag[i] = diag_recip_calc;
    }
a_col_b_row_loop:
    for (int i = 0; i < RowsColsA; i++) {
        diag_recip = diag[i];
        diag_recip_low = diag_recip; // For fixed-point, reduce precision to match subst_sum for multiplication
    a_row_loop:
        for (int j = 0; j < RowsColsA; j++) {
            if (j >= i) {
            b_col_loop:
                for (int k = 0; k < RowsColsA; k++) {
#pragma HLS PIPELINE II = BackSubstituteTraits::INNER_II

                    // Interleaving column results to relax the dependency on the column_multiplier/result calculation
                    // o As a result we need an array to store the row accumulations
                    if (k <= i) {
                        if (i == j) {
                            // Top of the column
                            if (k == i) {
                                // Just the diagonal
                                select_column_multiplier = diag_recip;
                            } else {
                                final_sum = row_sum[k][j];
                                neg_diag_prod = -final_sum * diag_recip_low;
                                select_column_multiplier = neg_diag_prod;
                            }
                            column_multiplier[k] = select_column_multiplier;
                            B[k][i] = select_column_multiplier; // (B[i][k]) Working with a upper triangular matrix
                        } else {
                            subst_prod_m1 = A[i][j]; // (A[j][i]) Working with a upper triangular matrix
                            subst_prod = subst_prod_m1 * column_multiplier[k];
                            subst_prod_sum = subst_prod; // Resize
                            if (k == i) {
                                // First accumulation in the row sum
                                subst_sum = subst_prod_sum;
                            } else {
                                subst_sum = row_sum[k][j] + subst_prod_sum;
                            }
                            row_sum[k][j] = subst_sum;
                        }
                    } else {
                        B[k][i] = 0; // Zero lower triangle
                    }
                }
            } else {
                continue;
            }
        }
    }
} // end template backSubstituteAlt

// ===================================================================================================================
// backSubstituteTop: Top level function taking a BackSubstituteTraits template parameter which defines internal types
// Call this function directly if you wish to override the default internal types
template <int RowsColsA, typename BackSubstituteTraits, typename InputType, typename OutputType>
void backSubstituteTop(const InputType A[RowsColsA][RowsColsA], OutputType B[RowsColsA][RowsColsA], int& is_singular) {
    switch (BackSubstituteTraits::ARCH) {
        case 0:
            backSubstituteBasic<RowsColsA, BackSubstituteTraits, InputType, OutputType>(A, B, is_singular);
            break;
        case 1:
            backSubstituteAlt<RowsColsA, BackSubstituteTraits, InputType, OutputType>(A, B, is_singular);
            break;
        default:
            backSubstituteBasic<RowsColsA, BackSubstituteTraits, InputType, OutputType>(A, B, is_singular);
            break;
    }
}

/**
 * @brief backSubstitute
 *
 * @tparam RowsColsA    Row and column dimensions
 * @tparam InputType    Input data type
 * @tparam OutputType   Output data type
 * @tparam TRIATS       Traits class
 *
 * @param matrixAStrm   Stream of Input matrix
 * @param matrixBStrm   Stream of Output matrix.
 * @param is_singular   Indicates the diagonal of B contains zeros.
 */
template <int RowsColsA,
          typename InputType,
          typename OutputType,
          typename TRAITS = backSubstituteTraits<RowsColsA, InputType, OutputType> >
void backSubstitute(hls::stream<InputType>& matrixAStrm, hls::stream<OutputType>& matrixBStrm, int& is_singular) {
    InputType A[RowsColsA][RowsColsA];
    OutputType B[RowsColsA][RowsColsA];

    for (int r = 0; r < RowsColsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < RowsColsA; c++) {
            matrixAStrm.read(A[r][c]);
        }
    }

    backSubstituteTop<RowsColsA, TRAITS, InputType, OutputType>(A, B, is_singular);

    for (int r = 0; r < RowsColsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < RowsColsA; c++) {
            matrixBStrm.write(B[r][c]);
        }
    }
}

} // end namespace solver
} // end namespace xf

#endif
