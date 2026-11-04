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
#ifndef _SOLVERLIB_HESSENBERG_HPP_
#define _SOLVERLIB_HESSENBERG_HPP_

/*
Hessenberg reduction kernel -- Stage 1 of the eigenvalue solver.

Reduces a TP_DIM x TP_DIM matrix A to upper Hessenberg form H using
Householder reflections (left and right), preserving eigenvalues while making
the subsequent iterative-QR eigenvalue extraction (see eig_qr.hpp) far
cheaper: a QR step on a Hessenberg matrix needs only one Givens rotation per
column, versus a full Householder QR on a dense matrix.

Storage layout note: the graph-level contract (see eigenvalue_graph.hpp)
specifies A is row-major. This kernel's indexing treats its input buffer as
column-major. Reading a row-major buffer with a column-major index formula
computes with A^T instead of A, but eigenvalues(A) = eigenvalues(A^T) always (plain transpose, no
conjugation, true for both real and complex matrices) -- so this kernel
correctly produces the eigenvalues of A without requiring any index-math
changes.
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
 * @brief Hessenberg reduction kernel.
 *
 * Reduces the TP_DIM x TP_DIM input matrix A to upper Hessenberg
 * form H via Householder reflections: H = Q^H A Q for some unitary Q, so A
 * and H share the same eigenvalues.
 *
 * Inputs:
 *   in[0]  A -- input matrix (TP_DIM x TP_DIM, TT_DATA).
 *
 * Outputs:
 *   out[0] H -- upper Hessenberg matrix (TP_DIM x TP_DIM, TT_DATA).
 *
 * @tparam TT_DATA      Data type: float or cfloat.
 * @tparam TP_DIM       Dimension of the square matrix A (= number of eigenvalues).
 **/
template <typename TT_DATA, unsigned int TP_DIM>
class hessenberg {
   public:
    static constexpr unsigned int kN = TP_DIM;
    static constexpr unsigned int kVecSizeIO = fnVecSampleNumIO<TT_DATA>();

    static constexpr unsigned int kInWindowVsize = kN * kN;
    static constexpr unsigned int kOutWindowVsize = kN * kN;

    // Constructor
    hessenberg();

    // Main kernel function
    void hessenberg_main(input_buffer<TT_DATA>& __restrict sig_i, output_buffer<TT_DATA>& __restrict sig_o);

    // Registration for AI Engine
    static void registerKernelClass() { REGISTER_FUNCTION(hessenberg::hessenberg_main); }

   private:
    // Working vector storage (aligned for SIMD access)
    alignas(32) TT_DATA v[kN]; // Householder vector
    alignas(32) TT_DATA w[kN]; // Temporary work vector

    // Compute squared vector norm:
    float squared_vector_norm(const TT_DATA* x);

    // Scale vector by scalar:
    void scale_vector(TT_DATA* x, TT_DATA alpha);

    // Compute Vector-Matrix Inner Product v^T * conj(M)
    void vm_inner_product(const TT_DATA* v, const TT_DATA* M, TT_DATA* result);

    // Column based rank-1 update: M = M - v * w'
    void rank1_update(TT_DATA* M, const TT_DATA* v, const TT_DATA* w);

    // Compute Matrix-Vector Product M * v
    void mv_product(const TT_DATA* M, const TT_DATA* v, TT_DATA* result);

    // Hessenberg reduction using Householder reflectors
    void hessenberg_reduction(TT_DATA* H);

    // Compute Householder vector for column k (below diagonal)
    // Returns beta = 2 / (v' * v)
    float compute_householder_vector(TT_DATA* H, unsigned k);

    // Apply left Householder reflection: H = (I - beta*v*v') * H
    void apply_left_householder(TT_DATA* H, unsigned k, float beta);

    // Apply right Householder reflection: H = H * (I - beta*v*v')
    void apply_right_householder(TT_DATA* H, unsigned k, float beta);
};

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_HESSENBERG_HPP_
