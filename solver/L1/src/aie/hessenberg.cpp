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

#include "hessenberg.hpp"
#include "eigenvalue_utils.hpp"

// Note: deliberately NOT writing "using namespace aie;" here -- this file is
// inside the xf::solver::aie::eigenvalue namespace, so an unqualified "aie::"
// would resolve to xf::solver::aie rather than the global AIE API namespace.
// Every AIE API call below is therefore explicitly "::aie::"-qualified (same
// convention already established in mp_pseudoinverse.cpp / matrix_mult_vecapi.cpp).

namespace xf {
namespace solver {
namespace aie {
namespace eigenvalue {

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM>
hessenberg<TT_DATA, TP_DIM>::hessenberg() {}

// ------------------------------------------------------------
// Run: Compute Hessenberg form of input matrix A using Householder reflections
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM>
NOINLINE_DECL void hessenberg<TT_DATA, TP_DIM>::hessenberg_main(input_buffer<TT_DATA>& __restrict sig_i,
                                                                output_buffer<TT_DATA>& __restrict sig_o) {
    // Set every invocation, not in the constructor: rounding/saturation mode
    // is tile-scope hardware state, not kernel-scope -- kernels can
    // time-share a tile, so another kernel's invocation may have changed
    // this in between calls to this kernel's hessenberg_main().
    ::aie::set_rounding(::aie::rounding_mode::conv_even);
    ::aie::set_saturation(::aie::saturation_mode::saturate);

    auto itr = ::aie::begin_restrict_vector<kVecSizeIO>(sig_i);
    auto itw = ::aie::begin_restrict_vector<kVecSizeIO>(sig_o);

    // ============================================================
    // Step 1: Copy input matrix A directly to the output buffer.
    // ============================================================
    for (unsigned ii = 0; ii < (kN * kN) / kVecSizeIO; ii++)
        chess_prepare_for_pipelining chess_loop_count((kN * kN) / kVecSizeIO) { *itw++ = *itr++; }

    // ============================================================
    // Step 2: Reduce the output buffer to Hessenberg form in-place
    // using Householder reflections.
    // ============================================================
    TT_DATA* H = sig_o.data(); // Work directly on the output buffer
    hessenberg_reduction(H);
}

// ------------------------------------------------------------
// Hessenberg Reduction
// ------------------------------------------------------------
// Reduces matrix H to upper Hessenberg form using Householder reflections.
// Processes columns k = 0 to kN-2. For each column, zeros out elements
// below the first subdiagonal.

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL void hessenberg<TT_DATA, TP_DIM>::hessenberg_reduction(TT_DATA* H) {
    for (unsigned k = 0; k < kN - 1; k++) {
        // Compute Householder vector v for column k
        float beta = compute_householder_vector(H, k);

        // Apply left multiplication: H = (I - beta*v*v') * H
        apply_left_householder(H, k, beta);

        // Apply right multiplication: H = H * (I - beta*v*v')
        apply_right_householder(H, k, beta);
    }
}

// ------------------------------------------------------------
// Compute Vector Norm
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL float hessenberg<TT_DATA, TP_DIM>::squared_vector_norm(const TT_DATA* x) {
    // Compute norm using SIMD
    auto norm_acc = ::aie::zeros<eigAccType_t<TT_DATA>, kVecSizeIO>();
    auto itx = ::aie::begin_restrict_vector<kVecSizeIO>(x);
    for (unsigned ii = 0; ii < kN / kVecSizeIO; ii++) chess_prepare_for_pipelining chess_loop_count(kN / kVecSizeIO) {
            auto vec = *itx++;
            if
                constexpr(fnIsComplexDataType<TT_DATA>()) { norm_acc = ::aie::mac(norm_acc, vec, ::aie::op_conj(vec)); }
            else {
                norm_acc = ::aie::mac(norm_acc, vec, vec);
            }
        }
    float norm_sq = getReal<TT_DATA>(::aie::reduce_add(norm_acc.template to_vector<TT_DATA>()));
    return (norm_sq);
}

// ------------------------------------------------------------
// Scale Vector by Scalar
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL void hessenberg<TT_DATA, TP_DIM>::scale_vector(TT_DATA* x, TT_DATA alpha) {
    auto alpha_vec = ::aie::broadcast<TT_DATA, kVecSizeIO>(alpha);
    auto itv = ::aie::begin_restrict_vector<kVecSizeIO>(x);
    auto itw = ::aie::begin_restrict_vector<kVecSizeIO>(x);
    for (unsigned ii = 0; ii < kN / kVecSizeIO; ii++) chess_prepare_for_pipelining chess_loop_count(kN / kVecSizeIO) {
            *itw++ = (::aie::mul(alpha_vec, *itv++)).template to_vector<TT_DATA>();
        }
}

// ------------------------------------------------------------
// Compute Vector-Matrix Inner Product v^T * conj(M)
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL void hessenberg<TT_DATA, TP_DIM>::vm_inner_product(const TT_DATA* v, const TT_DATA* M, TT_DATA* result) {
    auto itM = ::aie::begin_restrict_vector<kVecSizeIO>(M);
    for (unsigned j = 0; j < kN; j++) {
        auto acc = ::aie::zeros<eigAccType_t<TT_DATA>, kVecSizeIO>();
        auto itU = itM;
        auto itv = ::aie::begin_restrict_vector<kVecSizeIO>(v);
        for (unsigned ii = 0; ii < kN / kVecSizeIO; ii++)
            chess_prepare_for_pipelining chess_loop_count(kN / kVecSizeIO) {
                if
                    constexpr(fnIsComplexDataType<TT_DATA>()) { acc = ::aie::mac(acc, *itv++, ::aie::op_conj(*itU++)); }
                else {
                    acc = ::aie::mac(acc, *itv++, *itU++);
                }
            }
        result[j] = ::aie::reduce_add(acc.template to_vector<TT_DATA>());
        itM += kN / kVecSizeIO; // Move to next column
    }
}

// ------------------------------------------------------------
// Column based rank-1 update: M = M - v * w'
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL void hessenberg<TT_DATA, TP_DIM>::rank1_update(TT_DATA* M, const TT_DATA* v, const TT_DATA* w) {
    ::aie::accum<eigAccType_t<TT_DATA>, kVecSizeIO> acc;
    auto itM = ::aie::begin_restrict_vector<kVecSizeIO>(M);
    auto itMw = ::aie::begin_restrict_vector<kVecSizeIO>(M);
    for (unsigned j = 0; j < kN; j++) {
        auto w_j_conj = ::aie::broadcast<TT_DATA, kVecSizeIO>(getConj<TT_DATA>(w[j]));
        auto itU = itM;
        auto itv = ::aie::begin_restrict_vector<kVecSizeIO>(v);
        for (unsigned ii = 0; ii < kN / kVecSizeIO; ii++)
            chess_prepare_for_pipelining chess_loop_count(kN / kVecSizeIO) {
                acc.from_vector(*itU++);
                *itMw++ = ::aie::msc(acc, *itv++, w_j_conj).template to_vector<TT_DATA>();
            }
        itM += kN / kVecSizeIO; // Move to next column
    }
}

// ------------------------------------------------------------
// Compute Matrix-Vector Product M * v
// ------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL void hessenberg<TT_DATA, TP_DIM>::mv_product(const TT_DATA* M, const TT_DATA* v, TT_DATA* result) {
    for (unsigned i = 0; i < kN; i++) {
        auto acc = ::aie::zeros<eigAccType_t<TT_DATA>, kVecSizeIO>();
        auto itH = ::aie::begin(M, kN * kN) + i; // Row i
        auto itv = ::aie::begin_restrict_vector<kVecSizeIO>(v);

        for (unsigned jj = 0; jj < kN / kVecSizeIO; jj++)
            chess_prepare_for_pipelining chess_loop_count(kN / kVecSizeIO) {
                // Load kVecSizeIO elements from row i (strided by kN)
                ::aie::vector<TT_DATA, kVecSizeIO> h_vec;
                auto itU = itH;
                for (unsigned idx = 0; idx < kVecSizeIO; idx++) {
                    h_vec[idx] = *itU;
                    itU += kN; // Move to next column in same row
                }
                itH += kVecSizeIO * kN; // Move to next group of columns
                acc = ::aie::mac(acc, h_vec, *itv++);
            }
        result[i] = ::aie::reduce_add(acc.template to_vector<TT_DATA>());
    }
}

// ------------------------------------------------------------
// Compute Householder Vector
// ------------------------------------------------------------
// Computes Householder vector v to zero out elements H[k+2:kN-1, k]
// Returns beta = 2 / (v' * v)

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL float hessenberg<TT_DATA, TP_DIM>::compute_householder_vector(TT_DATA* H, unsigned k) {
    // Collect Householder vector v:
    auto itH = ::aie::begin_restrict_vector<kVecSizeIO>(H + k * kN);
    auto itv = ::aie::begin_restrict_vector<kVecSizeIO>(v);
    for (unsigned ii = 0; ii < kN / kVecSizeIO; ii++) chess_prepare_for_pipelining chess_loop_count(kN / kVecSizeIO) {
            *itv++ = *itH++;
        }
    for (unsigned ii = 0; ii <= k; ii++) v[ii] = makeReal<TT_DATA>(0.0f);

    // Compute norm of v[k+1:kN-1] using SIMD
    float sq_norm = squared_vector_norm(v);
    float norm = ::aie::sqrt(sq_norm);

    // Compute v = x + sign(x[k+1]) * norm * e_{k+1}
    // where x = H[k+1:kN-1, k]
    TT_DATA x_k1 = v[k + 1];
    float abs2 = getReal<TT_DATA>(x_k1) * getReal<TT_DATA>(x_k1) + getImag<TT_DATA>(x_k1) * getImag<TT_DATA>(x_k1);
    TT_DATA sigma = x_k1 * makeReal<TT_DATA>(norm * ::aie::invsqrt(abs2));
    v[k + 1] = v[k + 1] + sigma;

    // Recompute norm of updated v using SIMD:
    sq_norm = squared_vector_norm(v);
    float inv_norm = ::aie::invsqrt(sq_norm);

    // Normalize v (full kN-element processing):
    scale_vector(v, makeReal<TT_DATA>(inv_norm));

    // Return beta = 2
    return 2.0f;
}

// ------------------------------------------------------------
// Apply Left Householder Reflection
// ------------------------------------------------------------
// Computes H = (I - beta*v*v') * H = H - beta*v*(v'*H)

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL void hessenberg<TT_DATA, TP_DIM>::apply_left_householder(TT_DATA* H, unsigned k, float beta) {
    vm_inner_product(v, H, w);
    scale_vector(w, makeReal<TT_DATA>(beta));
    rank1_update(H, v, w);
}

// ------------------------------------------------------------
// Apply Right Householder Reflection
// ------------------------------------------------------------
// Computes H = H * (I - beta*v*v') = H - beta*(H*v)*v'

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL void hessenberg<TT_DATA, TP_DIM>::apply_right_householder(TT_DATA* H, unsigned k, float beta) {
    mv_product(H, v, w);
    scale_vector(w, makeReal<TT_DATA>(beta));
    rank1_update(H, w, v);
}

} // namespace eigenvalue
} // namespace aie
} // namespace solver
} // namespace xf
