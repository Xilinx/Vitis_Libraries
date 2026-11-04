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

#include "mp_pseudoinverse_svd_ref.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

// ---------------------------------------------------------------------------
// Scalar helpers (same conventions as mp_pseudoinverse_svd.cpp, self-contained)
// ---------------------------------------------------------------------------

// Complex conjugate
template <typename TT>
static inline TT ref_conj(TT a);
template <>
inline float ref_conj(float a) {
    return a;
}
template <>
inline cfloat ref_conj(cfloat a) {
    return {a.real, -a.imag};
}

// Complex multiplication
template <typename TT>
static inline TT ref_mul(TT a, TT b);
template <>
inline float ref_mul(float a, float b) {
    return a * b;
}
template <>
inline cfloat ref_mul(cfloat a, cfloat b) {
    return {a.real * b.real - a.imag * b.imag, a.real * b.imag + a.imag * b.real};
}

// Addition
template <typename TT>
static inline TT ref_add(TT a, TT b);
template <>
inline float ref_add(float a, float b) {
    return a + b;
}
template <>
inline cfloat ref_add(cfloat a, cfloat b) {
    return {a.real + b.real, a.imag + b.imag};
}

// Subtraction
template <typename TT>
static inline TT ref_sub(TT a, TT b);
template <>
inline float ref_sub(float a, float b) {
    return a - b;
}
template <>
inline cfloat ref_sub(cfloat a, cfloat b) {
    return {a.real - b.real, a.imag - b.imag};
}

// Scalar division by a real (float) divisor
template <typename TT>
static inline TT ref_div_real(TT a, float d);
template <>
inline float ref_div_real(float a, float d) {
    return a / d;
}
template <>
inline cfloat ref_div_real(cfloat a, float d) {
    return {a.real / d, a.imag / d};
}

// Return the real part of a value (used for pivot magnitude)
template <typename TT>
static inline float ref_real(TT a);
template <>
inline float ref_real(float a) {
    return a;
}
template <>
inline float ref_real(cfloat a) {
    return a.real;
}

// Zero value
template <typename TT>
static inline TT ref_zero();
template <>
inline float ref_zero() {
    return 0.0f;
}
template <>
inline cfloat ref_zero() {
    return {0.0f, 0.0f};
}

// One value (used to initialise identity matrix).
// Using brace-init syntax  (e.g. `{1.0f}`)  for assignment to cfloat fails in
// the x86sim clang++ back-end with "no viable overloaded '='"; this helper
// provides a portable alternative.
template <typename TT>
static inline TT ref_one();
template <>
inline float ref_one() {
    return 1.0f;
}
template <>
inline cfloat ref_one() {
    return {1.0f, 0.0f};
}

// Absolute value of real part (pivot magnitude)
static inline float ref_abs(float x) {
    return (x < 0.0f) ? -x : x;
}

// ---------------------------------------------------------------------------
// mp_pseudoinverse_svd_ref::run()
// ---------------------------------------------------------------------------
//
// Computes A^+ = (A^H * A)^-1 * A^H using Gauss-Jordan elimination.
//
// Storage:
//   inA   : A (TP_DIM_ROWS x TP_DIM_COLS, column-major), inA[i + j*TP_DIM_ROWS]
//   outAp  : A^+ (TP_DIM_COLS x TP_DIM_ROWS, column-major), outAp[i + j*TP_DIM_COLS]
//
// Works for full-rank overdetermined systems (TP_DIM_ROWS >= TP_DIM_COLS).
// Output buffer: exactly TP_DIM_COLS × TP_DIM_ROWS samples, column-major
// with leading dimension TP_DIM_COLS (no kStoreCols padding).
//
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS>
void mp_pseudoinverse_svd_ref<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS>::mp_pseudoinverse_svd_ref_main(
    input_buffer<TT_DATA>& __restrict inA, output_buffer<TT_DATA>& __restrict outAp) {
    TT_DATA* __restrict pA = inA.data();
    TT_DATA* __restrict pAp = outAp.data();

    // ------------------------------------------------------------------
    // Step 1: Compute B = A^H * A  (TP_DIM_COLS x TP_DIM_COLS, Hermitian).
    //   B[i,j] = sum_{r=0}^{TP_DIM_ROWS-1} conj(A[r,i]) * A[r,j]
    //   A[r,c] = pA[r + c * TP_DIM_ROWS]
    // ------------------------------------------------------------------
    // Augmented matrix [B | I_n], size TP_DIM_COLS x (2*TP_DIM_COLS).
    // Column-major for B part, then column-major for I part.
    // We store it as a flat 2D column-major buffer of width 2*n.
    static const unsigned int n = TP_DIM_COLS;
    TT_DATA aug[n][2 * n]; // aug[row][col], col in [0,n) is B, col in [n,2n) is I

    // Compute B and initialise identity simultaneously.
    for (unsigned int ci = 0; ci < n; ci++) {
        for (unsigned int cj = 0; cj < n; cj++) {
            TT_DATA acc = ref_zero<TT_DATA>();
            for (unsigned int r = 0; r < TP_DIM_ROWS; r++) {
                // A[r, ci] and A[r, cj]
                TT_DATA a_r_ci = pA[r + ci * TP_DIM_ROWS];
                TT_DATA a_r_cj = pA[r + cj * TP_DIM_ROWS];
                acc = ref_add(acc, ref_mul(ref_conj(a_r_ci), a_r_cj));
            }
            aug[ci][cj] = acc; // B[ci,cj]
            // Identity block
            if (ci == cj) {
                aug[ci][n + cj] = ref_one<TT_DATA>(); // diagonal 1
            } else {
                aug[ci][n + cj] = ref_zero<TT_DATA>();
            }
        }
    }

    // ------------------------------------------------------------------
    // Step 2: Gauss-Jordan elimination on aug[0..n-1][0..2n-1].
    // We work row-by-row (aug is row-indexed above for convenience).
    // ------------------------------------------------------------------
    for (unsigned int pivot = 0; pivot < n; pivot++) {
        // Find max |real| pivot in column 'pivot' at or below current row.
        unsigned int maxRow = pivot;
        float maxVal = ref_abs(ref_real(aug[pivot][pivot]));
        for (unsigned int r = pivot + 1; r < n; r++) {
            float v = ref_abs(ref_real(aug[r][pivot]));
            if (v > maxVal) {
                maxVal = v;
                maxRow = r;
            }
        }
        // Swap rows pivot <-> maxRow
        if (maxRow != pivot) {
            for (unsigned int c = 0; c < 2 * n; c++) {
                TT_DATA tmp = aug[pivot][c];
                aug[pivot][c] = aug[maxRow][c];
                aug[maxRow][c] = tmp;
            }
        }
        // Scale pivot row to make leading element 1.
        float pivotReal = ref_real(aug[pivot][pivot]);
        if (ref_abs(pivotReal) < 1.0e-30f) {
            // Singular matrix; leave A^+ as zero (rank deficiency).
            for (unsigned int idx = 0; idx < n * TP_DIM_ROWS; idx++) {
                pAp[idx] = ref_zero<TT_DATA>();
            }
            return;
        }
        // For Hermitian B the pivots are real; divide by the real pivot.
        for (unsigned int c = 0; c < 2 * n; c++) {
            aug[pivot][c] = ref_div_real(aug[pivot][c], pivotReal);
        }
        // Eliminate all other rows.
        for (unsigned int r = 0; r < n; r++) {
            if (r == pivot) continue;
            TT_DATA factor = aug[r][pivot];
            for (unsigned int c = 0; c < 2 * n; c++) {
                aug[r][c] = ref_sub(aug[r][c], ref_mul(factor, aug[pivot][c]));
            }
        }
    }
    // The right half aug[r][n..2n-1] now holds B^-1.

    // ------------------------------------------------------------------
    // Step 3: Compute A^+ = B^-1 * A^H.
    //   A^+[i,j] = sum_{k=0}^{n-1} B^-1[i,k] * conj(A[j,k])
    //             = sum_{k=0}^{n-1} aug[i][n+k] * conj(pA[j + k*TP_DIM_ROWS])
    //   i in [0,n), j in [0,TP_DIM_ROWS)
    //
    //   Output stored column-major with leading dimension n = TP_DIM_COLS:
    //     pAp[i + j*n]  for i in [0,n), j in [0,TP_DIM_ROWS)
    // ------------------------------------------------------------------
    for (unsigned int j = 0; j < TP_DIM_ROWS; j++) {
        for (unsigned int i = 0; i < n; i++) {
            TT_DATA acc = ref_zero<TT_DATA>();
            for (unsigned int k = 0; k < n; k++) {
                TT_DATA binv_ik = aug[i][n + k];
                TT_DATA aH_kj = ref_conj(pA[j + k * TP_DIM_ROWS]);
                acc = ref_add(acc, ref_mul(binv_ik, aH_kj));
            }
            pAp[i + j * n] = acc;
        }
    }
}

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf
