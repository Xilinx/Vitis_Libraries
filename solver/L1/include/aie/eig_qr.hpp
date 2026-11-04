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
#ifndef _SOLVERLIB_EIG_QR_HPP_
#define _SOLVERLIB_EIG_QR_HPP_

/*
Eigenvalue-extraction kernel -- Stage 2 of the eigenvalue solver.

Computes the eigenvalues of a TP_DIM x TP_DIM upper-Hessenberg
matrix H (as produced by hessenberg.hpp) using iterative QR decomposition
with a simple (Wilkinson-style) shift and deflation:

  repeat up to TP_PASSES times:
    mu    = H(n,n)                      -- shift = current bottom-right entry
    H    -= mu*I
    H     = Q*R                         -- QR-decompose via Givens rotations
                                           (cheap: H is Hessenberg, so each
                                           column needs only one rotation)
    H     = R*Q                         -- recombine
    H    += mu*I
    if subdiagonal H(n,n-1) has converged to ~0: deflate (n -= 1)
    if n == 0: all eigenvalues found, stop early

Converged diagonal entries are the eigenvalues, written to the output vector
E in descending index order as they deflate.

Numerical note (TT_DATA=float only): the QR algorithm in real arithmetic
converges to a real Schur form, which is only fully diagonal (i.e. only
ever fully deflates to 1x1 blocks) when every eigenvalue of the input is
real. A matrix with a complex-conjugate eigenvalue pair leaves a 2x2 block
that never deflates within this algorithm; TT_DATA=float is therefore only
valid for matrices whose eigenvalues are all real (e.g. symmetric/normal
matrices) -- see the library documentation for this constraint. TT_DATA=
cfloat has no such restriction.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

#include <adf.h>
#include "eigenvalue_traits.hpp"

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

/**
 * @brief Eigenvalue-extraction kernel (iterative QR with shift and deflation).
 *
 * Inputs:
 *   in[0]  H -- upper Hessenberg matrix (TP_DIM x TP_DIM, TT_DATA),
 *               as produced by the hessenberg kernel.
 *
 * Outputs:
 *   out[0] E -- eigenvalues of H (= eigenvalues of the original A)
 *               (TP_DIM elements, TT_DATA).
 *
 * @tparam TT_DATA      Data type: float or cfloat.
 * @tparam TP_DIM       Dimension of the original square matrix A
 *                      (= number of eigenvalues).
 * @tparam TP_PASSES    Maximum number of QR sweep iterations to perform
 *                      (with early exit once all eigenvalues have deflated).
 *                      More passes improve convergence for larger/harder
 *                      matrices at the cost of higher latency.
 **/
template <typename TT_DATA, unsigned int TP_DIM, unsigned int TP_PASSES>
class eig_qr {
   public:
    static constexpr unsigned int kN = TP_DIM;
    static constexpr unsigned int kVecSizeIO = fnVecSampleNumIO<TT_DATA>();

    // Convergence tolerance for deflation (relative to local diagonal magnitude).
    static constexpr float kTolerance = 1e-3f;

    static constexpr unsigned int kInWindowVsize = kN * kN;
    static constexpr unsigned int kOutWindowVsize = kN;

    // Internal Q/R scratch buffers, constructor-injected as array references.
    // Passing them in (rather than declaring plain kernel-class members) allows
    // the surrounding graph to place them in specific memory banks if needed via
    // REGISTER_PARAMETER.
    TT_DATA (&Q)[kN * kN]; // Q matrix from QR decomposition (column-major)
    TT_DATA (&R)[kN * kN]; // R matrix from QR decomposition (row-major, for SIMD efficiency)

    // Constructor - accepts array references for buffer storage
    eig_qr(TT_DATA (&Q_in)[kN * kN], TT_DATA (&R_in)[kN * kN]);

    // Main kernel function
    void eig_qr_main(input_buffer<TT_DATA>& __restrict sig_i, output_buffer<TT_DATA>& __restrict sig_o);

    // Registration for AI Engine
    static void registerKernelClass() {
        REGISTER_FUNCTION(eig_qr::eig_qr_main);
        REGISTER_PARAMETER(Q);
        REGISTER_PARAMETER(R);
    }

   private:
    // QR algorithm with shift and deflation
    void qr_algorithm(TT_DATA* A, TT_DATA* D);

    // Update diagonal: A := A + mu*I (pass negate(mu) to subtract)
    void update_diagonal(TT_DATA* A, unsigned n, TT_DATA mu);

    // Perform QR iteration: decompose A = Q*R using Givens rotations
    void qr_iteration(TT_DATA* A, unsigned n);

    // Perform RQ update: A := R*Q
    void rq_update(TT_DATA* A, unsigned n);

    // Check for deflation (convergence of subdiagonal elements)
    bool check_deflation(TT_DATA* A, unsigned n, unsigned& deflate_idx, TT_DATA* D);

    // Helper: Apply Givens rotations to two rows/columns of data
    void apply_Givens_rotations(TT_DATA* data0, TT_DATA* data1, TT_DATA p1, TT_DATA p2, TT_DATA p3, TT_DATA p4);

    // Helper: Initialize array to zero using SIMD
    void initialize_zero(TT_DATA* ptr, unsigned size);
};

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_EIG_QR_HPP_
