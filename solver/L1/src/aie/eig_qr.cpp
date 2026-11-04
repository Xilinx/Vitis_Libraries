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

#include "eig_qr.hpp"
#include "eigenvalue_utils.hpp"

// Note: deliberately NOT writing "using namespace aie;" here -- see the
// identical note at the top of hessenberg.cpp. Every AIE API call below is
// explicitly "::aie::"-qualified.

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
eig_qr<TT_DATA, TP_DIM, TP_PASSES>::eig_qr(TT_DATA (&Q_in)[kN * kN], TT_DATA (&R_in)[kN * kN]) : Q(Q_in), R(R_in) {}

// ------------------------------------------------------------
// Helper: Initialize array to zero using SIMD
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
INLINE_DECL void eig_qr<TT_DATA, TP_DIM, TP_PASSES>::initialize_zero(TT_DATA* ptr, unsigned size) {
    auto it = ::aie::begin_restrict_vector<kVecSizeIO>(ptr);
    // size varies by call site (kN for the output vector, kN*kN for Q/R) but
    // is always one of those two compile-time-constant values -- never
    // runtime/data-dependent -- so a range (not a single chess_loop_count) is
    // the correct hint for this noinline, multi-call-site function.
    for (unsigned ii = 0; ii < size / kVecSizeIO; ii++)
        chess_prepare_for_pipelining chess_loop_range(kN / kVecSizeIO, (kN * kN) / kVecSizeIO) {
            *it++ = ::aie::zeros<TT_DATA, kVecSizeIO>();
        }
}

// ------------------------------------------------------------
// Helper: Apply Givens Rotations to Two Rows/Columns
// ------------------------------------------------------------
// Applies Givens rotation transformation to two consecutive rows/columns:
//   data0_new = p1 * data0 + p3 * data1
//   data1_new = p2 * data0 + p4 * data1

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
INLINE_DECL void eig_qr<TT_DATA, TP_DIM, TP_PASSES>::apply_Givens_rotations(
    TT_DATA* data0, TT_DATA* data1, TT_DATA p1, TT_DATA p2, TT_DATA p3, TT_DATA p4) {
    alignas(32) TT_DATA tmpRk0[kN];
    alignas(32) TT_DATA tmpRk1[kN];

    auto itrD0 = ::aie::begin_restrict_vector<kVecSizeIO>(data0);
    auto itrD1 = ::aie::begin_restrict_vector<kVecSizeIO>(data1);
    auto itK0 = ::aie::begin_restrict_vector<kVecSizeIO>(tmpRk0);
    auto itK1 = ::aie::begin_restrict_vector<kVecSizeIO>(tmpRk1);

    auto p1_bc = ::aie::broadcast<TT_DATA, kVecSizeIO>(p1);
    auto p2_bc = ::aie::broadcast<TT_DATA, kVecSizeIO>(p2);
    auto p3_bc = ::aie::broadcast<TT_DATA, kVecSizeIO>(p3);
    auto p4_bc = ::aie::broadcast<TT_DATA, kVecSizeIO>(p4);

    // First pass: compute p1*data0 and p2*data0, store in temporaries
    for (unsigned jj = 0; jj < kN / kVecSizeIO; jj++) chess_prepare_for_pipelining chess_loop_count(kN / kVecSizeIO) {
            auto d0 = *itrD0++;
            *itK0++ =
                ::aie::mac(::aie::from_vector<eigAccType_t<TT_DATA> >(::aie::zeros<TT_DATA, kVecSizeIO>()), p1_bc, d0);
            *itK1++ =
                ::aie::mac(::aie::from_vector<eigAccType_t<TT_DATA> >(::aie::zeros<TT_DATA, kVecSizeIO>()), p2_bc, d0);
        }

    // Second pass: add p3*data1 and p4*data1, write results
    auto itw0 = ::aie::begin_restrict_vector<kVecSizeIO>(data0);
    auto itw1 = ::aie::begin_restrict_vector<kVecSizeIO>(data1);
    itK0 = ::aie::begin_restrict_vector<kVecSizeIO>(tmpRk0);
    itK1 = ::aie::begin_restrict_vector<kVecSizeIO>(tmpRk1);

    for (unsigned jj = 0; jj < kN / kVecSizeIO; jj++) chess_prepare_for_pipelining chess_loop_count(kN / kVecSizeIO) {
            auto d1 = *itrD1++;
            *itw0++ = ::aie::mac(::aie::from_vector<eigAccType_t<TT_DATA> >(*itK0++), p3_bc, d1)
                          .template to_vector<TT_DATA>();
            *itw1++ = ::aie::mac(::aie::from_vector<eigAccType_t<TT_DATA> >(*itK1++), p4_bc, d1)
                          .template to_vector<TT_DATA>();
        }
}

// ------------------------------------------------------------
// Run: Compute eigenvalues of input Hessenberg matrix H using QR algorithm
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
NOINLINE_DECL void eig_qr<TT_DATA, TP_DIM, TP_PASSES>::eig_qr_main(input_buffer<TT_DATA>& __restrict sig_i,
                                                                   output_buffer<TT_DATA>& __restrict sig_o) {
    // Set every invocation, not in the constructor: rounding/saturation mode
    // is tile-scope hardware state, not kernel-scope -- kernels can
    // time-share a tile, so another kernel's invocation may have changed
    // this in between calls to this kernel's eig_qr_main().
    ::aie::set_rounding(::aie::rounding_mode::conv_even);
    ::aie::set_saturation(::aie::saturation_mode::saturate);

    // Matrix to be modified in-place during the QR iterations.
    TT_DATA* A = sig_i.data();

    // Initialize output buffer (eigenvalue vector) to zero.
    initialize_zero(sig_o.data(), kN);

    // Apply QR algorithm with shift and deflation; eigenvalues written directly to sig_o.
    qr_algorithm(A, sig_o.data());
}

// ------------------------------------------------------------
// QR Algorithm with Shift and Deflation
// ------------------------------------------------------------
// Iteratively applies QR decomposition and RQ update until convergence.

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
INLINE_DECL void eig_qr<TT_DATA, TP_DIM, TP_PASSES>::qr_algorithm(TT_DATA* A, TT_DATA* D) {
    unsigned n = kN - 1;
    for (unsigned iter = 0; iter < TP_PASSES; iter++) {
        bool done = check_deflation(A, n, n, D);
        if (done) {
            break;
        }
        TT_DATA mu = A[n * kN + n];
        update_diagonal(A, n, negate<TT_DATA>(mu)); // Subtract mu
        qr_iteration(A, n);
        rq_update(A, n);
        update_diagonal(A, n, mu); // Add mu back
        if (n == 0) {
            break;
        }
    }
    if (n > 0) {
        TT_DATA* __restrict ptrA = A;
        TT_DATA* __restrict ptrD = D;
        // Loop trip count (kN) is fixed and unconditional -- only the work
        // inside the loop is runtime-gated by "i <= n", not the iteration
        // count itself -- so chess_loop_count is still a correct hint here.
        for (unsigned i = 0; i < kN; i++) chess_prepare_for_pipelining chess_loop_count(kN) {
                if (i <= n) {
                    *ptrD++ = *ptrA;
                    ptrA += kN + 1;
                }
            }
    }
}

// ------------------------------------------------------------
// Update Diagonal: A := A + mu*I
// ------------------------------------------------------------
// Adds scalar mu to diagonal elements of the (n+1)x(n+1) active submatrix.
// Matrix A is kN x kN in column-major order. Pass negate(mu) to subtract.

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
INLINE_DECL void eig_qr<TT_DATA, TP_DIM, TP_PASSES>::update_diagonal(TT_DATA* A, unsigned n, TT_DATA mu) {
    TT_DATA* __restrict ptr = A;
    // As above (qr_algorithm): trip count kN is unconditional, only the body
    // is runtime-gated by "i <= n".
    for (unsigned i = 0; i < kN; i++) chess_prepare_for_pipelining chess_loop_count(kN) {
            if (i <= n) {
                *ptr = *ptr + mu;
                ptr += kN + 1;
            }
        }
}

