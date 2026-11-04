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

#include "matrix_mult_vecapi.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace mp_pseudoinverse_svd {

// ---------------------------------------------------------------------------
// matrix_mult_vecapi::matrix_mult_vecapi_main()
//
// C = A * B via broadcast-scalar x row-vector multiply-accumulate:
// for each output row i, for each k, MAC scalar A[i,k] against the full
// row vector B[k,:], accumulating a vector that holds an entire row of C.
// ::aie::mul(scalar, vector) / ::aie::mac(accum, scalar, vector) map to the
// native broadcast-multiply hardware instruction on every AIE architecture
// (unlike plain C++ scalar float arithmetic, which lowers to softfloat
// library calls on AIE-ML/AIE-MLv2 -- see hw_mul() in svd_utils.hpp).
// ---------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_DIM_A_LEADING,
          unsigned int TP_DIM_OUT_LEADING,
          unsigned int TP_CONJ_A,
          unsigned int TP_CONJ_B>
NOINLINE_DECL void
matrix_mult_vecapi<TT_DATA_A,
                   TT_DATA_B,
                   TT_OUT_DATA,
                   TP_DIM_A,
                   TP_DIM_AB,
                   TP_DIM_B,
                   TP_DIM_A_LEADING,
                   TP_DIM_OUT_LEADING,
                   TP_CONJ_A,
                   TP_CONJ_B>::matrix_mult_vecapi_main(input_buffer<TT_DATA_A>& __restrict inWindowA,
                                                       input_buffer<TT_DATA_B>& __restrict inWindowB,
                                                       output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    using TT_DATA = TT_DATA_A; // homogeneous across A/B/OUT, enforced by static_assert
    using dataVec_t = ::aie::vector<TT_DATA, kVecSize>;
    using acc_t = ::aie::accum<vecapiAccType_t<TT_DATA>, kVecSize>;

    TT_DATA* __restrict pA = (TT_DATA*)inWindowA.data();
    TT_DATA* __restrict pB = (TT_DATA*)inWindowB.data();
    TT_DATA* __restrict pC = (TT_DATA*)outWindow.data();

    constexpr unsigned int kNumBlocks = TP_DIM_B / kVecSize;

    // A[i,k] -- scalar index, either leading dimension costs the same since
    // A is never read as a vector.
    auto loadA = [&](unsigned int i, unsigned int k) -> TT_DATA {
        TT_DATA a;
        if
            constexpr(TP_DIM_A_LEADING == ROW_MAJOR) { a = pA[i * TP_DIM_AB + k]; }
        else {
            a = pA[i + k * TP_DIM_A];
        }
        if
            constexpr(TP_CONJ_A) { a = ::aie::conj(a); }
        return a;
    };
    // B[k, nb*kVecSize : nb*kVecSize+kVecSize] -- contiguous vector read,
    // B is always row-major (see matrix_mult_vecapi.hpp).
    auto loadB = [&](unsigned int k, unsigned int nb) -> dataVec_t {
        dataVec_t b = ::aie::load_v<kVecSize>(pB + k * TP_DIM_B + nb * kVecSize);
        if
            constexpr(TP_CONJ_B) { b = ::aie::conj(b); }
        return b;
    };

    for (unsigned int i = 0; i < TP_DIM_A; ++i) chess_prepare_for_pipelining chess_loop_count(TP_DIM_A) {
            for (unsigned int nb = 0; nb < kNumBlocks; ++nb) chess_prepare_for_pipelining chess_loop_count(kNumBlocks) {
                    acc_t acc = ::aie::mul(loadA(i, 0), loadB(0, nb));
                    for (unsigned int k = 1; k < TP_DIM_AB; ++k)
                        chess_prepare_for_pipelining chess_loop_count(TP_DIM_AB - 1) {
                            acc = ::aie::mac(acc, loadA(i, k), loadB(k, nb));
                        }
                    dataVec_t resultVec = acc.template to_vector<TT_DATA>();

                    if
                        constexpr(TP_DIM_OUT_LEADING == ROW_MAJOR) {
                            ::aie::store_v(pC + i * TP_DIM_B + nb * kVecSize, resultVec);
                        }
                    else {
// Col-major output: the row this loop just computed is
// stored strided (stride TP_DIM_A) rather than contiguous.
// Only mp_pseudoinverse_svd_graph's T*U^H -> A^+ stage uses this
// path; the O(M*K*N) compute above stays fully vectorized
// either way, only this O(M*N) store is scalar.
#pragma unroll(kVecSize)
                        for (unsigned int lane = 0; lane < kVecSize; ++lane) {
                            pC[i + (nb * kVecSize + lane) * TP_DIM_A] = resultVec[lane];
                        }
                    }
                }
        }
}

} // namespace mp_pseudoinverse_svd
} // namespace aie
} // namespace solver
} // namespace xf
