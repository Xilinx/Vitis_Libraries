/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
 * @file cholesky.hpp
 * @brief This file contains cholesky functions
 *   - cholesky                : Entry point function
 *   - choleskyTop             : Top level function that selects implementation architecture and internal types based
 *                               on a traits class.
 *   - choleskyBasic           : Basic implementation (ARCH=0). Simple, sequential column-by-column reference
 *                               design; fewest DSPs of the three architectures.
 *   - choleskyAlt             : Balanced implementation (ARCH=1) with lower latency than choleskyBasic,
 *                               requiring only modestly more resource.
 *   - choleskyAlt2            : Lowest-latency, highest-resource architecture (ARCH=2). Fastest option at
 *                               small/moderate matrix size; resource usage scales up as matrix size grows,
 *                               and timing may not close at larger sizes.
 *
 * All three architectures share a square root back end selected on the element type; see
 * choleskyMathOps below.  The diagonal needs both sqrt(x) and 1/sqrt(x), and evaluating them as two
 * independent operators dominates the critical path, so only the reciprocal is computed and the
 * root is recovered from it where that is profitable.
 */

#ifndef _XF_SOLVER_CHOLESKY_HPP_
#define _XF_SOLVER_CHOLESKY_HPP_

#include "ap_fixed.h"
#include "hls_x_complex.h"
#include <complex>
#include "utils/std_complex_utils.h"
#include "utils/x_matrix_utils.hpp"
#include "hls_stream.h"

namespace xf {
namespace solver {

// ===================================================================================================================
// Default traits struct defining the internal variable types for the cholesky function
template <bool LowerTriangularL, int RowsColsA, typename InputType, typename OutputType>
struct choleskyTraits {
    typedef InputType PROD_T;
    typedef InputType ACCUM_T;
    typedef InputType ADD_T;
    typedef InputType DIAG_T;
    typedef InputType RECIP_DIAG_T;
    typedef InputType OFF_DIAG_T;
    typedef OutputType L_OUTPUT_T;
    static const int ARCH =
        1; // Select implementation: 0=Basic, 1=Lower latency architecture, 2=Further improved latency architecture
    static const int INNER_II = 1; // Specify the pipelining target for the inner loop
    static const int ARCH2_ZERO_LOOP =
        true; // Additional implementation "switch" for the choleskyAlt2 architecture (2).
};

// Specialization for complex
template <bool LowerTriangularL, int RowsColsA, typename InputBaseType, typename OutputBaseType>
struct choleskyTraits<LowerTriangularL, RowsColsA, hls::x_complex<InputBaseType>, hls::x_complex<OutputBaseType> > {
    typedef hls::x_complex<InputBaseType> PROD_T;
    typedef hls::x_complex<InputBaseType> ACCUM_T;
    typedef hls::x_complex<InputBaseType> ADD_T;
    typedef hls::x_complex<InputBaseType> DIAG_T;
    typedef InputBaseType RECIP_DIAG_T;
    typedef hls::x_complex<InputBaseType> OFF_DIAG_T;
    typedef hls::x_complex<OutputBaseType> L_OUTPUT_T;
    static const int ARCH = 1;
    static const int INNER_II = 1;
    static const int ARCH2_ZERO_LOOP = true;
};

// Specialization for std complex
template <bool LowerTriangularL, int RowsColsA, typename InputBaseType, typename OutputBaseType>
struct choleskyTraits<LowerTriangularL, RowsColsA, std::complex<InputBaseType>, std::complex<OutputBaseType> > {
    typedef std::complex<InputBaseType> PROD_T;
    typedef std::complex<InputBaseType> ACCUM_T;
    typedef std::complex<InputBaseType> ADD_T;
    typedef std::complex<InputBaseType> DIAG_T;
    typedef InputBaseType RECIP_DIAG_T;
    typedef std::complex<InputBaseType> OFF_DIAG_T;
    typedef std::complex<OutputBaseType> L_OUTPUT_T;
    static const int ARCH = 1;
    static const int INNER_II = 1;
    static const int ARCH2_ZERO_LOOP = true;
};

// Specialization for ap_fixed
template <bool LowerTriangularL,
          int RowsColsA,
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
struct choleskyTraits<LowerTriangularL, RowsColsA, ap_fixed<W1, I1, Q1, O1, N1>, ap_fixed<W2, I2, Q2, O2, N2> > {
    typedef ap_fixed<W1 + W1, I1 + I1, AP_RND_CONV, AP_SAT, 0> PROD_T;
    typedef ap_fixed<(W1 + W1) + BitWidth<RowsColsA>::Value,
                     (I1 + I1) + BitWidth<RowsColsA>::Value,
                     AP_RND_CONV,
                     AP_SAT,
                     0>
        ACCUM_T;
    typedef ap_fixed<W1 + 1, I1 + 1, AP_RND_CONV, AP_SAT, 0> ADD_T;
    typedef ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> DIAG_T;     // Takes result of sqrt
    typedef ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> OFF_DIAG_T; // Takes result of /
    // Reciprocal of the diagonal, 1/sqrt(x).  See the std::complex<ap_fixed> specialization for the
    // derivation: the width is bounded by the sqrt *input*, ADD_T, not by the output type.
    typedef ap_fixed<(1 + ((W1 - I1) + 1) / 2) + (W2 - I2) + 1, 1 + ((W1 - I1) + 1) / 2, AP_RND_CONV, AP_SAT, 0>
        RECIP_DIAG_T;
    typedef ap_fixed<W2, I2, AP_RND_CONV, AP_SAT, 0>
        L_OUTPUT_T; // Takes new L value.  Same as L output but saturation set
    static const int ARCH = 1;
    static const int INNER_II = 1;
    static const int ARCH2_ZERO_LOOP = true;
};

// Further specialization for hls::complex<ap_fixed>
template <bool LowerTriangularL,
          int RowsColsA,
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
struct choleskyTraits<LowerTriangularL,
                      RowsColsA,
                      hls::x_complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                      hls::x_complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
    typedef hls::x_complex<ap_fixed<W1 + W1, I1 + I1, AP_RND_CONV, AP_SAT, 0> > PROD_T;
    typedef hls::x_complex<ap_fixed<(W1 + W1) + BitWidth<RowsColsA>::Value,
                                    (I1 + I1) + BitWidth<RowsColsA>::Value,
                                    AP_RND_CONV,
                                    AP_SAT,
                                    0> >
        ACCUM_T;
    typedef hls::x_complex<ap_fixed<W1 + 1, I1 + 1, AP_RND_CONV, AP_SAT, 0> > ADD_T;
    typedef hls::x_complex<ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> > DIAG_T;     // Takes result of sqrt
    typedef hls::x_complex<ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> > OFF_DIAG_T; // Takes result of /
    // Reciprocal of the diagonal, 1/sqrt(x).  See the std::complex<ap_fixed> specialization for the
    // derivation: the width is bounded by the sqrt *input*, ADD_T, not by the output type.
    typedef ap_fixed<(1 + ((W1 - I1) + 1) / 2) + (W2 - I2) + 1, 1 + ((W1 - I1) + 1) / 2, AP_RND_CONV, AP_SAT, 0>
        RECIP_DIAG_T;
    typedef hls::x_complex<ap_fixed<W2, I2, AP_RND_CONV, AP_SAT, 0> >
        L_OUTPUT_T; // Takes new L value.  Same as L output but saturation set
    static const int ARCH = 1;
    static const int INNER_II = 1;
    static const int ARCH2_ZERO_LOOP = true;
};

// Further specialization for std::complex<ap_fixed>
template <bool LowerTriangularL,
          int RowsColsA,
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
struct choleskyTraits<LowerTriangularL,
                      RowsColsA,
                      std::complex<ap_fixed<W1, I1, Q1, O1, N1> >,
                      std::complex<ap_fixed<W2, I2, Q2, O2, N2> > > {
    typedef std::complex<ap_fixed<W1 + W1, I1 + I1, AP_RND_CONV, AP_SAT, 0> > PROD_T;
    typedef std::complex<ap_fixed<(W1 + W1) + BitWidth<RowsColsA>::Value,
                                  (I1 + I1) + BitWidth<RowsColsA>::Value,
                                  AP_RND_CONV,
                                  AP_SAT,
                                  0> >
        ACCUM_T;
    typedef std::complex<ap_fixed<W1 + 1, I1 + 1, AP_RND_CONV, AP_SAT, 0> > ADD_T;
    typedef std::complex<ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> > DIAG_T;     // Takes result of sqrt
    typedef std::complex<ap_fixed<(W1 + 1) * 2, I1 + 1, AP_RND_CONV, AP_SAT, 0> > OFF_DIAG_T; // Takes result of /
    // Reciprocal of the diagonal, 1/sqrt(x).
    // o The width is bounded by the *input* of the square root, not by the output type: x is an
    //   ADD_T, ap_fixed<W1+1, I1+1>, so its smallest non-zero magnitude is 2^-(W1-I1) and
    //   1/sqrt(x) cannot exceed 2^((W1-I1)/2).  Sizing the integer part from the output type
    //   instead (2 + W2 - I2 bits) makes this type ~8 bits wider than it needs to be and inflates
    //   every multiplier that consumes it.
    // o The fraction is kept at W2 - I2 + 1 bits, as before.
    typedef ap_fixed<(1 + ((W1 - I1) + 1) / 2) + (W2 - I2) + 1, 1 + ((W1 - I1) + 1) / 2, AP_RND_CONV, AP_SAT, 0>
        RECIP_DIAG_T;
    typedef std::complex<ap_fixed<W2, I2, AP_RND_CONV, AP_SAT, 0> >
        L_OUTPUT_T; // Takes new L value.  Same as L output but saturation set
    static const int ARCH = 1;
    static const int INNER_II = 1;
    static const int ARCH2_ZERO_LOOP = true;
};

// ===================================================================================================================
// Helper functions

// Square root
// o Overloaded versions of the sqrt function
// o The square root of a complex number is expensive.  However, the diagonal values of a Cholesky decomposition are
// always
//   real so we don't need a full complex square root.
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
// Square root back ends, selected on the data type.
//
// The square root and its reciprocal are the dominant cost of the Cholesky diagonal, and the right
// implementation is entirely type dependent:
//
//   type              1/sqrt(x)                            sqrt(x)
//   ---------------   ----------------------------------   ------------------------------------------
//   half/float/       the type's own reciprocal square      x_sqrt(): cheap and exactly rounded, so
//   double            root operator                         it is taken directly
//   ap_fixed<W,I>     evaluated in native floating point;   x * (1/sqrt(x)): x_sqrt() of a fixed
//   ap_ufixed<W,I>    the working precision is derived      point value evaluates a *double
//                     from the fixed point formats so       precision* hls::sqrt() and a second one
//                     the result is resolved to its own     cannot be afforded on the critical path
//                     quantisation step
//
// Any other element type is a compile time error rather than a silent fallthrough.
//
// The reciprocal square root itself, and the working precision it needs, are generic numeric
// utilities and live in x_matrix_utils.hpp.  What is decided here is only how the square root is
// recovered from it, which is a Cholesky critical path trade off.
//
// o Native floating point: the type's own operators.  A native square root is cheap and exactly
//   rounded, and recovering it as x * (1/sqrt(x)) would turn x == 0 into 0 * inf = NaN, so it is
//   evaluated directly.
template <typename T>
struct choleskyMathOpsFloat {
    template <typename T_RECIP>
    static void rsqrt(T x, T_RECIP& res) {
        res = x_rsqrt(x);
    }
    template <typename T_RECIP, typename T_OUT>
    static void sqrt_from_rsqrt(T x, T_RECIP recip, T_OUT& res) {
        res = x_sqrt(x);
    }
};

// o Fixed point: x_sqrt() of a fixed point value evaluates a double precision hls::sqrt(), and a
//   second one cannot be afforded on the critical path, so the root is recovered with a multiply.
//   A zero input saturates the reciprocal, so the product is correctly zero.
template <typename T>
struct choleskyMathOpsFixed {
    template <typename T_RECIP>
    static void rsqrt(T x, T_RECIP& res) {
        x_rsqrt(x, res);
    }
    template <typename T_RECIP, typename T_OUT>
    static void sqrt_from_rsqrt(T x, T_RECIP recip, T_OUT& res) {
        res = x * recip;
    }
};

template <typename T>
struct choleskyAlwaysFalse {
    static const bool VALUE = false;
};

// Back end selection.  The primary template is a hard error rather than a silent fallthrough to
// floating point, so an unhandled element type is reported at compile time.
template <typename T>
struct choleskyMathOps {
    static_assert(choleskyAlwaysFalse<T>::VALUE,
                  "cholesky: no square root back end for this element type; add a "
                  "choleskyMathOps specialisation for it.");
};
template <>
struct choleskyMathOps<half> : choleskyMathOpsFloat<half> {};
template <>
struct choleskyMathOps<float> : choleskyMathOpsFloat<float> {};
template <>
struct choleskyMathOps<double> : choleskyMathOpsFloat<double> {};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
struct choleskyMathOps<ap_fixed<W, I, Q, O, N> > : choleskyMathOpsFixed<ap_fixed<W, I, Q, O, N> > {};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
struct choleskyMathOps<ap_ufixed<W, I, Q, O, N> > : choleskyMathOpsFixed<ap_ufixed<W, I, Q, O, N> > {};

// Reciprocal square root.
template <typename InputType, typename OutputType>
void cholesky_rsqrt(InputType x, OutputType& res) {
Function_cholesky_rsqrt:;
    choleskyMathOps<InputType>::rsqrt(x, res);
}

// Recovery of the square root from the reciprocal square root.
template <typename T_IN, typename T_RECIP, typename T_OUT>
void cholesky_sqrt_from_rsqrt(T_IN a, T_RECIP recip, T_OUT& b) {
Function_cholesky_sqrt_from_rsqrt:;
    choleskyMathOps<T_IN>::sqrt_from_rsqrt(a, recip, b);
}

// Combined square root and reciprocal square root of the same value.
// o The Cholesky diagonal needs both sqrt(x) (the diagonal value itself) and 1/sqrt(x) (kept to
//   avoid a divide in every off-diagonal calculation).  Calling cholesky_sqrt_op() and
//   cholesky_rsqrt() separately instantiates two square root operators which the scheduler runs
//   back to back on the critical path of every row/column.
// o Only the reciprocal square root is computed here; the square root is recovered with a single
//   multiply, sqrt(x) = x * (1/sqrt(x)).
template <typename T_IN, typename T_RECIP, typename T_OUT>
int cholesky_rsqrt_and_sqrt(T_IN a, T_RECIP& recip, T_OUT& b) {
Function_cholesky_rsqrt_and_sqrt_real:;
    const T_IN ZERO = 0;
    const T_RECIP RECIP_ZERO = 0;
    if (a < ZERO) {
        b = ZERO;
        recip = RECIP_ZERO;
        return (1);
    }
    cholesky_rsqrt(a, recip);
    cholesky_sqrt_from_rsqrt(a, recip, b);
    return (0);
}
template <typename T_IN, typename T_RECIP, typename T_OUT>
int cholesky_rsqrt_and_sqrt(hls::x_complex<T_IN> din, T_RECIP& recip, hls::x_complex<T_OUT>& dout) {
Function_cholesky_rsqrt_and_sqrt_complex:;
    const T_IN ZERO = 0;
    const T_RECIP RECIP_ZERO = 0;
    T_IN a = din.real();
    dout.imag(ZERO);

    if (a < ZERO) {
        dout.real(ZERO);
        recip = RECIP_ZERO;
        return (1);
    }

    cholesky_rsqrt(a, recip);
    T_OUT sqrt_res;
    cholesky_sqrt_from_rsqrt(a, recip, sqrt_res);
    dout.real(sqrt_res);
    return (0);
}
template <typename T_IN, typename T_RECIP, typename T_OUT>
int cholesky_rsqrt_and_sqrt(std::complex<T_IN> din, T_RECIP& recip, std::complex<T_OUT>& dout) {
Function_cholesky_rsqrt_and_sqrt_complex:;
    const T_IN ZERO = 0;
    const T_RECIP RECIP_ZERO = 0;
    T_IN a = din.real();
    dout.imag(ZERO);

    if (a < ZERO) {
        dout.real(ZERO);
        recip = RECIP_ZERO;
        return (1);
    }

    cholesky_rsqrt(a, recip);
    T_OUT sqrt_res;
    cholesky_sqrt_from_rsqrt(a, recip, sqrt_res);
    dout.real(sqrt_res);
    return (0);
}

// Local multiplier to handle a complex case currently not supported by the hls::x_complex class
// - Complex multiplied by a real of a different type
// - Required for complex fixed point implementations
template <typename AType, typename BType, typename CType>
void cholesky_prod_sum_mult(AType A, BType B, CType& C) {
Function_cholesky_prod_sum_mult_real:;
    C = A * B;
}
template <typename AType, typename BType, typename CType>
void cholesky_prod_sum_mult(hls::x_complex<AType> A, BType B, hls::x_complex<CType>& C) {
Function_cholesky_prod_sum_mult_complex:;
    C.real(A.real() * B);
    C.imag(A.imag() * B);
}
template <typename AType, typename BType, typename CType>
void cholesky_prod_sum_mult(std::complex<AType> A, BType B, std::complex<CType>& C) {
Function_cholesky_prod_sum_mult_complex:;
    C.real(A.real() * B);
    C.imag(A.imag() * B);
}

// ===================================================================================================================
// choleskyBasic
template <bool LowerTriangularL, int RowsColsA, typename CholeskyTraits, class InputType, class OutputType>
int choleskyBasic(const InputType A[RowsColsA][RowsColsA], OutputType L[RowsColsA][RowsColsA]) {
    int return_code = 0;

    // Use the traits struct to specify the correct type for the intermediate variables. This is really only needed for
    // fixed point.
    typename CholeskyTraits::PROD_T prod;
    typename CholeskyTraits::ACCUM_T sum[RowsColsA];
    typename CholeskyTraits::ACCUM_T A_cast_to_sum;    // A with the same dimensions as sum.
    typename CholeskyTraits::ACCUM_T prod_cast_to_sum; // prod with the same dimensions as sum.

    typename CholeskyTraits::ADD_T A_minus_sum;
    typename CholeskyTraits::DIAG_T new_L_diag;         // sqrt(A_minus_sum)
    typename CholeskyTraits::OFF_DIAG_T new_L_off_diag; // sum/L
    typename CholeskyTraits::OFF_DIAG_T L_cast_to_new_L_off_diag;

    typename CholeskyTraits::L_OUTPUT_T new_L;
    OutputType retrieved_L;
    // Internal memory used to aviod read access from function output argument L.
    // NOTE: The internal matrix only needs to be triangular but optimization using a 1-D array it will require addition
    // logic to generate the indexes. Refer to the choleskyAlt function.
    OutputType L_internal[RowsColsA][RowsColsA];

col_loop:
    for (int j = 0; j < RowsColsA; j++) {
        sum[j] = 0;

    // Calculate the diagonal value for this column
    diag_loop:
        for (int k = 0; k < RowsColsA; k++) {
            if (k <= (j - 1)) {
                if (LowerTriangularL == true) {
                    retrieved_L = L_internal[j][k];
                } else {
                    retrieved_L = L_internal[k][j];
                }
                // Accumulates sum(|L[j][k]|^2) over k < j; a plain assignment here would leave
                // only the last term and is wrong for every column beyond the second.
                sum[j] += hls::x_conj(retrieved_L) * retrieved_L;
            }
        }
        A_cast_to_sum = A[j][j];

        A_minus_sum = A_cast_to_sum - sum[j];

        if (cholesky_sqrt_op(A_minus_sum, new_L_diag)) {
#ifndef __SYNTHESIS__
            printf("ERROR: Trying to find the square root of a negative number\n");
#endif
            return_code = 1;
        }

        // Round to target format using method specifed by traits defined types.
        new_L = new_L_diag;

        if (LowerTriangularL == true) {
            L_internal[j][j] = new_L;
            L[j][j] = new_L;
        } else {
            L_internal[j][j] = hls::x_conj(new_L);
            L[j][j] = hls::x_conj(new_L);
        }

    // Calculate the off diagonal values for this column
    off_diag_loop:
        for (int i = 0; i < RowsColsA; i++) {
            if (i > j) {
                if (LowerTriangularL == true) {
                    sum[j] = A[i][j];
                } else {
                    sum[j] = hls::x_conj(A[j][i]);
                }

            sum_loop:
                for (int k = 0; k < RowsColsA; k++) {
#pragma HLS PIPELINE II = CholeskyTraits::INNER_II
                    if (k <= (j - 1)) {
                        if (LowerTriangularL == true) {
                            prod = -L_internal[i][k] * hls::x_conj(L_internal[j][k]);
                        } else {
                            prod = -hls::x_conj(L_internal[k][i]) * (L_internal[k][j]);
                        }

                        prod_cast_to_sum = prod;
                        sum[j] += prod_cast_to_sum;
                    }
                }

                new_L_off_diag = sum[j];

                L_cast_to_new_L_off_diag = L_internal[j][j];

                // Diagonal is always real, avoid complex division
                new_L_off_diag = new_L_off_diag / hls::x_real(L_cast_to_new_L_off_diag);

                // Round to target format using method specifed by traits defined types.
                new_L = new_L_off_diag;

                if (LowerTriangularL == true) {
                    L[i][j] = new_L;
                    L_internal[i][j] = new_L;
                } else {
                    L[j][i] = hls::x_conj(new_L);
                    L_internal[j][i] = hls::x_conj(new_L);
                }
            } else if (i < j) {
                if (LowerTriangularL == true) {
                    L[i][j] = 0;
                } else {
                    L[j][i] = 0;
                }
            }
        }
    }
    return (return_code);
}

// ===================================================================================================================
// choleskyAlt: Alternative architecture with improved latency at the expense of higher resource
template <bool LowerTriangularL, int RowsColsA, typename CholeskyTraits, class InputType, class OutputType>
int choleskyAlt(const InputType A[RowsColsA][RowsColsA], OutputType L[RowsColsA][RowsColsA]) {
    int return_code = 0;

    // Optimize internal memories
    // - For complex data types the diagonal will be real only, plus for fixed point it must be stored to a
    //   higher precision.
    // - Requires additional logic to generate the memory indexes
    // - For smaller matrix sizes there maybe be an increase in memory usage
    OutputType L_internal[(RowsColsA * RowsColsA - RowsColsA) / 2];
    typename CholeskyTraits::RECIP_DIAG_T diag_internal[RowsColsA];

    typename CholeskyTraits::ACCUM_T square_sum;
    typename CholeskyTraits::ACCUM_T A_cast_to_sum;
    typename CholeskyTraits::ADD_T A_minus_sum;
    typename CholeskyTraits::DIAG_T new_L_diag;
    typename CholeskyTraits::RECIP_DIAG_T new_L_diag_recip;
    typename CholeskyTraits::PROD_T prod;
    typename CholeskyTraits::ACCUM_T prod_cast_to_sum;
    typename CholeskyTraits::ACCUM_T product_sum;
    typename CholeskyTraits::OFF_DIAG_T prod_cast_to_off_diag;
    typename CholeskyTraits::RECIP_DIAG_T L_diag_recip;
    typename CholeskyTraits::OFF_DIAG_T new_L_off_diag;
    typename CholeskyTraits::L_OUTPUT_T new_L;
    typename CholeskyTraits::L_OUTPUT_T new_L_recip;

row_loop:
    for (int i = 0; i < RowsColsA; i++) {
        // Index generation for optimized/packed L_internal memory
        int i_sub1 = i - 1;
        int i_off = ((i_sub1 * i_sub1 - i_sub1) / 2) + i_sub1;

        // Off diagonal calculation
        square_sum = 0;
    col_loop:
        for (int j = 0; j < i; j++) {
#pragma HLS loop_tripcount max = 1 + RowsColsA / 2
            // Index generation
            int j_sub1 = j - 1;
            int j_off = ((j_sub1 * j_sub1 - j_sub1) / 2) + j_sub1;
            // Prime the off-diagonal sum with target elements A value.
            if (LowerTriangularL == true) {
                product_sum = A[i][j];
            } else {
                product_sum = hls::x_conj(A[j][i]);
            }
        sum_loop:
            for (int k = 0; k < j; k++) {
#pragma HLS loop_tripcount max = 1 + RowsColsA / 2
#pragma HLS PIPELINE II = CholeskyTraits::INNER_II
                prod = -L_internal[i_off + k] * hls::x_conj(L_internal[j_off + k]);
                prod_cast_to_sum = prod;
                product_sum += prod_cast_to_sum;
            }
            prod_cast_to_off_diag = product_sum;
            // Fetch diagonal value
            L_diag_recip = diag_internal[j];
            // Diagonal is stored in its reciprocal form so only need to multiply the product sum
            cholesky_prod_sum_mult(prod_cast_to_off_diag, L_diag_recip, new_L_off_diag);
            // Round to target format using method specifed by traits defined types.
            new_L = new_L_off_diag;
            // Build sum for use in diagonal calculation for this row.
            square_sum += hls::x_conj(new_L) * new_L;
            // Store result
            L_internal[i_off + j] = new_L;
            if (LowerTriangularL == true) {
                L[i][j] = new_L; // store in lower triangle
                L[j][i] = 0;     // Zero upper
            } else {
                L[j][i] = hls::x_conj(new_L); // store in upper triangle
                L[i][j] = 0;                  // Zero lower
            }
        }

        // Diagonal calculation
        A_cast_to_sum = A[i][i];
        A_minus_sum = A_cast_to_sum - square_sum;
        // Generate the reciprocal of the diagonal for internal use to aviod the latency of a divide in every
        // off-diagonal calculation, and recover the diagonal itself as x * (1/sqrt(x)) so that only a single
        // square root operator is instantiated on the critical path.
        if (cholesky_rsqrt_and_sqrt(A_minus_sum, new_L_diag_recip, new_L_diag)) {
#ifndef __SYNTHESIS__
            printf("ERROR: Trying to find the square root of a negative number\n");
#endif
            return_code = 1;
        }
        // Round to target format using method specifed by traits defined types.
        new_L = new_L_diag;
        // Store diagonal value
        diag_internal[i] = new_L_diag_recip;
        if (LowerTriangularL == true) {
            L[i][i] = new_L;
        } else {
            L[i][i] = hls::x_conj(new_L);
        }
    }
    return (return_code);
}

// ===================================================================================================================
// choleskyAlt2: Flattens the column loop into the same pipeline as the reduction/finalize work,
// instead of choleskyAlt's separate, un-pipelined outer column loop where every column serially
// paid its diagonal reciprocal-square-root latency (the single largest term in the critical
// dependency chain) with no chance of hiding it behind other columns' independent work. Column j's
// diagonal reciprocal-square-root is placed at the otherwise-idle (k == 0, i == j) slot (the k == 0
// sweep primes product_sum_array for i > j, so i == j is unused there); its result is read back at
// (k == j, i > j) for that same column's off-diagonal finalize, j*RowsColsA flattened iterations
// later, which is the only place in the design a later column's independent reduction work can
// plausibly overlap with an earlier column's still-draining reciprocal-square-root.
// Fastest option at small/moderate RowsColsA; see the file header for the large-matrix caveat.

// Uniquely-named, non-overloaded wrapper around cholesky_rsqrt_and_sqrt so that
// #pragma HLS ALLOCATION below has an unambiguous function to bind: the real function has three
// overloads (real/x_complex/std::complex), and HLS 207-7045 ("not a valid function pointer
// expression") is the tool refusing to guess which overload the bare, overloaded name means.
template <typename T_IN, typename T_RECIP, typename T_OUT>
static inline int choleskyAlt2_rsqrt_and_sqrt(T_IN a, T_RECIP& recip, T_OUT& b) {
    return cholesky_rsqrt_and_sqrt(a, recip, b);
}

// Same wrapper trick for cholesky_prod_sum_mult (also overloaded real/x_complex/std::complex).
// Only used in the finalize branch (k == j, i > j) -- ~N(N-1)/2 calls total, each already gated
// behind that column's diag_recip_array[j], itself paced by the now-shared rsqrt unit above. So
// sharing this one too should cost little: the calls it makes are not on the critical throughput
// path the way the reduction's multiply-accumulate (operator*/operator+=) is.
template <typename T_A, typename T_B, typename T_C>
static inline void choleskyAlt2_prod_sum_mult(T_A a, T_B b, T_C& c) {
    cholesky_prod_sum_mult(a, b, c);
}

template <bool LowerTriangularL, int RowsColsA, typename CholeskyTraits, class InputType, class OutputType>
int choleskyAlt2(const InputType A[RowsColsA][RowsColsA], OutputType L[RowsColsA][RowsColsA]) {
#pragma HLS PIPELINE
// clang-format off
#pragma HLS ALLOCATION function instances=choleskyAlt2_rsqrt_and_sqrt<typename CholeskyTraits::ADD_T, typename CholeskyTraits::RECIP_DIAG_T, typename CholeskyTraits::DIAG_T> limit=1
#pragma HLS ALLOCATION function instances=choleskyAlt2_prod_sum_mult<typename CholeskyTraits::OFF_DIAG_T, typename CholeskyTraits::RECIP_DIAG_T, typename CholeskyTraits::OFF_DIAG_T> limit=1
    // clang-format on
    int return_code = 0;

    OutputType L_internal[RowsColsA][RowsColsA];
    typename CholeskyTraits::ACCUM_T square_sum_array[RowsColsA];
    typename CholeskyTraits::ACCUM_T A_cast_to_sum;
    typename CholeskyTraits::ADD_T A_minus_sum;
    typename CholeskyTraits::DIAG_T new_L_diag;
    typename CholeskyTraits::RECIP_DIAG_T new_L_diag_recip;
    // Persists the reciprocal diagonal per column: the diagonal calc is part of the flattened
    // iteration space below, so its result must survive from (k==0, i==j) to (k==j, i>j).
    typename CholeskyTraits::RECIP_DIAG_T diag_recip_array[RowsColsA];
    typename CholeskyTraits::PROD_T prod;
    typename CholeskyTraits::ACCUM_T prod_cast_to_sum;
    typename CholeskyTraits::ACCUM_T product_sum_array[RowsColsA];
    typename CholeskyTraits::OFF_DIAG_T prod_cast_to_off_diag;
    typename CholeskyTraits::OFF_DIAG_T new_L_off_diag;
    typename CholeskyTraits::L_OUTPUT_T new_L;

#pragma HLS ARRAY_PARTITION variable = product_sum_array complete dim = 1
#pragma HLS ARRAY_PARTITION variable = diag_recip_array complete dim = 1
#pragma HLS ARRAY_PARTITION variable = square_sum_array complete dim = 1
#pragma HLS ARRAY_PARTITION variable = L_internal complete dim = 1
#pragma HLS ARRAY_PARTITION variable = A complete dim = 0
#pragma HLS ARRAY_PARTITION variable = L complete dim = 0

// Column j flattened together with k and i, instead of being a separate, un-pipelined outer loop.
col_loop:
    for (int j = 0; j < RowsColsA; j++) {
        for (int k = 0; k < RowsColsA; k++) {
            for (int i = 0; i < RowsColsA; i++) {
                if (k == 0 && i == j) {
                    // Diagonal calculation for this column, placed in the (k == 0, i == j) slot,
                    // which the i > j guards below leave otherwise idle.
                    A_cast_to_sum = A[j][j];
                    if (j == 0) {
                        A_minus_sum = A_cast_to_sum;
                    } else {
                        A_minus_sum = A_cast_to_sum - square_sum_array[j];
                    }
                    // Generate the reciprocal of the diagonal for internal use to aviod the latency of a divide in
                    // every off-diagonal calculation, and recover the diagonal itself as x * (1/sqrt(x)) so that
                    // only a single square root operator is instantiated on the critical path.
                    if (choleskyAlt2_rsqrt_and_sqrt(A_minus_sum, new_L_diag_recip, new_L_diag)) {
#ifndef __SYNTHESIS__
                        printf("ERROR: Trying to find the square root of a negative number\n");
#endif
                        return_code = 1;
                    }
                    diag_recip_array[j] = new_L_diag_recip;
                    // Round to target format using method specifed by traits defined types.
                    new_L = new_L_diag;
                    // Store diagonal value
                    if (LowerTriangularL == true) {
                        L[j][j] = new_L;
                    } else {
                        L[j][j] = hls::x_conj(new_L);
                    }
                } else if (i > j) {
                    // Prime, accumulate, and finalize are three INDEPENDENT "if"s, not an
                    // if/else-if/else chain. This matters for two overlapping cases:
                    //  - column j == 0: k == 0 and k == j are the SAME iteration, so both prime
                    //    and finalize must fire on it (prime sets product_sum_array[i], which
                    //    finalize immediately consumes).
                    //  - any column j >= 1: k == 0 is ALSO the first term of the reduction
                    //    (k < j, since 0 < j), so both prime and accumulate must fire on it --
                    //    prime sets product_sum_array[i] = A[i][j], then accumulate immediately
                    //    adds the k == 0 product term on top. Chaining prime and accumulate as
                    //    if/else-if lets the k == 0 prime branch silently steal the iteration and
                    //    drop that first reduction term for every column.
                    if (k == 0) {
                        // Prime first sum
                        if (LowerTriangularL == true) {
                            A_cast_to_sum = A[i][j];
                        } else {
                            A_cast_to_sum = hls::x_conj(A[j][i]);
                        }
                        product_sum_array[i] = A_cast_to_sum;
                    }
                    if (k < j) {
                        // Accumulate row sum of columns
                        prod = L_internal[i][k] * (-hls::x_conj(L_internal[j][k]));
                        prod_cast_to_sum = prod;
                        product_sum_array[i] += prod_cast_to_sum;
                    }
                    if (k == j) {
                        // final calculation for off diagonal value, using the diagonal
                        // reciprocal computed at this column's (k == 0, i == j) slot above.
                        // (k > j does nothing here -- correctly idle.)
                        prod_cast_to_off_diag = product_sum_array[i];
                        // Diagonal is stored in its reciprocal form so only need to multiply the product sum
                        choleskyAlt2_prod_sum_mult(prod_cast_to_off_diag, diag_recip_array[j], new_L_off_diag);
                        // Round to target format using method specifed by traits defined types.
                        new_L = new_L_off_diag;
                        // Build sum for use in the diagonal calculation of row i.
                        if (j == 0) {
                            square_sum_array[i] = hls::x_conj(new_L) * new_L;
                        } else {
                            square_sum_array[i] += hls::x_conj(new_L) * new_L;
                        }
                        // Store result
                        L_internal[i][j] = new_L;
                        if (LowerTriangularL == true) {
                            L[i][j] = new_L;                                   // Store in lower triangle
                            if (!CholeskyTraits::ARCH2_ZERO_LOOP) L[j][i] = 0; // Zero upper
                        } else {
                            L[j][i] = hls::x_conj(new_L);                      // Store in upper triangle
                            if (!CholeskyTraits::ARCH2_ZERO_LOOP) L[i][j] = 0; // Zero lower
                        }
                    }
                }
            }
        }
    }
    // Zero upper/lower triangle
    if (CholeskyTraits::ARCH2_ZERO_LOOP) {
    zero_rows_loop:
        for (int i = 0; i < RowsColsA - 1; i++) {
        zero_cols_loop:
            for (int j = i + 1; j < RowsColsA; j++) {
                if (LowerTriangularL == true) {
                    L[i][j] = 0;
                } else {
                    L[j][i] = 0;
                }
            }
        }
    }
    return (return_code);
}

// ===================================================================================================================
// choleskyTop: Top level function that selects implementation architecture and internal types based on the
// traits class provided via the CholeskyTraits template parameter.
// o Call this function directly if you wish to override the default architecture choice or internal types
template <bool LowerTriangularL, int RowsColsA, typename CholeskyTraits, class InputType, class OutputType>
int choleskyTop(const InputType A[RowsColsA][RowsColsA], OutputType L[RowsColsA][RowsColsA]) {
    switch (CholeskyTraits::ARCH) {
        case 0:
            return choleskyBasic<LowerTriangularL, RowsColsA, CholeskyTraits, InputType, OutputType>(A, L);
        case 1:
            return choleskyAlt<LowerTriangularL, RowsColsA, CholeskyTraits, InputType, OutputType>(A, L);
        case 2:
            return choleskyAlt2<LowerTriangularL, RowsColsA, CholeskyTraits, InputType, OutputType>(A, L);
        default:
            return choleskyBasic<LowerTriangularL, RowsColsA, CholeskyTraits, InputType, OutputType>(A, L);
    }
}

/**
* @brief cholesky
*
* @tparam LowerTriangularL   When false generates the result in the upper triangle
* @tparam RowsColsA          Defines the matrix dimensions
* @tparam InputType          Input data type
* @tparam OutputType         Output data type
* @tparam TRAITS             choleskyTraits class
*
* @param matrixAStrm         Stream of Hermitian/symmetric positive definite input matrix
* @param matrixLStrm         Stream of Lower or upper triangular output matrix
*
* @return                    An integer type. 0=Success. 1=Failure. The function attempted to find the square root of
* a negative number i.e. the input matrix A was not Hermitian/symmetric positive definite.
*/
template <bool LowerTriangularL,
          int RowsColsA,
          class InputType,
          class OutputType,
          typename TRAITS = choleskyTraits<LowerTriangularL, RowsColsA, InputType, OutputType> >
int cholesky(hls::stream<InputType>& matrixAStrm, hls::stream<OutputType>& matrixLStrm) {
    InputType A[RowsColsA][RowsColsA];
    OutputType L[RowsColsA][RowsColsA];

    for (int r = 0; r < RowsColsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < RowsColsA; c++) {
            matrixAStrm.read(A[r][c]);
        }
    }

    int ret = 0;
    ret = choleskyTop<LowerTriangularL, RowsColsA, TRAITS, InputType, OutputType>(A, L);

    for (int r = 0; r < RowsColsA; r++) {
#pragma HLS PIPELINE
        for (int c = 0; c < RowsColsA; c++) {
            matrixLStrm.write(L[r][c]);
        }
    }
    return ret;
}

} // end namespace solver
} // end namespace xf
#endif
