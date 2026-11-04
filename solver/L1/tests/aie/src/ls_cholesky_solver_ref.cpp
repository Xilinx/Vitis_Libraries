/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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
/*
ls_cholesky_solver reference model.

Solves Ax = y for x where A is a symmetric positive definite (SPD) square matrix.

Algorithm (matching the UUT's three-stage chain):
  1. Cholesky decomposition  A = L * L^H  with DIAG_INV = 1
       L is lower triangular; diagonal elements store 1/sqrt(d) instead of sqrt(d)
  2. Forward  substitution   L * w = y   (with DIAG_INV = 1 diagonal convention)
  3. Backward substitution   L^H * x = w (with DIAG_INV = 1 diagonal convention)

Column-major storage convention throughout:
  Matrix element (row r, col c) is at flat index c * TP_DIM + r.
*/

#include "adf.h"
#include "device_defs.h"
#include <cmath>
#include "../inc/cholesky_ref_utils.hpp" // getReal, getConj, getScalarAsType, zero<>
#include "../inc/ls_cholesky_solver_ref.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace ls_cholesky_solver {

// ---------------------------------------------------------------------------
// Arithmetic helpers  (avoid operator overloads that may not exist for cfloat)
// ---------------------------------------------------------------------------
template <typename TT_DATA>
static inline TT_DATA mul_ref(TT_DATA a, TT_DATA b) {
    if
        constexpr(std::is_same<TT_DATA, cfloat>::value) {
            TT_DATA r;
            r.real = a.real * b.real - a.imag * b.imag;
            r.imag = a.real * b.imag + a.imag * b.real;
            return r;
        }
    else {
        return a * b;
    }
}

template <typename TT_DATA>
static inline TT_DATA add_ref(TT_DATA a, TT_DATA b) {
    if
        constexpr(std::is_same<TT_DATA, cfloat>::value) {
            TT_DATA r;
            r.real = a.real + b.real;
            r.imag = a.imag + b.imag;
            return r;
        }
    else {
        return a + b;
    }
}

template <typename TT_DATA>
static inline TT_DATA sub_ref(TT_DATA a, TT_DATA b) {
    if
        constexpr(std::is_same<TT_DATA, cfloat>::value) {
            TT_DATA r;
            r.real = a.real - b.real;
            r.imag = a.imag - b.imag;
            return r;
        }
    else {
        return a - b;
    }
}

template <typename TT_DATA>
static inline TT_DATA scale_ref(TT_DATA a, float s) {
    if
        constexpr(std::is_same<TT_DATA, cfloat>::value) {
            TT_DATA r;
            r.real = a.real * s;
            r.imag = a.imag * s;
            return r;
        }
    else {
        return a * s;
    }
}

// ---------------------------------------------------------------------------
// Main kernel function
// ---------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_NUM_FRAMES>
void ls_cholesky_solver_ref<TT_DATA, TP_DIM, TP_NUM_FRAMES>::ls_cholesky_solver_main(
    input_buffer<TT_DATA>& __restrict in_A,
    input_buffer<TT_DATA>& __restrict in_y,
    output_buffer<TT_DATA>& __restrict out_x) {
    const TT_DATA* A_ptr = in_A.data();
    const TT_DATA* y_ptr = in_y.data();
    TT_DATA* x_ptr = out_x.data();

    TT_DATA L[TP_DIM * TP_DIM];
    TT_DATA w[TP_DIM];

    for (unsigned int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        // ------------------------------------------------------------------
        // Stage 1: Cholesky decomposition  A = L * L^H  (DIAG_INV = 1)
        //
        // Column-major: element (row r, col c) stored at index c*TP_DIM + r.
        // Process column c = 0 .. TP_DIM-1:
        //
        //   diag_sq  = real(A(c,c)) - sum_{k<c} real(L(c,k) * conj(L(c,k)))
        //   L(c,c)   = 1 / sqrt(diag_sq)                [DIAG_INV = 1]
        //   L(r,c)   = [A(r,c) - sum_{k<c} L(r,k)*conj(L(c,k))] * (1/sqrt(diag_sq))
        //   for r = c+1 .. TP_DIM-1
        // ------------------------------------------------------------------
        for (unsigned int i = 0; i < TP_DIM * TP_DIM; i++) {
            L[i] = zero<TT_DATA>();
        }

        for (unsigned int c = 0; c < TP_DIM; c++) {
            // Diagonal element: diag_sq = A(c,c) - sum|L(c,k)|^2
            float diag_sq = getReal<TT_DATA>(A_ptr[c * TP_DIM + c]);
            for (unsigned int k = 0; k < c; k++) {
                TT_DATA Lck = L[k * TP_DIM + c]; // L(c,k): col-major index k*DIM + c
                diag_sq -= getReal<TT_DATA>(mul_ref(Lck, getConj<TT_DATA>(Lck)));
            }
            float inv_sqrt = 1.0f / std::sqrt(diag_sq);
            L[c * TP_DIM + c] = getScalarAsType<TT_DATA>(inv_sqrt); // L(c,c)

            // Sub-diagonal elements in column c
            for (unsigned int r = c + 1; r < TP_DIM; r++) {
                TT_DATA sum = zero<TT_DATA>();
                for (unsigned int k = 0; k < c; k++) {
                    TT_DATA Lrk = L[k * TP_DIM + r]; // L(r,k)
                    TT_DATA Lck = L[k * TP_DIM + c]; // L(c,k)
                    sum = add_ref(sum, mul_ref(Lrk, getConj<TT_DATA>(Lck)));
                }
                // A(r,c) is at col-major index c*TP_DIM + r
                TT_DATA arc = sub_ref(A_ptr[c * TP_DIM + r], sum);
                L[c * TP_DIM + r] = scale_ref(arc, inv_sqrt); // L(r,c)
            }
        }

        // ------------------------------------------------------------------
        // Stage 2: Forward substitution  L * w = y  (DIAG_INV = 1)
        //
        // For i = 0 .. TP_DIM-1:
        //   w[i] = (y[i] - sum_{j<i} L(i,j) * w[j]) * L(i,i)
        // L(i,j) at col-major index j*TP_DIM + i.
        // L(i,i) = 1/sqrt(d_i), so multiplying by it applies the inverse diagonal.
        // ------------------------------------------------------------------
        for (unsigned int i = 0; i < TP_DIM; i++) {
            TT_DATA sum = zero<TT_DATA>();
            for (unsigned int j = 0; j < i; j++) {
                sum = add_ref(sum, mul_ref(L[j * TP_DIM + i], w[j])); // L(i,j)*w[j]
            }
            float diag = getReal<TT_DATA>(L[i * TP_DIM + i]); // 1/sqrt(d_i)
            w[i] = scale_ref(sub_ref(y_ptr[i], sum), diag);
        }

        // ------------------------------------------------------------------
        // Stage 3: Backward substitution  L^H * x = w  (DIAG_INV = 1)
        //
        // L^H(i,j) = conj(L(j,i)) for j >= i.
        // L(j,i) at col-major index i*TP_DIM + j.
        //
        // For i = TP_DIM-1 .. 0:
        //   x[i] = (w[i] - sum_{j>i} conj(L(j,i)) * x[j]) * L(i,i)
        // ------------------------------------------------------------------
        for (int si = (int)TP_DIM - 1; si >= 0; si--) {
            unsigned int i = (unsigned int)si;
            TT_DATA sum = zero<TT_DATA>();
            for (unsigned int j = i + 1; j < TP_DIM; j++) {
                TT_DATA Lji = L[i * TP_DIM + j]; // L(j,i) at col-major index i*DIM+j
                sum = add_ref(sum, mul_ref(getConj<TT_DATA>(Lji), x_ptr[j]));
            }
            float diag = getReal<TT_DATA>(L[i * TP_DIM + i]); // 1/sqrt(d_i)
            x_ptr[i] = scale_ref(sub_ref(w[i], sum), diag);
        }

        // Advance frame pointers
        A_ptr += TP_DIM * TP_DIM;
        y_ptr += TP_DIM;
        x_ptr += TP_DIM;
    }
}

} // namespace ls_cholesky_solver
} // namespace aie
} // namespace solver
} // namespace xf
