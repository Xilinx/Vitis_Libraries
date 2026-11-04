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
#ifndef _SOLVERLIB_MATRIX_MULT_VECAPI_HPP_
#define _SOLVERLIB_MATRIX_MULT_VECAPI_HPP_

/*
matrix_mult_vecapi -- vector-API matrix multiply, no `mmul` intrinsic.

Private fallback kernel used by mp_pseudoinverse_svd_graph on AIE-ML and
AIE-MLv2, where the xf::dsp matrix_mult/matrix_mult_graph GEMM core has no
mmul hardware tiling for float (AIE-ML) or cfloat (AIE-ML, AIE-MLv2) -- see
matrix_mult_tiling_scheme.hpp. AIE1 is unaffected and keeps using the
faster mmul-based matrix_mult_graph.

Algorithm: C = A * B  (A: TP_DIM_A x TP_DIM_AB, B: TP_DIM_AB x TP_DIM_B).
For each output row i, for each k, take the scalar A[i,k] and
multiply-accumulate it against the full row vector B[k,:], building a
vector accumulator that holds an entire row of C. No transpose of B and
no reduce_add are needed. This is the same "scalar broadcast x vector"
MAC idiom already used in this library for float/cfloat on AIE-ML/AIE-MLv2:
see applyGivensRotations() in svd_utils.hpp and matrixBlockEliminations()
in cholesky_utils.hpp (::aie::mac(accum, scalar, vector) / ::aie::mul
(scalar, vector) -- these map to the native broadcast-multiply hardware
instruction, unlike plain C++ scalar float arithmetic which lowers to
softfloat library calls on AIE-ML/AIE-MLv2, see svd_utils.hpp hw_mul()).

B must be stored row-major (B[k,:] contiguous, length TP_DIM_B) -- this is
a hard requirement of the algorithm, not a configurable option.
mp_pseudoinverse_svd_graph's sole call site (T * U^H -> A^+) already satisfies
this without any reformatting: B's operand U is stored column-major by the
SVD (U[i,k] = buf[i + k*TP_DIM_ROWS]); the vector this algorithm needs for
"row k of U^H" is conj(column k of U), which is exactly the contiguous
chunk buf[k*TP_DIM_ROWS .. k*TP_DIM_ROWS+TP_DIM_ROWS-1].
A is accessed purely by scalar index, so either leading dimension is
supported at equal cost -- A's operand T = V*diag(S^{-1}) is col-major
(the fused S-reciprocal/V-scale kernel preserves V's layout).

The output leading dimension is configurable: ROW_MAJOR stores are fully
vectorized; COL_MAJOR stores (needed for the A^+ result, since downstream
test scripts assume col-major) scatter each computed row with a
per-element strided store -- the O(M*K*N) compute loop stays fully
vectorized either way, only the O(M*N) final store is scalar in that case.

No cascade support: mp_pseudoinverse_svd_graph only ever instantiates
single-kernel matmuls (TP_CASC_LEN=1), so cascade plumbing is omitted.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

#include <adf.h>
#include <type_traits>
// Declares the real global ::aie vector/accum API. Not pulled in by <adf.h>
// alone in every build config; svd_traits.hpp is deliberately vector-API-free.
// Without this, ::aie below resolves to our own xf::solver::aie namespace
// instead of the real one. svd.hpp includes this for the same reason.
#include "aie_api/aie.hpp"
#include "svd_traits.hpp"

using namespace adf;

#ifndef ROW_MAJOR
#define ROW_MAJOR 0
#endif // ROW_MAJOR
#ifndef COL_MAJOR
#define COL_MAJOR 1
#endif // COL_MAJOR

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

using namespace xf::solver::aie::svd; // fnIsComplexDataType, fnVecSampleNumIO

// ---------------------------------------------------------------------------
// Accumulator type trait (float -> accfloat, cfloat -> caccfloat).
// Mirrors the identical trait already duplicated locally per kernel family
// in this library (see accType_t in cholesky_traits.hpp).
// ---------------------------------------------------------------------------
template <typename TT>
struct vecapi_acc_t {
    using type = accfloat;
};
template <>
struct vecapi_acc_t<cfloat> {
    using type = caccfloat;
};
template <typename TT>
using vecapiAccType_t = typename vecapi_acc_t<TT>::type;

/**
 * @brief Vector-API matrix multiply: C = A * B (no mmul intrinsic).
 *
 * @tparam TT_DATA_A   Scalar type of A: float or cfloat.
 * @tparam TT_DATA_B   Scalar type of B: float or cfloat.
 * @tparam TT_OUT_DATA Scalar type of C: float or cfloat.
 *                     Must equal TT_DATA_A and TT_DATA_B (homogeneous only).
 * @tparam TP_DIM_A    Rows of A / rows of C (M).
 * @tparam TP_DIM_AB   Shared/contraction dimension (K).
 * @tparam TP_DIM_B    Cols of B / cols of C (N). Must be a multiple of
 *                     fnVecSampleNumIO<TT_DATA_B>() (8 for float, 4 for
 *                     cfloat -- fixed 256-bit width, safe on every
 *                     architecture regardless of native vector width).
 * @tparam TP_DIM_A_LEADING   ROW_MAJOR or COL_MAJOR (A is scalar-indexed;
 *                             both cost the same).
 * @tparam TP_DIM_OUT_LEADING ROW_MAJOR (vectorized store) or COL_MAJOR
 *                             (scalar strided store).
 * @tparam TP_CONJ_A   Conjugate each element of A before multiplying.
 *                     Requires TT_DATA_A complex.
 * @tparam TP_CONJ_B   Conjugate each element of B before multiplying.
 *                     Requires TT_DATA_B complex.
 **/
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_CONJ_A = 0,
          unsigned int TP_CONJ_B = 0>
class matrix_mult_vecapi {
   public:
    static_assert(std::is_same<TT_DATA_A, float>::value || std::is_same<TT_DATA_A, cfloat>::value,
                  "ERROR: TT_DATA_A must be either float or cfloat.");
    static_assert(std::is_same<TT_DATA_A, TT_DATA_B>::value&& std::is_same<TT_DATA_B, TT_OUT_DATA>::value,
                  "ERROR: matrix_mult_vecapi requires TT_DATA_A == TT_DATA_B == TT_OUT_DATA "
                  "(homogeneous float or cfloat only).");
    static_assert(TP_CONJ_A == 0 || std::is_same<TT_DATA_A, cfloat>::value,
                  "ERROR: TP_CONJ_A=1 requires a complex TT_DATA_A type (cfloat).");
    static_assert(TP_CONJ_B == 0 || std::is_same<TT_DATA_B, cfloat>::value,
                  "ERROR: TP_CONJ_B=1 requires a complex TT_DATA_B type (cfloat).");
    static_assert(TP_DIM_A_LEADING == ROW_MAJOR || TP_DIM_A_LEADING == COL_MAJOR,
                  "ERROR: TP_DIM_A_LEADING must be ROW_MAJOR or COL_MAJOR.");
    static_assert(TP_DIM_OUT_LEADING == ROW_MAJOR || TP_DIM_OUT_LEADING == COL_MAJOR,
                  "ERROR: TP_DIM_OUT_LEADING must be ROW_MAJOR or COL_MAJOR.");

    // Fixed 256-bit vector width (8 float / 4 cfloat), safe on every
    // architecture: TP_DIM_B is guaranteed a multiple of this width by
    // construction at both mp_pseudoinverse_svd call sites (kStoreCols is
    // padded to the SVD's vector-store alignment; TP_DIM_ROWS is required
    // to be a multiple of the SVD's IO vector size). The architecture's
    // native max vector width (16 float / 8 cfloat on AIE-MLv2) is NOT
    // used here because it can exceed TP_DIM_ROWS's alignment guarantee.
    static constexpr unsigned int kVecSize = fnVecSampleNumIO<TT_DATA_B>();
    static_assert(TP_DIM_B % kVecSize == 0,
                  "ERROR: TP_DIM_B must be a multiple of fnVecSampleNumIO<TT_DATA_B>() "
                  "(8 for float, 4 for cfloat).");

    static void registerKernelClass() { REGISTER_FUNCTION(matrix_mult_vecapi::matrix_mult_vecapi_main); }

    /**
     * @param inWindowA  A operand (TP_DIM_A x TP_DIM_AB, leading dim per TP_DIM_A_LEADING).
     * @param inWindowB  B operand (TP_DIM_AB x TP_DIM_B, row-major).
     * @param outWindow  C = A*B (TP_DIM_A x TP_DIM_B, leading dim per TP_DIM_OUT_LEADING).
     **/
    void matrix_mult_vecapi_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                 input_buffer<TT_DATA_B>& __restrict inWindowB,
                                 output_buffer<TT_OUT_DATA>& __restrict outWindow);
};

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_MATRIX_MULT_VECAPI_HPP_
