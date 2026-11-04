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
#ifndef _SOLVERLIB_MP_PSEUDOINVERSE_SVD_HPP_
#define _SOLVERLIB_MP_PSEUDOINVERSE_SVD_HPP_

/*
Moore-Penrose Pseudoinverse -- fused S-reciprocal / V-scale kernel.

T = V * diag(S^{-1}) is mathematically just a per-column scale of V: since
diag(S^{-1}) is zero off the diagonal, (V * S^{-1})[i,j] = V[i,j] * S^{-1}[j]
-- the only term in the dot product that survives is k=j. Building the full
kStoreCols x kStoreCols diagonal matrix and feeding it to a general matrix
multiply therefore wastes the large majority of the multiply-accumulates:
of the kStoreCols^3 MACs a real GEMM would perform, only kStoreCols^2
(the diagonal contributions) are non-zero.

This kernel computes the same T directly, without ever materialising
diag(S^{-1}) or invoking a matrix-multiply kernel:

    S^{-1}[k] = 1 / S[k]   if |S[k]| > threshold
              = 0           otherwise   (truncated pseudoinverse)
    T[:,k]    = V[:,k] * S^{-1}[k]      (scale column k of V by the scalar)

TP_DIAG_INV=1 (paired with an SVD instance also configured with
TP_DIAG_INV=1): S already arrives as 1/S (SVD computes this anyway, as a
byproduct of normalizing its own U output), so this kernel skips the
division/threshold above entirely and uses S directly. This is the same
optimization pattern as cholesky/substitution's TP_DIAG_INV. mp_pseudoinverse_svd_
graph.hpp always pairs the two (TP_DIAG_INV=1 on both), since it owns both
ends of this pipe; svd_graph on its own defaults to TP_DIAG_INV=0 for
standalone/conventional use.

The overall pseudoinverse pipeline is:

  SVD(A) -> (U, S, V)
  this kernel: S, V  ->  T = V * diag(S^{-1})   (kStoreCols x kStoreCols, col-major, TT_DATA)
  MatMul:      T * U^T  ->  A^+                 (kStoreCols x TP_DIM_ROWS)

V is col-major (V[i,k] = buf[i + k*kStoreCols]); scaling column k by a scalar
does not change that layout, so T is emitted col-major too (matching V) --
unlike the diagonal-matmul approach this replaces, whose matrix_mult output
was row-major.

The kStoreCols-padded A^+ output is then stripped to TP_DIM_COLS x TP_DIM_ROWS
in the post-processing (depadding) step before comparison and validation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

#include <adf.h>
#include "svd_traits.hpp"

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

using namespace xf::solver::aie::svd; // for trait helpers

/**
 * @brief Fused S-reciprocal / V-column-scale kernel.
 *
 * Computes T = V * diag(S^{-1}) without ever materialising the
 * kStoreCols x kStoreCols diagonal matrix: since diag(S^{-1}) is zero
 * off-diagonal, this reduces to scaling column k of V by the scalar
 * S^{-1}[k], for each k. Replaces what was previously a separate
 * S-inversion kernel feeding a general matrix-multiply kernel.
 *
 * Inputs:
 *   in[0]  inS   -- vector from SVD (kStoreCols floats, real): singular
 *                    values (TP_DIAG_INV=0) or their reciprocals already
 *                    (TP_DIAG_INV=1).
 *   in[1]  inV   -- V from SVD (kStoreCols x kStoreCols, col-major, TT_DATA)
 *
 * Outputs:
 *   out[0] outT  -- T = V * diag(S^{-1}) (kStoreCols x kStoreCols, col-major,
 *                    TT_DATA), i.e. column k of V scaled by 1/S[k]
 *                    (0 for near-zero singular values).
 *
 * @tparam TT_DATA   Data type: float or cfloat.
 * @tparam TP_DIM_ROWS  Number of rows of the original matrix A.
 * @tparam TP_DIM_COLS  Number of columns of the original matrix A
 *                       (= number of valid singular values).
 * @tparam TP_DIAG_INV  Set to 1 when paired with an SVD instance configured
 *                       with its own TP_DIAG_INV=1 (inS already holds 1/S) --
 *                       this kernel then skips its own reciprocal entirely
 *                       and uses inS directly, mirroring the
 *                       cholesky/substitution TP_DIAG_INV optimization. Set
 *                       to 0 (the normal case) when inS holds singular
 *                       values and this kernel must invert them itself.
 *                       Note: when TP_DIAG_INV=1, the near-zero-singular-value
 *                       truncation threshold is whatever the upstream SVD
 *                       used (not this kernel's own 1e-30) -- see
 *                       mp_pseudoinverse_svd.cpp.
 **/
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS, unsigned int TP_DIAG_INV>
class mp_pseudoinverse_svd {
   public:
    // -----------------------------------------------------------------------
    // Dimension constants (matching the SVD graph padding conventions)
    // -----------------------------------------------------------------------
    static constexpr bool kIsComplex = fnIsComplexDataType<TT_DATA>();
    static constexpr unsigned int kVecSizeIO = fnVecSampleNumIO<TT_DATA>();
    static constexpr unsigned int kStoreAlign = fnVecStoreAlign<TT_DATA>();
    // Round TP_DIM_COLS up to even (Jacobi schedule requirement)
    static constexpr unsigned int kSchedCols = TP_DIM_COLS + (TP_DIM_COLS % 2);
    // Round up to the vector store-alignment boundary used by the SVD kernel
    static constexpr unsigned int kStoreCols = ((kSchedCols + kStoreAlign - 1) / kStoreAlign) * kStoreAlign;

    // kStoreCols is always a multiple of kStoreAlign, which is itself always
    // a multiple of kVecSizeIO (256-bit IO vector width) -- see
    // fnVecStoreAlign()/fnVecSampleNumIO() in svd_traits.hpp. Each
    // kStoreCols-long column of V can therefore be scaled in whole
    // kVecSizeIO-wide vector chunks with no remainder handling.
    static_assert(kStoreCols % kVecSizeIO == 0, "ERROR: kStoreCols must be a multiple of kVecSizeIO.");

    // Input buffer sizes
    static constexpr unsigned int kInSVsize = kStoreCols;              // S vector (float)
    static constexpr unsigned int kInVVsize = kStoreCols * kStoreCols; // V matrix (TT_DATA, col-major)

    // Output buffer size: T = V * diag(S^{-1}) (TT_DATA, col-major, same shape as V)
    static constexpr unsigned int kOutTVsize = kStoreCols * kStoreCols;

    static void registerKernelClass() { REGISTER_FUNCTION(mp_pseudoinverse_svd::mp_pseudoinverse_svd_main); }

    /**
     * @brief Compute T = V * diag(S^{-1}) by scaling each column of V.
     *
     * @param inS   in[0]: singular-value vector (kStoreCols floats).
     * @param inV   in[1]: V (kStoreCols x kStoreCols, col-major, TT_DATA).
     * @param outT  out[0]: T = V * diag(S^{-1}) (kStoreCols x kStoreCols,
     *                      col-major, TT_DATA).
     **/
    void mp_pseudoinverse_svd_main(input_buffer<float>& __restrict inS,
                                   input_buffer<TT_DATA>& __restrict inV,
                                   output_buffer<TT_DATA>& __restrict outT);
};

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_MP_PSEUDOINVERSE_SVD_HPP_