// ------------------------------------------------------------
// QR Iteration: Decompose A = Q*R using Givens rotations
// ------------------------------------------------------------
// For a Hessenberg matrix, each column requires only one Givens rotation.
// Operates on the (n+1)x(n+1) active submatrix but uses zero-padded kN x kN
// storage for SIMD.

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
INLINE_DECL void eig_qr<TT_DATA, TP_DIM, TP_PASSES>::qr_iteration(TT_DATA* A, unsigned n) {
    // ============================================================
    // Step 1: Initialize Q = I (identity) and R = A using zero-padding
    // ============================================================

    // Initialize Q as identity matrix (zero-padded to kN x kN, column-major)
    // Initialize R = A (transpose from column-major A to row-major R)
    initialize_zero(Q, kN * kN);
    initialize_zero(R, kN * kN);
    for (unsigned i = 0, idx = 0; i <= n; i++, idx += kN + 1) {
        Q[idx] = makeReal<TT_DATA>(1.0f);
    }

    for (unsigned row = 0; row <= n; row++) {
        for (unsigned col = 0; col <= n; col++) {
            R[row * kN + col] = A[col * kN + row];
        }
    }

    // ============================================================
    // Step 2: Apply Givens rotations to zero out subdiagonal elements
    // ============================================================
    // For a Hessenberg matrix, only need to zero R(k+1, k) for each column k.

    for (unsigned k = 0; k < n; k++) {
        // Compute Givens rotation to zero out R(k+1, k)
        TT_DATA a = R[k * kN + k];       // R(k, k) - row-major indexing
        TT_DATA b = R[(k + 1) * kN + k]; // R(k+1, k) - element to zero out, row-major
        float r_mag =
            ::aie::sqrt(getReal<TT_DATA>(a) * getReal<TT_DATA>(a) + getImag<TT_DATA>(a) * getImag<TT_DATA>(a) +
                        getReal<TT_DATA>(b) * getReal<TT_DATA>(b) + getImag<TT_DATA>(b) * getImag<TT_DATA>(b));

        // Compute Givens rotation parameters
        TT_DATA c_val, s_val;
        if (r_mag < 1e-12f) {
            c_val = makeReal<TT_DATA>(1.0f);
            s_val = makeReal<TT_DATA>(0.0f);
        } else {
            // c_val = a*scale, s_val = b*scale: scaling a complex value by a
            // real scalar `scale` is exactly `value * makeReal(scale)`
            // (complex multiply degenerates to real scaling when the second
            // operand's imaginary part is zero) -- avoids needing to
            // reconstruct real/imag fields by hand.
            float scale = ::aie::inv(r_mag);
            c_val = a * makeReal<TT_DATA>(scale);
            s_val = b * makeReal<TT_DATA>(scale);
        }

        // ============================================================
        // Apply Givens rotation to R: operate on rows k and k+1
        // ============================================================
        // For each column j, update R(k,j) and R(k+1,j)
        // [ R(k,j)   ]  =  [ conj(c)  conj(s) ] [ R(k,j)   ]
        // [ R(k+1,j) ]     [ -s       c       ] [ R(k+1,j) ]

        TT_DATA c_conj_val = getConj<TT_DATA>(c_val);
        TT_DATA s_conj_val = getConj<TT_DATA>(s_val);
        TT_DATA s_neg_val = negate<TT_DATA>(s_val);
        apply_Givens_rotations(&R[k * kN], &R[(k + 1) * kN], c_conj_val, s_neg_val, s_conj_val, c_val);

        // ============================================================
        // Apply same Givens rotation to Q: operate on columns k and k+1
        // ============================================================
        // For each row i, update Q(i,k) and Q(i,k+1)
        // [ Q(i,k) Q(i,k+1) ] = [ Q(i,k) Q(i,k+1) ] [ c  -conj(s) ]
        //                                           [ s   conj(c) ]
        TT_DATA s_conj_neg_val = negate<TT_DATA>(getConj<TT_DATA>(s_val));
        apply_Givens_rotations(&Q[k * kN], &Q[(k + 1) * kN], c_val, s_conj_neg_val, s_val, c_conj_val);
    }
}

// ------------------------------------------------------------
// RQ Update: A := R*Q
// ------------------------------------------------------------
// Computes A(i,j) = sum_k R(i,k) * Q(k,j) for the full kN x kN matrices
// (zero-padded outside the active (n+1)x(n+1) submatrix).
//
// R is row-major (row i is the contiguous chunk R[i*kN .. i*kN+kN-1]) and Q
// is column-major (column j is the contiguous chunk Q[j*kN .. j*kN+kN-1]),
// so each output element is a plain vectorized dot product of a row of R
// against a column of Q. This dot-product form is used for TT_DATA/kN
// portability, rather than device-specific complex-multiply intrinsics with
// explicit lane-select masks, which do not generalise to `float` or to
// arbitrary kN. Output A is column-major: A[j*kN + i] = A(i,j).

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
INLINE_DECL void eig_qr<TT_DATA, TP_DIM, TP_PASSES>::rq_update(TT_DATA* A, unsigned n) {
    TT_DATA* __restrict ptr_a_col = A; // Start at column 0 of A

    for (unsigned j = 0; j < kN; j++) {
        TT_DATA* colQ = &Q[j * kN]; // Column j of Q, contiguous
        for (unsigned i = 0; i < kN; i++) chess_prepare_for_pipelining chess_loop_count(kN) {
                TT_DATA* rowR = &R[i * kN]; // Row i of R, contiguous
                auto itQ = ::aie::begin_restrict_vector<kVecSizeIO>(colQ);
                auto itR = ::aie::begin_restrict_vector<kVecSizeIO>(rowR);
                auto acc = ::aie::mul(*itR++, *itQ++);
                for (unsigned kk = 1; kk < kN / kVecSizeIO; kk++) {
                    acc = ::aie::mac(acc, *itR++, *itQ++);
                }
                ptr_a_col[i] = ::aie::reduce_add(acc.template to_vector<TT_DATA>());
            }
        ptr_a_col += kN; // Move to next column of A
    }
}

// ------------------------------------------------------------
// Check Deflation
// ------------------------------------------------------------
// Checks if any subdiagonal element has converged to zero. Stores converged
// eigenvalues in output buffer D. Returns true if deflation occurred, with
// deflate_idx indicating new problem size.

template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
INLINE_DECL bool eig_qr<TT_DATA, TP_DIM, TP_PASSES>::check_deflation(TT_DATA* A,
                                                                     unsigned n,
                                                                     unsigned& deflate_idx,
                                                                     TT_DATA* D) {
    // Check from bottom-right upward for converged eigenvalues.
    unsigned m = n;

    while (m > 0) {
        // A(m,m-1) is the subdiagonal element in column m-1, row m.
        unsigned idx_subdiag = (m - 1) * kN + m;         // Column m-1, row m
        unsigned idx_diag_prev = (m - 1) * kN + (m - 1); // A(m-1, m-1)
        unsigned idx_diag_curr = m * kN + m;             // A(m, m)
        TT_DATA subdiag = A[idx_subdiag];
        TT_DATA diag_prev = A[idx_diag_prev];
        TT_DATA diag_curr = A[idx_diag_curr];

        float subdiag_mag = magnitude<TT_DATA>(subdiag);
        float diag_prev_mag = magnitude<TT_DATA>(diag_prev);
        float diag_curr_mag = magnitude<TT_DATA>(diag_curr);

        if (subdiag_mag < kTolerance * (diag_prev_mag + diag_curr_mag)) {
            // Eigenvalue has converged, store it in output buffer.
            D[m] = diag_curr;

            // Deflate: reduce problem size.
            m = m - 1;

            // Check if we've reduced to size 1.
            if (m == 0) {
                D[0] = A[0];
                deflate_idx = 0;
                return true; // All eigenvalues found
            }
        } else {
            break; // No deflation possible, continue iterations
        }
    }
    deflate_idx = m;
    return (m == 0); // Return true if converged to single element
}

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf
