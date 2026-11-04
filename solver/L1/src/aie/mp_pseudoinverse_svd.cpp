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

#include "mp_pseudoinverse_svd.hpp"
#include "aie_api/aie.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

// ---------------------------------------------------------------------------
// Scalar helper for type-generic construction of a TT_DATA value from a
// single real float.  Template specialisations cover both float and cfloat
// without requiring vector intrinsics, so the kernel compiles for x86sim
// as well as the AIE device targets.
// ---------------------------------------------------------------------------

/** Construct a TT_DATA "real-only" value from a float scalar. */
template <typename TT>
static INLINE_DECL TT make_real_val(float v);
template <>
INLINE_DECL float make_real_val<float>(float v) {
    return v;
}
template <>
INLINE_DECL cfloat make_real_val<cfloat>(float v) {
    return {v, 0.0f};
}

// ---------------------------------------------------------------------------
// Kernel mp_pseudoinverse_svd_main()
// ---------------------------------------------------------------------------
//
// Computes T = V * diag(S^{-1}) by scaling each column of V by the scalar
// S^{-1}[k], instead of materialising the kStoreCols x kStoreCols diagonal
// matrix and running a general matrix multiply against it -- which would
// spend the overwhelming majority of its MACs multiplying by the zero
// off-diagonal entries (only kStoreCols of the kStoreCols^3 MACs a real GEMM
// would perform are non-zero).
//
//   S^{-1}[k] = 1/S[k]  if S[k] > 1e-30
//             = 0        otherwise   (truncated pseudoinverse)
//   T[:,k]    = V[:,k] * S^{-1}[k]
//
// If TP_DIAG_INV=1, the upstream SVD is assumed to have been configured with
// its own TP_DIAG_INV=1, so inS already holds 1/S (with SVD's own kEpsilon
// near-zero guard already applied) -- this kernel then skips its own
// inversion and threshold entirely and uses inS directly. This mirrors the
// cholesky/substitution TP_DIAG_INV optimization: the reciprocal SVD needs
// anyway (to normalize U) is reused instead of being recomputed here.
//
// V (and therefore T) is col-major, so column k is the contiguous run of
// kStoreCols elements starting at offset k*kStoreCols. Each column is scaled
// with vectorized ::aie::mul(scalar, vector) broadcast-multiplies -- the
// same "scalar broadcast x vector" idiom used by matrix_mult_vecapi.cpp,
// which maps to the native broadcast-multiply hardware instruction on every
// AIE architecture (unlike plain C++ scalar float arithmetic, which lowers
// to softfloat library calls on AIE-ML/AIE-MLv2 -- see hw_mul() in
// svd_utils.hpp). For TT_DATA = cfloat, scaling by sInvVal (real value,
// zero imaginary part) via the complex multiply correctly degenerates to
// real scaling of both components: (a+bi)*(s+0i) = as+bsi.
//
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS, unsigned int TP_DIAG_INV>
NOINLINE_DECL void mp_pseudoinverse_svd<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_DIAG_INV>::mp_pseudoinverse_svd_main(
    input_buffer<float>& __restrict inS,
    input_buffer<TT_DATA>& __restrict inV,
    output_buffer<TT_DATA>& __restrict outT) {
    using dataVec_t = ::aie::vector<TT_DATA, kVecSizeIO>;

    float* __restrict pS = inS.data();
    TT_DATA* __restrict pV = inV.data();
    TT_DATA* __restrict pT = outT.data();

    // All kStoreCols positions are checked (not just the first TP_DIM_COLS)
    // to handle the case where the Jacobi SVD, after a limited number of
    // passes, has not yet sorted all non-zero singular values into the first
    // TP_DIM_COLS positions.
    for (unsigned int k = 0; k < kStoreCols; ++k) chess_prepare_for_pipelining chess_loop_count(kStoreCols) {
            float sv = pS[k];
            float sInv;
            if
                constexpr(TP_DIAG_INV) {
                    // sv is already 1/S[k] (thresholded by SVD's own kEpsilon) -- nothing to do.
                    sInv = sv;
                }
            else {
#if defined(__AIENGINE__) && !defined(__X86SIM__)
                // Use the hardware vector reciprocal intrinsic instead of scalar
                // "1.0f / sv".  The scalar form pulls in three Berkeley SoftFloat
                // library tables (softfloat_countLeadingZeros8 [256 B],
                // softfloat_approxRecip_1k0s [32 B], softfloat_approxRecip_1k1s
                // [32 B]) totalling 320 bytes of kernel-tile heap.  With alignment
                // padding for the 1-byte local variable that coexists in the same
                // stack frame, the tile's 357-byte heap budget is exceeded by 3 B
                // causing an aiecompiler "Heap Overflow" error.
                // ::aie::inv() uses the native VLIW float-reciprocal instruction and
                // requires no runtime lookup tables.
                // Notes:
                //  - The leading '::' is required because this code is inside the
                //    xf::solver::aie namespace; without it the compiler resolves 'aie'
                //    to xf::solver::aie rather than the global AIE API namespace.
                //  - The !defined(__X86SIM__) guard excludes the x86sim pthread build
                //    which also defines __AIENGINE__ but uses adf/x86sim/aieNative.h
                //    (not aie_core.h) and has no tile-heap budget constraints.
                sInv = 0.0f;
                if (sv > 1.0e-30f) {
                    ::aie::vector<float, 8> vSv = ::aie::broadcast<float, 8>(sv);
                    sInv = ::aie::inv(vSv)[0];
                }
#else
                sInv = (sv > 1.0e-30f) ? (1.0f / sv) : 0.0f;
#endif
            }
            TT_DATA sInvVal = make_real_val<TT_DATA>(sInv);

            TT_DATA* __restrict colV = pV + k * kStoreCols;
            TT_DATA* __restrict colT = pT + k * kStoreCols;
            for (unsigned int r = 0; r < kStoreCols; r += kVecSizeIO)
                chess_prepare_for_pipelining chess_loop_count(kStoreCols / kVecSizeIO) {
                    dataVec_t v = ::aie::load_v<kVecSizeIO>(colV + r);
                    dataVec_t t = ::aie::mul(sInvVal, v).template to_vector<TT_DATA>();
                    ::aie::store_v(colT + r, t);
                }
        }
}

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf
