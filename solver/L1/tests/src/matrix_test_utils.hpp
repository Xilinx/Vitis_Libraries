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

#ifndef _XF_SOLVER_MATRIX_TEST_UTILS_HPP_
#define _XF_SOLVER_MATRIX_TEST_UTILS_HPP_

// -------------------------------------------------
// Utilities for matrix *test* code.
// -------------------------------------------------
// Not for use in synthesisable code
// -------------------------------------------------

#include "ap_fixed.h"
#include "hls_math.h"

#include "hls_x_complex.h"
#include "utils/std_complex_utils.h"
#include "utils/x_hls_utils.h"
#include "utils/x_hls_traits.h"

// test_utils
#include "src/x_tb_utils.hpp"
#include "src/scalar_test_utils.hpp"
#include "src/type_test_utils.hpp"

#include <iomanip>
#include <string>
#include <sstream>
#include <iostream>

// Matrix Compare
// ============================================================================

template <unsigned ROWS, unsigned COLS, typename T>
bool are_matrices_equal(T* actual, T* expected, unsigned allowed_ulp_mismatch = 0, T* delta = NULL) {
    bool ret = true;

    typedef xil_equality<T> e;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            T a = *(expected + (r * COLS) + c);
            T b = *(actual + (r * COLS) + c);

            if (delta != NULL) *(delta + (r * COLS) + c) = a - b;

            if (e::equal(a, b, allowed_ulp_mismatch) == false) {
                ret = false;
            }
        }
    }
    return ret;
}

template <unsigned ROWS, unsigned COLS, typename T>
bool are_matrices_equal(T actual[ROWS][COLS],
                        T expected[ROWS][COLS],
                        unsigned allowed_ulp_mismatch = 0,
                        T* delta = NULL) {
    return are_matrices_equal<ROWS, COLS, T>((T*)actual, (T*)expected, allowed_ulp_mismatch, delta);
}

// Generic test for NaN anywhere in a floating-point matrix
template <int ROWS, int COLS, typename T>
int anyNaN(T A[ROWS][COLS]) {
    int foundNaN = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (hls::__isnan(A[r][c]) == 1) {
                foundNaN = 1;
            }
        }
    }
    return foundNaN;
}

// Generic test for NaN anywhere in a complex-valued floating-point matrix
template <int ROWS, int COLS, typename T>
int anyNaN(hls::x_complex<T> A[ROWS][COLS]) {
    int foundNaN = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (hls::__isnan(hls::x_real(A[r][c])) == 1 || hls::__isnan(hls::x_imag(A[r][c])) == 1) {
                foundNaN = 1;
            }
        }
    }
    return foundNaN;
}
template <int ROWS, int COLS, typename T>
int anyNaN(std::complex<T> A[ROWS][COLS]) {
    int foundNaN = 0;
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (hls::__isnan(hls::x_real(A[r][c])) == 1 || hls::__isnan(hls::x_imag(A[r][c])) == 1) {
                foundNaN = 1;
            }
        }
    }
    return foundNaN;
}
// NaN does not apply for fixed-point
template <int ROWS, int COLS, int W, int I, ap_q_mode Q, ap_o_mode O, int N>
int anyNaN(ap_fixed<W, I, Q, O, N> A[ROWS][COLS]) {
    return 0;
}

// NaN does not apply for complex fixed-point
template <int ROWS, int COLS, int W, int I, ap_q_mode Q, ap_o_mode O, int N>
int anyNaN(hls::x_complex<ap_fixed<W, I, Q, O, N> > A[ROWS][COLS]) {
    return 0;
}

template <int ROWS, int COLS, int W, int I, ap_q_mode Q, ap_o_mode O, int N>
int anyNaN(std::complex<ap_fixed<W, I, Q, O, N> > A[ROWS][COLS]) {
    return 0;
}
// Basic matrix operations
// ============================================================================

// Basic matrix mult
template <bool transA,
          bool transB,
          int AROWS,
          int ACOLS,
          int BROWS,
          int BCOLS,
          int CROWS,
          int CCOLS,
          class T_IN,
          class T_OUT>
void mmult(const T_IN A[AROWS][ACOLS], const T_IN B[BROWS][BCOLS], T_OUT C[CROWS][CCOLS]) {
    const int AROWS_INT = (transA ? ACOLS : AROWS);
    const int ACOLS_INT = (transA ? AROWS : ACOLS);
    const int BROWS_INT = (transB ? BCOLS : BROWS);
    const int BCOLS_INT = (transB ? BROWS : BCOLS);

    assert(ACOLS_INT == BROWS_INT);
    assert(CROWS == AROWS_INT);
    assert(CCOLS == BCOLS_INT);

    T_IN a_tmp, b_tmp;
    T_OUT sum;
    for (int a_row = 0; a_row < AROWS_INT; a_row++) {
        for (int b_col = 0; b_col < BCOLS_INT; b_col++) {
            sum = 0;
            for (int i = 0; i < ACOLS_INT; i++) {
                if (transA) {
                    a_tmp = hls::x_conj(A[i][a_row]);
                } else {
                    a_tmp = A[a_row][i];
                }
                if (transB) {
                    b_tmp = hls::x_conj(B[b_col][i]);
                } else {
                    b_tmp = B[i][b_col];
                }
                sum = sum + (a_tmp * b_tmp);
            }
            C[a_row][b_col] = sum;
        }
    }
};

// Basic sub
template <int ROWS, int COLS, class T_IN, class T_OUT>
int msub(const T_IN A[ROWS][COLS], const T_IN B[ROWS][COLS], T_OUT C[ROWS][COLS]) {
    T_IN tmp;
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            // Hack due to the rubish implementation of the x_complex LHS can't be a const....
            tmp = A[row][col];
            C[row][col] = tmp - B[row][col];
        }
    }
    return 0;
};

// Matrix Conditioning
// ============================================================================

template <typename T>
void set_upper_triangle_to_zero(T* a, unsigned dim) {
    for (int r = 0; r < dim; r++) {
        for (int c = r + 1; c < dim; c++) {
            *(a + (r * dim) + c) = 0.0;
        }
    }
}

template <typename T>
void set_lower_triangle_to_zero(T* a, unsigned dim) {
    for (int r = 0; r < dim; r++) {
        for (int c = 0; c < r; c++) {
            *(a + (r * dim) + c) = 0.0;
        }
    }
}

template <typename T, typename T_BASE>
T_BASE norm1_magnitude(T_BASE re, T_BASE im) {
    const T_BASE ONE = 1.0f; // REVISIT: not suitable for double
    const T_BASE ZERO = 0.0f;
    T_BASE t;

    re = solver_tb::fabs(re); // should call float version of fabs in C++?
    im = solver_tb::fabs(im);
    if (re > im) {
        t = im / re;
        return re * sqrt(ONE + t * t); // should call float version of sqrt in C++?
    } else {
        if (im == ZERO) {
            return ZERO;
        }
        t = re / im;
        return im * sqrt(ONE + t * t);
    }
}

template <typename T, typename T_BASE>
T_BASE norm1_abs(T x) {
    T_BASE y;
    y = norm1_magnitude<T, T_BASE>(hls::x_real(x), hls::x_imag(x));
    return y;
}

// Compute 1-norm of a matrix directly
template <int ROWS, int COLS, typename T, typename T_BASE>
T_BASE norm1(T in[ROWS][COLS]) {
    T_BASE norm = 0;
    T_BASE norm_cols[COLS];

    // Initialise
    for (int c = 0; c < COLS; c++) {
        norm_cols[c] = 0;
    }

    // Sum column absolute values
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            norm_cols[c] += norm1_abs<T, T_BASE>(in[r][c]);
        }
    }

    // Find largest column sum
    for (int c = 0; c < COLS; c++) {
        if (norm_cols[c] > norm) {
            norm = norm_cols[c];
        }
    }

    return norm;
}

// Variant of norm1 which computes in double and returns double.
// This is necessary for computing the 1-norm of A, as very large values can
// cause float to overflow.
template <int ROWS, int COLS, typename T, typename T_BASE>
double norm1_dbl(T in[ROWS][COLS]) {
    double norm = 0;
    double norm_cols[COLS];

    // Initialise
    for (int c = 0; c < COLS; c++) {
        norm_cols[c] = 0;
    }

    // Sum column absolute values
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            norm_cols[c] += norm1_abs<T, double>(in[r][c]);
        }
    }

    // Find largest column sum
    for (int c = 0; c < COLS; c++) {
        if (norm_cols[c] > norm) {
            norm = norm_cols[c];
        }
    }

    return norm;
}

// Checks the TB generated test criteria ratios:
// - isnan
// - isinf
// - iszero - optional. Default disabled
// - less than zero
// Returns non-zero if test fails
int check_ratio(double ratio, bool check_is_zero = false) {
    if (isnan(ratio)) {
        std::cout << "ERROR: check_ratio: ISNAN" << std::endl;
        return (1);
    }
    if (isinf(ratio)) {
        std::cout << "ERROR: check_ratio: ISINF" << std::endl;
        return (2);
    }
    if (ratio == 0 && check_is_zero) {
        std::cout << "ERROR: check_ratio: Is zero" << std::endl;
        return (4);
    }
    if (ratio < 0) {
        std::cout << "ERROR: check_ratio: Negative ratio: " << ratio << std::endl;
        return (1);
    }
    return (0);
}

#endif
