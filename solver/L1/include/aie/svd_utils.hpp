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
#ifndef _DSPLIB_SVD_UTILS_HPP_
#define _DSPLIB_SVD_UTILS_HPP_

/*
SVD Utilities
This file contains helper functions used by the SVD kernel implementation.
These are separate from the traits file because they use vector types and
are purely for kernel use, not graph-level compilation.
*/

#include "device_defs.h"
#include "svd.hpp"

// Enable split-accumulator float reductions on AIE-ML, where they improve QoR.
#define XF_SOLVER_SVD_USE_SPLIT_ACCUMULATOR_REDUCTIONS (__AIE_ARCH__ == 20)

namespace xf {
namespace solver {
namespace aie {
namespace svd {

//-----------------------------------------------------------------------------------------------------
// hw_* helpers: hardware scalar float operations avoiding the softfloat library.
//
// On AIE-ML and AIE-MLv2 (__AIE_ARCH__ >= 20), ALL plain C++ scalar float
// operators (*, +, -, <) resolve to softfloat library routines, pulling in
// ~2-4 KB of PM. The fix is to broadcast the scalar to a native float vector,
// execute a hardware vector instruction, and extract lane 0.
//
// On AIE1 (__AIE_ARCH__ < 20), scalar float is native — plain C++
// operators compile directly to hardware instructions with zero overhead.
// Both cases use the same implementation; __X86SIM__ needs no separate path.
#if __AIE_ARCH__ < 20
// AIE1: scalar float arithmetic and elementary functions are native hardware.
static inline float hw_mul(float a, float b) {
    return a * b;
}
static inline float hw_add(float a, float b) {
    return a + b;
}
static inline float hw_sub(float a, float b) {
    return a - b;
}
static inline float hw_abs(float a) {
    return a < 0.0f ? -a : a;
}
static inline bool hw_lt(float a, float b) {
    return a < b;
}
static inline float hw_inv(float x) {
    return ::aie::inv(x);
}
static inline float hw_invsqrt(float x) {
    return ::aie::inv(::aie::sqrt(x));
}
static inline float hw_sqrt(float x) {
    return ::aie::sqrt(x);
}
#else
static constexpr int kHwVecSize = __MAX_READ_WRITE__ / 8 / sizeof(float);
static NOINLINE_DECL float hw_mul(float a, float b) {
    ::aie::vector<float, kHwVecSize> va = ::aie::broadcast<float, kHwVecSize>(a);
    return ::aie::mul(va, b).to_vector<float>()[0];
}
static NOINLINE_DECL float hw_add(float a, float b) {
    ::aie::vector<float, kHwVecSize> va = ::aie::broadcast<float, kHwVecSize>(a);
    ::aie::vector<float, kHwVecSize> vb = ::aie::broadcast<float, kHwVecSize>(b);
    return ::aie::add(va, vb)[0];
}
static NOINLINE_DECL float hw_sub(float a, float b) {
    ::aie::vector<float, kHwVecSize> va = ::aie::broadcast<float, kHwVecSize>(a);
    ::aie::vector<float, kHwVecSize> vb = ::aie::broadcast<float, kHwVecSize>(b);
    return ::aie::sub(va, vb)[0];
}
static NOINLINE_DECL float hw_abs(float a) {
    ::aie::vector<float, kHwVecSize> v = ::aie::broadcast<float, kHwVecSize>(a);
    return ::aie::abs(v)[0];
}
static NOINLINE_DECL bool hw_lt(float a, float b) {
    ::aie::vector<float, kHwVecSize> va = ::aie::broadcast<float, kHwVecSize>(a);
    ::aie::vector<float, kHwVecSize> vb = ::aie::broadcast<float, kHwVecSize>(b);
    float diff = ::aie::sub(va, vb)[0];
    // Extract sign bit via memcpy (avoids strict aliasing UB from pointer cast).
    uint32_t bits;
    __builtin_memcpy(&bits, &diff, sizeof(bits));
    return bits >> 31;
}
static NOINLINE_DECL float hw_inv(float x) {
    ::aie::vector<float, kHwVecSize> v = ::aie::broadcast<float, kHwVecSize>(x);
    return ::aie::inv(v)[0];
}

#if __USES_NATIVE_SQRT_FUNC__
// AIE-MLv2 (arch 22): native hardware invsqrt.
static NOINLINE_DECL float hw_invsqrt(float x) {
    ::aie::vector<float, kHwVecSize> v = ::aie::broadcast<float, kHwVecSize>(x);
    return ::aie::invsqrt(v)[0];
}
#else
// AIE-ML (arch 20): ::aie::invsqrt is a ~12-bit software approximation.
// One Newton-Raphson step y1 = y0*(1.5 - 0.5*x*y0^2) reaches full precision.
static NOINLINE_DECL float hw_invsqrt(float x) {
    ::aie::vector<float, kHwVecSize> vconst_0_5 = ::aie::broadcast<float, kHwVecSize>(0.5f);
    ::aie::vector<float, kHwVecSize> vconst_1_5 = ::aie::broadcast<float, kHwVecSize>(1.5f);
    ::aie::vector<float, kHwVecSize> vx = ::aie::broadcast<float, kHwVecSize>(x);
    ::aie::vector<float, kHwVecSize> vy = ::aie::invsqrt(vx);
    ::aie::vector<float, kHwVecSize> vysqr = ::aie::mul(vy, vy);
    ::aie::vector<float, kHwVecSize> vhalf = ::aie::mul(vconst_0_5, vx);
    ::aie::vector<float, kHwVecSize> t = ::aie::mul(vysqr, vhalf);
    ::aie::vector<float, kHwVecSize> t_sub = ::aie::sub(vconst_1_5, t);
    return ::aie::mul(vy, t_sub).to_vector<float>()[0];
}
#endif // __USES_NATIVE_SQRT_FUNC__

// hw_sqrt: x * (1/sqrt(x)) = sqrt(x) via hw_invsqrt.
// Bit-test for zero/negative avoids a hw_lt call; inputs here (normSq, mag)
// are always >= 0 so exact-zero is the only case requiring a guard.
static NOINLINE_DECL float hw_sqrt(float x) {
    uint32_t bits;
    __builtin_memcpy(&bits, &x, sizeof(bits));
    if ((bits & 0x7FFFFFFFu) == 0u) return 0.0f;
    return hw_mul(x, hw_invsqrt(x));
}
#endif

// Single convergence threshold used throughout the kernel.
static constexpr float kEpsilon = 1e-10f;

//-----------------------------------------------------------------------------------------------------
// Helper: Accumulate dot products for one column pair (Phase 1)
//
// Returns:
//   dotII     — <col_i, col_i>  (real, always >= 0)
//   dotJJ     — <col_j, col_j>  (real, always >= 0)
//   dotIJreal — Re(<col_i, col_j>) for cfloat; real dot product for float
//   dotIJimag — Im(<col_i, col_j>) for cfloat; 0 (unused) for float
//
// The caller is responsible for normalising phase and computing |dotIJ| for cfloat.
// Used by both the single-kernel and cascaded paths.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int kVecSize, bool kIsComplex, unsigned int kVecsPerUCol>
static NOINLINE_DECL void computePairDots(::aie::vector<TT_DATA, kVecSize>* __restrict iPtr,
                                          ::aie::vector<TT_DATA, kVecSize>* __restrict jPtr,
                                          float& dotII,
                                          float& dotJJ,
                                          float& dotIJreal,
                                          float& dotIJimag) {
    using acc_t = ::aie::accum<accfloat, kVecSize>;

    if
        constexpr(kIsComplex) {
            using cacc_t = ::aie::accum<caccfloat, kVecSize>;
            cacc_t norm_i_acc = ::aie::mul(::aie::op_conj(iPtr[0]), iPtr[0]);
            cacc_t norm_j_acc = ::aie::mul(::aie::op_conj(jPtr[0]), jPtr[0]);
            cacc_t dotIJ_acc = ::aie::mul(::aie::op_conj(iPtr[0]), jPtr[0]);

            for (int v = 1; v < kVecsPerUCol; v++) {
                norm_i_acc = ::aie::mac(norm_i_acc, ::aie::op_conj(iPtr[v]), iPtr[v]);
                norm_j_acc = ::aie::mac(norm_j_acc, ::aie::op_conj(jPtr[v]), jPtr[v]);
                dotIJ_acc = ::aie::mac(dotIJ_acc, ::aie::op_conj(iPtr[v]), jPtr[v]);
            }
            cfloat ni = ::aie::reduce_add(norm_i_acc.template to_vector<cfloat>());
            cfloat nj = ::aie::reduce_add(norm_j_acc.template to_vector<cfloat>());
            cfloat dij = ::aie::reduce_add(dotIJ_acc.template to_vector<cfloat>());
            dotII = ni.real;
            dotJJ = nj.real;
            dotIJreal = dij.real;
            dotIJimag = dij.imag;
        }
    else {
#if XF_SOLVER_SVD_USE_SPLIT_ACCUMULATOR_REDUCTIONS
        // Split-accumulator (N=2) to break the single fpmac feedback recurrence.
        // Each of the three reductions (norm_i, norm_j, dotIJ) uses two independent
        // accumulators advanced over even/odd vectors, then combined. This raises the
        // available ILP so the chess compiler can hide the fpmac latency (4-cycle on
        // AIE1, 2-cycle on AIE-ML), reducing the effective loop II. A scalar remainder
        // handles the case where kVecsPerUCol is odd. Float path only — the cfloat path
        // is intentionally left single-accumulator because cfloat cascaded AIE22 kernels
        // are already near the 16 KB program-memory limit.
        constexpr bool haveAcc1 = (kVecsPerUCol > 1);
        acc_t norm_i_acc0 = ::aie::mul_square(iPtr[0]);
        acc_t norm_j_acc0 = ::aie::mul_square(jPtr[0]);
        acc_t dotIJ_acc0 = ::aie::mul(iPtr[0], jPtr[0]);
        acc_t norm_i_acc1;
        acc_t norm_j_acc1;
        acc_t dotIJ_acc1;
        int v = 1;
        if
            constexpr(haveAcc1) {
                norm_i_acc1 = ::aie::mul_square(iPtr[1]);
                norm_j_acc1 = ::aie::mul_square(jPtr[1]);
                dotIJ_acc1 = ::aie::mul(iPtr[1], jPtr[1]);
                v = 2;
            }
        for (; v + 1 < (int)kVecsPerUCol; v += 2) chess_prepare_for_pipelining {
                norm_i_acc0 = ::aie::mac_square(norm_i_acc0, iPtr[v]);
                norm_i_acc1 = ::aie::mac_square(norm_i_acc1, iPtr[v + 1]);
                norm_j_acc0 = ::aie::mac_square(norm_j_acc0, jPtr[v]);
                norm_j_acc1 = ::aie::mac_square(norm_j_acc1, jPtr[v + 1]);
                dotIJ_acc0 = ::aie::mac(dotIJ_acc0, iPtr[v], jPtr[v]);
                dotIJ_acc1 = ::aie::mac(dotIJ_acc1, iPtr[v + 1], jPtr[v + 1]);
            }
        // Scalar remainder for odd kVecsPerUCol.
        for (; v < (int)kVecsPerUCol; v++) {
            norm_i_acc0 = ::aie::mac_square(norm_i_acc0, iPtr[v]);
            norm_j_acc0 = ::aie::mac_square(norm_j_acc0, jPtr[v]);
            dotIJ_acc0 = ::aie::mac(dotIJ_acc0, iPtr[v], jPtr[v]);
        }
        if
            constexpr(haveAcc1) {
                auto ni = ::aie::add(norm_i_acc0.template to_vector<float>(), norm_i_acc1.template to_vector<float>());
                auto nj = ::aie::add(norm_j_acc0.template to_vector<float>(), norm_j_acc1.template to_vector<float>());
                auto dij = ::aie::add(dotIJ_acc0.template to_vector<float>(), dotIJ_acc1.template to_vector<float>());
                dotII = ::aie::reduce_add(ni);
                dotJJ = ::aie::reduce_add(nj);
                dotIJreal = ::aie::reduce_add(dij);
            }
        else {
            dotII = ::aie::reduce_add(norm_i_acc0.template to_vector<float>());
            dotJJ = ::aie::reduce_add(norm_j_acc0.template to_vector<float>());
            dotIJreal = ::aie::reduce_add(dotIJ_acc0.template to_vector<float>());
        }
        dotIJimag = 0.0f;
#else
        acc_t norm_i_acc = ::aie::mul_square(iPtr[0]);
        acc_t norm_j_acc = ::aie::mul_square(jPtr[0]);
        acc_t dotIJ_acc = ::aie::mul(iPtr[0], jPtr[0]);
        for (int v = 1; v < kVecsPerUCol; v++) {
            norm_i_acc = ::aie::mac_square(norm_i_acc, iPtr[v]);
            norm_j_acc = ::aie::mac_square(norm_j_acc, jPtr[v]);
            dotIJ_acc = ::aie::mac(dotIJ_acc, iPtr[v], jPtr[v]);
        }
        dotII = ::aie::reduce_add(norm_i_acc.template to_vector<float>());
        dotJJ = ::aie::reduce_add(norm_j_acc.template to_vector<float>());
        dotIJreal = ::aie::reduce_add(dotIJ_acc.template to_vector<float>());
        dotIJimag = 0.0f;
#endif
    }
}

//-----------------------------------------------------------------------------------------------------
// Helper: Givens rotation computation (Phase 2)
//
// Scalar per pair using hw_* helpers. On AIE1/x86sim, hw_* are inline plain
// operators (zero overhead). On AIE-ML/AIE22, hw_* are NOINLINE vector
// broadcast/extract wrappers — necessary to avoid softfloat on scalar float ops.
//
// The vectorized-across-pairs approach was explored but rejected: for cfloat cascaded
// kernels on AIE22 (16 KB PM limit), the vectorized Givens body grew ~5× (928 → 4816 bytes)
// causing PM overflow. The scalar loop is compact and Phase 2 is a small fraction of
// total compute, so the overhead is acceptable.
//
// Used identically by both the single-kernel and cascaded paths.
//-----------------------------------------------------------------------------------------------------
template <unsigned int kNumPairs>
static NOINLINE_DECL void computeGivensRotations(float* dotII, float* dotJJ, float* dotIJ, float* c_arr, float* s_arr) {
    for (int p = 0; p < kNumPairs; p++) {
        float absIJ = hw_abs(dotIJ[p]);
        if (hw_lt(absIJ, kEpsilon)) {
            c_arr[p] = 1.0f;
            s_arr[p] = 0.0f;
            continue;
        }
        float invAbsIJ = hw_inv(absIJ);
        float tau = hw_mul(hw_sub(dotII[p], dotJJ[p]), hw_mul(0.5f, invAbsIJ));
        float absTau = hw_abs(tau);
        float recipDenom = hw_inv(hw_add(absTau, hw_sqrt(hw_add(1.0f, hw_mul(tau, tau)))));
        float t = hw_lt(dotJJ[p], dotII[p]) ? -recipDenom : recipDenom;
        float c = hw_invsqrt(hw_add(1.0f, hw_mul(t, t)));
        c_arr[p] = c;
        s_arr[p] = hw_mul(hw_mul(hw_mul(dotIJ[p], t), c), invAbsIJ);
    }
}

//-----------------------------------------------------------------------------------------------------
// Helper: Apply Givens rotations to U and V column pairs (Phase 3)
//
// Applies pre-computed c/s values to each column pair in the current set.
//   cfloat: incorporates the complex phase factor conj(phase) into the rotation.
//           Simple loop body (no prefetch) to minimise register pressure on AIE1.
//   float:  standard [[c,-s],[s,c]] rotation with software-pipelined prefetch.
//
// Used identically by both the single-kernel and cascaded paths.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int kVecSize, bool kIsComplex>
static NOINLINE_DECL void applyGivensRotations(::aie::vector<TT_DATA, kVecSize>* __restrict U,
                                               ::aie::vector<TT_DATA, kVecSize>* __restrict V,
                                               const float* c_arr,
                                               const float* s_arr,
                                               const float* phase_real,
                                               const float* phase_imag,
                                               const uint8_t (*pairs)[2],
                                               int setStart,
                                               int numPairs,
                                               int vecsPerUCol,
                                               int vecsPerVCol) {
    using vec_t = ::aie::vector<TT_DATA, kVecSize>;
    using acc_t = ::aie::accum<accfloat, kVecSize>;

    for (int p = 0; p < numPairs; p++) {
        int i = pairs[setStart + p][0];
        int j = pairs[setStart + p][1];

        vec_t* Ui = U + i * vecsPerUCol;
        vec_t* Uj = U + j * vecsPerUCol;
        vec_t* Vi = V + i * vecsPerVCol;
        vec_t* Vj = V + j * vecsPerVCol;

        if
            constexpr(kIsComplex) {
                // Complex Jacobi rotation: J = diag(1, conj(phase)) * Givens
                //   new_i = c*col_i - s*conj(phase)*col_j
                //   new_j = s*col_i + c*conj(phase)*col_j
                TT_DATA c_val = {c_arr[p], 0.0f};
                TT_DATA s_val = {s_arr[p], 0.0f};
                TT_DATA s_conj_phase = {hw_mul(s_arr[p], phase_real[p]), -hw_mul(s_arr[p], phase_imag[p])};
                TT_DATA c_conj_phase = {hw_mul(c_arr[p], phase_real[p]), -hw_mul(c_arr[p], phase_imag[p])};
                using cacc_t = ::aie::accum<caccfloat, kVecSize>;

                // Rotate U columns
                for (int v = 0; v < vecsPerUCol; v++) {
                    vec_t ai = Ui[v], aj = Uj[v];
                    cacc_t newI = ::aie::msc(::aie::mul(c_val, ai), s_conj_phase, aj);
                    cacc_t newJ = ::aie::mac(::aie::mul(s_val, ai), c_conj_phase, aj);
                    Ui[v] = newI.template to_vector<TT_DATA>();
                    Uj[v] = newJ.template to_vector<TT_DATA>();
                }

                // Rotate V columns.
                for (int v = 0; v < vecsPerVCol; v++) {
                    vec_t vi = Vi[v], vj = Vj[v];
                    cacc_t newVi = ::aie::msc(::aie::mul(c_val, vi), s_conj_phase, vj);
                    cacc_t newVj = ::aie::mac(::aie::mul(s_val, vi), c_conj_phase, vj);
                    Vi[v] = newVi.template to_vector<TT_DATA>();
                    Vj[v] = newVj.template to_vector<TT_DATA>();
                }
            }
        else {
            // float rotation: [[c, -s], [s, c]]
            TT_DATA c = c_arr[p];
            TT_DATA s = s_arr[p];

            // Rotate U columns
            for (int v = 0; v < vecsPerUCol; v++) {
                vec_t ai = Ui[v], aj = Uj[v];
                acc_t newI = ::aie::msc(::aie::mul(c, ai), s, aj);
                acc_t newJ = ::aie::mac(::aie::mul(s, ai), c, aj);
                Ui[v] = newI.template to_vector<TT_DATA>();
                Uj[v] = newJ.template to_vector<TT_DATA>();
            }

            // Rotate V columns.
            // Uses msc/mac which works correctly at kVecSizeComp=16 (v16float) on AIE22.
            // The aie2ps VMSC.f bug only manifested at kVecSizeComp=8 (v8float) where
            // adjacent columns shared a 512-bit block. With kStoreAlign forcing
            // kVecSizeComp=16, each column is a full 512-bit block and the bug does
            // not occur. The 4-mul decomposition also failed at 16-lane — msc/mac is correct.
            for (int v = 0; v < vecsPerVCol; v++) {
                vec_t vi = Vi[v], vj = Vj[v];
                acc_t newVi = ::aie::msc(::aie::mul(c, vi), s, vj);
                acc_t newVj = ::aie::mac(::aie::mul(s, vi), c, vj);
                Vi[v] = newVi.template to_vector<TT_DATA>();
                Vj[v] = newVj.template to_vector<TT_DATA>();
            }
        }
    }
}

//-----------------------------------------------------------------------------------------------------
// Helper: Sort singular values descending, permuting U and V columns
//
// Selection sort O(n^2) scalar comparisons + column swaps.
// kNumCols should be TP_DIM_COLS (real columns only); phantom columns are
// zero and always sort to the end naturally, so they need not be included.
//
// Used identically by both the single-kernel and cascaded paths.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int kVecSize>
static NOINLINE_DECL void sortSingularValues(T_outSDataType* __restrict S,
                                             ::aie::vector<TT_DATA, kVecSize>* __restrict U,
                                             ::aie::vector<TT_DATA, kVecSize>* __restrict V,
                                             int numCols,
                                             int vecsPerUCol,
                                             int vecsPerVCol) {
    using vec_t = ::aie::vector<TT_DATA, kVecSize>;

    // Selection sort: use chess_protect_access to prevent chess compiler from
    // caching/hoisting S reads across the data-dependent inner loop
    // (maxIdx changes each iteration) and across outer-loop swaps.
    // chess_protect_access (not volatile) allows S reads to be reordered
    // relative to U/V column swaps — volatile would prevent that.
    chess_protect_access T_outSDataType* Sv = S;
    for (int i = 0; i < numCols - 1; i++) {
        int maxIdx = i;
        for (int j = i + 1; j < numCols; j++) {
            if (hw_lt(Sv[maxIdx], Sv[j])) maxIdx = j;
        }
        if (maxIdx != i) {
            T_outSDataType tmpS = Sv[i];
            Sv[i] = Sv[maxIdx];
            Sv[maxIdx] = tmpS;
            vec_t* Ui = U + i * vecsPerUCol;
            vec_t* Uj = U + maxIdx * vecsPerUCol;
            for (int v = 0; v < vecsPerUCol; v++) {
                vec_t t = Ui[v];
                Ui[v] = Uj[v];
                Uj[v] = t;
            }
            vec_t* Vi = V + i * vecsPerVCol;
            vec_t* Vj = V + maxIdx * vecsPerVCol;
            for (int v = 0; v < vecsPerVCol; v++) {
                vec_t t = Vi[v];
                Vi[v] = Vj[v];
                Vj[v] = t;
            }
        }
    }
}

//-----------------------------------------------------------------------------------------------------
// Helper: Initialize U from input A (with phantom column zero-padding) and V to identity.
//
// Shared by pass-pipelined SVD kernels. Mirrors the init block at the
// top of svdMain/svdCascadedMain; kept as a separate helper rather than refactoring those two
// (proven, in-production) entry points to call it, to avoid regression risk.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int kVecSize, bool kIsComplex>
static NOINLINE_DECL void initUVIdentity(::aie::vector<TT_DATA, kVecSize>* __restrict A_in,
                                         ::aie::vector<TT_DATA, kVecSize>* __restrict U,
                                         ::aie::vector<TT_DATA, kVecSize>* V,
                                         TT_DATA* Vscalar,
                                         int numCols,
                                         int storeCols,
                                         int vecsPerUCol,
                                         int vecsPerVCol) {
    // Copy true columns from input (column-major, A_in and U share stride vecsPerUCol).
    for (int i = 0; i < numCols * vecsPerUCol; i++) {
        U[i] = A_in[i];
    }
    // Zero-pad schedule/storage phantom columns.
    for (int col = numCols; col < storeCols; col++) {
        for (int v = 0; v < vecsPerUCol; v++) {
            U[col * vecsPerUCol + v] = ::aie::zeros<TT_DATA, kVecSize>();
        }
    }
    // V identity: storeCols x storeCols.
    for (int v = 0; v < storeCols * vecsPerVCol; v++) {
        V[v] = ::aie::zeros<TT_DATA, kVecSize>();
    }
    // Set the identity diagonal through the same vector view consumed by the
    // sweep. Using a scalar alias here is miscompiled by the AIE backend when
    // this NOINLINE helper returns to another helper: for float, the earlier
    // vector zero-fill wins and V remains entirely zero. A single vector view
    // avoids the alias ambiguity on every architecture.
    for (int col = 0; col < storeCols; col++) {
        TT_DATA one;
        if
            constexpr(kIsComplex) { one = TT_DATA{1.0f, 0.0f}; }
        else {
            one = 1.0f;
        }
        int vec = col / (int)kVecSize;
        int lane = col % (int)kVecSize;
        V[col * vecsPerVCol + vec].set(one, lane);
    }

    chess_memory_fence();
}

//-----------------------------------------------------------------------------------------------------
// Helper: Run one full Jacobi sweep (all kNumSets, phases 1-3) over U/V held ENTIRELY LOCALLY
// (no row split, no stream exchange — the caller already owns the complete matrix).
//
// Shared by pass-pipelined SVD kernels, where each kernel
// performs exactly one sweep before forwarding U/V to the next kernel. kPairsPerSet/kVecsPerUCol/
// kVecsPerVCol are template (not runtime) parameters because computeGivensRotations/computePairDots
// require compile-time trip counts, matching the existing single-kernel/cascaded code.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int kVecSize,
          bool kIsComplex,
          unsigned int kPairsPerSet,
          unsigned int kVecsPerUCol,
          unsigned int kVecsPerVCol>
static NOINLINE_DECL void runJacobiSweep(::aie::vector<TT_DATA, kVecSize>* __restrict U,
                                         ::aie::vector<TT_DATA, kVecSize>* V,
                                         float* dotII,
                                         float* dotJJ,
                                         float* dotIJ,
                                         float* c_arr,
                                         float* s_arr,
                                         float* phase_real,
                                         float* phase_imag,
                                         const uint8_t (*pairs)[2],
                                         int kNumSets) {
    using vec_t = ::aie::vector<TT_DATA, kVecSize>;

    for (int setIdx = 0; setIdx < kNumSets; setIdx++) {
        int setStart = setIdx * (int)kPairsPerSet;

        // ============ PHASE 1: Compute all dot products ============
        for (int p = 0; p < (int)kPairsPerSet; p++) {
            int i = pairs[setStart + p][0];
            int j = pairs[setStart + p][1];
            vec_t* iPtr = U + i * kVecsPerUCol;
            vec_t* jPtr = U + j * kVecsPerUCol;

            float raw_real, raw_imag;
            computePairDots<TT_DATA, kVecSize, kIsComplex, kVecsPerUCol>(iPtr, jPtr, dotII[p], dotJJ[p], raw_real,
                                                                         raw_imag);

            if
                constexpr(kIsComplex) {
                    float mag = hw_sqrt(hw_add(hw_mul(raw_real, raw_real), hw_mul(raw_imag, raw_imag)));
                    dotIJ[p] = mag;
                    if (hw_lt(kEpsilon, mag)) {
                        float inv_mag = hw_inv(mag);
                        phase_real[p] = hw_mul(raw_real, inv_mag);
                        phase_imag[p] = hw_mul(raw_imag, inv_mag);
                    } else {
                        phase_real[p] = 1.0f;
                        phase_imag[p] = 0.0f;
                    }
                }
            else {
                dotIJ[p] = raw_real;
            }
        }
        // See svdMain: one fence per iteration is well-defined (Phase 1 always before,
        // Phase 2-3 always after).
        chess_memory_fence();

        // ============ PHASE 2: Scalar Givens computation ============
        computeGivensRotations<kPairsPerSet>(dotII, dotJJ, dotIJ, c_arr, s_arr);

        chess_separator_scheduler();

        // ============ PHASE 3: Apply rotations to U and V ============
        applyGivensRotations<TT_DATA, kVecSize, kIsComplex>(U, V, c_arr, s_arr, phase_real, phase_imag, pairs, setStart,
                                                            (int)kPairsPerSet, (int)kVecsPerUCol, (int)kVecsPerVCol);
        // See svdMain: guards against the chess compiler delaying V stores (not __restrict)
        // past the loop boundary, which would leave stale V reads on the next set.
        chess_separator_scheduler();
    }
}

//-----------------------------------------------------------------------------------------------------
// Helper: Finalize — extract singular values, normalize U, sort descending, compact V.
//
// Shared by the final pass-pipeline stage, which runs this on the U/V
// state handed to it by the last pass-kernel (no sweep of its own). Mirrors the finalize block at
// the bottom of svdMain/svdCascadedMain.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int kVecSize,
          bool kIsComplex,
          unsigned int kVecsPerUCol,
          unsigned int kVecsPerVCol>
static NOINLINE_DECL void finalizeSVD(::aie::vector<TT_DATA, kVecSize>* __restrict U,
                                      ::aie::vector<TT_DATA, kVecSize>* V,
                                      T_outSDataType* __restrict S,
                                      int numCols,
                                      int storeCols) {
    using vec_t = ::aie::vector<TT_DATA, kVecSize>;
    using acc_t = ::aie::accum<accfloat, kVecSize>;

    for (int col = 0; col < numCols; col++) {
        vec_t* colPtr = U + col * kVecsPerUCol;

        float normSq;
        if
            constexpr(kIsComplex) {
                using cacc_t = ::aie::accum<caccfloat, kVecSize>;
                cacc_t cacc = ::aie::mul(::aie::op_conj(colPtr[0]), colPtr[0]);
                for (int v = 1; v < (int)kVecsPerUCol; v++) {
                    cacc = ::aie::mac(cacc, ::aie::op_conj(colPtr[v]), colPtr[v]);
                }
                cfloat norm_c = ::aie::reduce_add(cacc.template to_vector<cfloat>());
                normSq = norm_c.real;
            }
        else {
            // Split-accumulator (N=2): see svdMain finalize for rationale.
            constexpr bool haveAcc1 = (kVecsPerUCol > 1);
            acc_t nacc0 = ::aie::mul_square(colPtr[0]);
            acc_t nacc1;
            int v = 1;
            if
                constexpr(haveAcc1) {
                    nacc1 = ::aie::mul_square(colPtr[1]);
                    v = 2;
                }
            for (; v + 1 < (int)kVecsPerUCol; v += 2) chess_prepare_for_pipelining {
                    nacc0 = ::aie::mac_square(nacc0, colPtr[v]);
                    nacc1 = ::aie::mac_square(nacc1, colPtr[v + 1]);
                }
            for (; v < (int)kVecsPerUCol; v++) {
                nacc0 = ::aie::mac_square(nacc0, colPtr[v]);
            }
            if
                constexpr(haveAcc1) {
                    auto nv = ::aie::add(nacc0.template to_vector<float>(), nacc1.template to_vector<float>());
                    normSq = ::aie::reduce_add(nv);
                }
            else {
                normSq = ::aie::reduce_add(nacc0.template to_vector<float>());
            }
        }
        float sigma = hw_sqrt(normSq);
        S[col] = sigma;

        float invSigma = hw_lt(kEpsilon, sigma) ? hw_inv(sigma) : 0.0f;
        for (int v = 0; v < (int)kVecsPerUCol; v++) {
            colPtr[v] = ::aie::mul(colPtr[v], invSigma);
        }
    }
    // Phantom columns: set sigma=0 directly for the sort.
    for (int col = numCols; col < storeCols; col++) {
        S[col] = 0.0f;
    }

    // Sort real columns only; phantom columns are zero and naturally trail any real sigma.
    sortSingularValues<TT_DATA, kVecSize>(S, U, V, numCols, (int)kVecsPerUCol, (int)kVecsPerVCol);

    // V remains in its storeCols x storeCols padded column-major layout.
    // MatMul1 in mp_pseudoinverse_graph reads V as storeCols x storeCols col-major,
    // which matches this layout. Padding columns/rows contribute zero to the result
    // since the corresponding S^-1 diagonal entries are zero. (See svd.cpp svdMain
    // for the same rationale -- this helper must not reintroduce the V-compaction
    // that was removed there.)
}

} // namespace svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _DSPLIB_SVD_UTILS_HPP_
