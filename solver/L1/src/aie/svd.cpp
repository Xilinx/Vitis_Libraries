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
/*
SVD kernel code - Parallel Pairs Implementation
Supports arbitrary TP_DIM_COLS (including odd and non-multiple-of-kVecSize)
via internal padding: kSchedCols (next even >= TP_DIM_COLS) for the Jacobi
pair schedule, kStoreCols (next multiple of kVecSize >= kSchedCols)
for SIMD-aligned V column storage. Padding is transparent to the user.

Coding conventions:
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <stdio.h>
#include "device_defs.h"
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "svd.hpp"
#include "svd_utils.hpp"
#include "kernel_api_utils.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace svd {

//-----------------------------------------------------------------------------------------------------
// Single kernel SVD using Parallel Pairs optimization
//
// Key insight: In Jacobi SVD, we can organize column pairs into "sets" where
// all pairs in a set are independent (no shared columns). This allows us to:
//   1. Compute dot products for all pairs in a set
//   2. Compute all Givens rotations for the set from the reduced dot products
//   3. Apply rotations to each independent column pair
//
// Arbitrary TP_DIM_COLS: internally padded to kSchedCols (next even) for the
// pair schedule and kStoreCols (next multiple of kVecSize) for V storage.
// Phantom columns are zero in U, producing identity rotations (c=1, s=0).
//-----------------------------------------------------------------------------------------------------

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
NOINLINE_DECL void
SVDecomposition<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION>::svdMain(
    input_buffer<TT_DATA>& __restrict inWindow,
    output_buffer<TT_DATA>& __restrict outUWindow,
    output_buffer<T_outSDataType>& __restrict outSWindow,
    output_buffer<TT_DATA>& __restrict outVWindow) {
    // Vector configuration
    static constexpr bool kIsComplex = fnIsComplexDataType<TT_DATA>();
    // kVecSizeIO: fixed 256-bit width used for output port sizing (matches graph).
    // kVecSizeCompRaw: native hardware vector width (512-bit on AIE-ML/AIE22).
    // kVecSizeComp: actual compute width — uses native 512-bit when both
    //   TP_DIM_ROWS and kStoreCols are divisible by it; falls back to 256-bit
    //   for small configurations that cannot fill a full native vector.
    static constexpr unsigned int kVecSizeIO = fnVecSampleNumIO<TT_DATA>();
    static constexpr unsigned int kVecSizeCompRaw = fnVecSampleNum<TT_DATA>();

    // --------------------------------------------------
    // Padding constants for arbitrary TP_DIM_COLS support
    // kSchedCols: minimum even column count needed by the Jacobi schedule
    // kStoreCols: next multiple of kVecSizeIO >= kSchedCols (256-bit aligned,
    //             matches graph port dimensions — unchanged by kVecSizeComp)
    // --------------------------------------------------
    static constexpr unsigned int kSchedCols = TP_DIM_COLS + (TP_DIM_COLS % 2);
    static constexpr unsigned int kStoreAlign = fnVecStoreAlign<TT_DATA>();
    static constexpr unsigned int kStoreCols = ((kSchedCols + kStoreAlign - 1) / kStoreAlign) * kStoreAlign;

    // Select compute width: use native (512-bit) when dimensions allow,
    // fall back to IO width (256-bit) for small matrices.
    static constexpr unsigned int kVecSizeComp =
        (kStoreCols % kVecSizeCompRaw == 0 && TP_DIM_ROWS % kVecSizeCompRaw == 0) ? kVecSizeCompRaw : kVecSizeIO;

    // Dimensions derived from padded column count
    static constexpr int kInputSize = TP_DIM_ROWS * TP_DIM_COLS;
    static constexpr int kPairsPerSet = kSchedCols / 2;
    static constexpr int kNumSets = kSchedCols - 1;
    static constexpr unsigned int kVecsPerUCol = TP_DIM_ROWS / kVecSizeComp;
    static constexpr unsigned int kVecsPerVCol = kStoreCols / kVecSizeComp;

    // Type aliases (use compute width)
    using vec_t = ::aie::vector<TT_DATA, kVecSizeComp>;
    using acc_t = ::aie::accum<accfloat, kVecSizeComp>;

    // Pointers to working memory (output buffers used as workspace)
    // NOTE: V and Vscalar alias the same buffer, so neither may be marked
    // __restrict without invoking undefined behavior.
    vec_t* __restrict U = (vec_t*)outUWindow.data();
    vec_t* V = (vec_t*)outVWindow.data();
    TT_DATA* Vscalar = (TT_DATA*)outVWindow.data();
    T_outSDataType* __restrict S = (T_outSDataType*)outSWindow.data();
    vec_t* __restrict A_in = (vec_t*)inWindow.data();

    // Access compile-time pair organization (uses kSchedCols, always even)
    constexpr auto& pairs = ParallelPairSets<kSchedCols>::pairs;

    // --------------------------------------------------
    // Initialize: Copy A to U (true columns), zero-pad phantom columns,
    //             set V to identity (kStoreCols x kStoreCols)
    // --------------------------------------------------
    // Copy true columns from input (column-major, both have stride kVecsPerUCol).
    for (int i = 0; i < kInputSize / kVecSizeComp; i++) {
        U[i] = A_in[i];
    }
    // Zero-pad schedule/storage phantom columns
    for (int col = TP_DIM_COLS; col < (int)kStoreCols; col++) {
        for (int v = 0; v < (int)kVecsPerUCol; v++) {
            U[col * kVecsPerUCol + v] = ::aie::zeros<TT_DATA, kVecSizeComp>();
        }
    }

    // V identity: kStoreCols x kStoreCols
    // Zero entire V buffer via vectorised store, then set diagonal to 1.
    for (int v = 0; v < (int)(kStoreCols * kStoreCols / kVecSizeComp); v++) {
        V[v] = ::aie::zeros<TT_DATA, kVecSizeComp>();
    }
    for (int col = 0; col < (int)kStoreCols; col++) {
        if
            constexpr(kIsComplex) { Vscalar[col * kStoreCols + col] = TT_DATA{1.0f, 0.0f}; }
        else {
            Vscalar[col * kStoreCols + col] = 1.0f;
        }
    }

    // --------------------------------------------------
    // Working arrays in DATA MEMORY (not stack) to avoid register spilling.
    // Static placement prevents these from consuming stack/register space.
    // --------------------------------------------------
    // Scratch buffer pointers — padded stride for vector stream I/O.
    // No __restrict: all slices of the same scratchBuf.
    static constexpr unsigned int kScratchStride = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kPaddedPairsPerSet;
    float* dotII = this->scratchBuf;
    float* dotJJ = dotII + kScratchStride;
    float* dotIJ = dotJJ + kScratchStride;
    float* c_arr = dotIJ + kScratchStride;
    float* s_arr = c_arr + kScratchStride;
    float* phase_real = s_arr + kScratchStride;
    float* phase_imag = phase_real + kScratchStride;

    // --------------------------------------------------
    // Main Jacobi SVD iteration
    // --------------------------------------------------
    for (int pass = 0; pass < TP_PASSES; pass++) {
        for (int setIdx = 0; setIdx < kNumSets; setIdx++) {
            int setStart = setIdx * kPairsPerSet;

            // ============ PHASE 1: Compute all dot products ============
            for (int p = 0; p < kPairsPerSet; p++) {
                int i = pairs[setStart + p][0];
                int j = pairs[setStart + p][1];
                vec_t* iPtr = U + i * kVecsPerUCol;
                vec_t* jPtr = U + j * kVecsPerUCol;

                float raw_real, raw_imag;
                computePairDots<TT_DATA, kVecSizeComp, kIsComplex>(iPtr, jPtr, dotII[p], dotJJ[p], raw_real, raw_imag,
                                                                   kVecsPerUCol);

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
            // chess_memory_fence ensures Phase 1 scratch buffer stores are
            // committed before Phase 2 reads them. One fence per loop iteration
            // is well-defined: Phase 1 ops always before, Phase 2-3 always after.
            chess_memory_fence();

            // ============ PHASE 2: Scalar Givens computation ============
            computeGivensRotations<kPairsPerSet>(dotII, dotJJ, dotIJ, c_arr, s_arr);

            chess_separator_scheduler();

            // ============ PHASE 3: Apply rotations to U and V ============
            applyGivensRotations<TT_DATA, kVecSizeComp, kIsComplex>(U, V, c_arr, s_arr, phase_real, phase_imag, pairs,
                                                                    setStart, kPairsPerSet, kVecsPerUCol, kVecsPerVCol);
            // chess_separator_scheduler ensures Phase 3 U and V stores are committed
            // before Phase 1 of the next set reads U and V. Without this barrier the
            // chess compiler on AIE22 may delay V stores past the loop boundary (U is
            // protected by __restrict; V is not), causing stale V reads on the next
            // iteration and leaving duplicate columns in the output.
            chess_separator_scheduler();
        }
    }

    // --------------------------------------------------
    // Finalize: Extract singular values and normalize U
    // Only real columns need norm computation; phantom columns (zero throughout)
    // have sigma=0 — written directly to skip redundant work.
    // --------------------------------------------------
    for (int col = 0; col < TP_DIM_COLS; col++) {
        vec_t* colPtr = U + col * kVecsPerUCol;

        // Compute column norm
        float normSq;
        if
            constexpr(kIsComplex) {
                using cacc_t = ::aie::accum<caccfloat, kVecSizeComp>;
                cacc_t cacc = ::aie::mul(::aie::op_conj(colPtr[0]), colPtr[0]);
                for (int v = 1; v < kVecsPerUCol; v++) {
                    cacc = ::aie::mac(cacc, ::aie::op_conj(colPtr[v]), colPtr[v]);
                }
                cfloat norm_c = ::aie::reduce_add(cacc.template to_vector<cfloat>());
                normSq = norm_c.real;
            }
        else {
            acc_t nacc = ::aie::mul_square(colPtr[0]);
            for (int v = 1; v < kVecsPerUCol; v++) {
                nacc = ::aie::mac_square(nacc, colPtr[v]);
            }
            normSq = ::aie::reduce_add(nacc.template to_vector<float>());
        }
        float sigma = hw_sqrt(normSq);
        S[col] = sigma;

        float invSigma = hw_lt(kEpsilon, sigma) ? hw_inv(sigma) : 0.0f;
        for (int v = 0; v < kVecsPerUCol; v++) {
            colPtr[v] = ::aie::mul(colPtr[v], invSigma);
        }
    }
    // Phantom columns: set sigma=0 directly for the sort.
    for (int col = TP_DIM_COLS; col < (int)kStoreCols; col++) {
        S[col] = 0.0f;
    }

    // Sort real columns only; phantom columns are zero and naturally trail any real sigma.
    sortSingularValues<TT_DATA, kVecSizeComp>(S, U, V, TP_DIM_COLS, kVecsPerUCol, kVecsPerVCol);
    if
        constexpr(kStoreCols > TP_DIM_COLS) {
            TT_DATA* Vscalar_out = (TT_DATA*)outVWindow.data();
            for (int col = 0; col < (int)TP_DIM_COLS; col++) {
                for (int row = 0; row < (int)TP_DIM_COLS; row++) {
                    Vscalar_out[col * TP_DIM_COLS + row] = Vscalar_out[col * kStoreCols + row];
                }
            }
        }
}

//-----------------------------------------------------------------------------------------------------
// Stream phase helpers for cascaded SVD.
//
// NOINLINE_DECL forces each phase (1c forward accumulation, 1d backward broadcast)
// into a separate function with an opaque boundary. This prevents the chess compiler
// from scheduling stream operations from different phases into the same scheduling
// block, which causes stream deadlocks on AIE1 for certain CASC_LEN/kPairsPerSet
// configurations (both cfloat and large float pair counts).
//
// The functions are templated on compile-time kernel position flags so the compiler
// still eliminates dead branches, but stream ordering is guaranteed by the call boundary.
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int kPairsPerSet,
          unsigned int kScratchStride,
          unsigned int kVecSizeIO,
          bool kIsComplex,
          bool kIsFirst,
          bool kIsLast>
static NOINLINE_DECL void streamPhase1c(output_stream<TT_DATA>& outStream,
                                        input_stream<TT_DATA>& inStream,
                                        float* dotII,
                                        float* dotJJ,
                                        float* dotIJ,
                                        float* phase_real,
                                        float* phase_imag) {
    if
        constexpr(kIsFirst) {
            if
                constexpr(kIsComplex) {
                    for (int p = 0; p < kPairsPerSet; p++) {
                        writeincr(&outStream, TT_DATA{dotII[p], dotJJ[p]});
                        writeincr(&outStream, TT_DATA{phase_real[p], phase_imag[p]});
                    }
                }
            else {
                using svec_t = ::aie::vector<TT_DATA, kVecSizeIO>;
                for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotII + i));
                for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotJJ + i));
                for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotIJ + i));
            }
        }
    else {
        if
            constexpr(kIsComplex) {
                for (int p = 0; p < kPairsPerSet; p++) {
                    TT_DATA v1 = readincr(&inStream);
                    dotII[p] = hw_add(dotII[p], v1.real);
                    dotJJ[p] = hw_add(dotJJ[p], v1.imag);
                    TT_DATA v2 = readincr(&inStream);
                    phase_real[p] = hw_add(phase_real[p], v2.real);
                    phase_imag[p] = hw_add(phase_imag[p], v2.imag);
                }
            }
        else {
            using svec_t = ::aie::vector<TT_DATA, kVecSizeIO>;
            for (int i = 0; i < kScratchStride; i += kVecSizeIO) {
                svec_t inc = readincr_v<kVecSizeIO>(&inStream);
                *(svec_t*)(dotII + i) = ::aie::add(*(svec_t*)(dotII + i), inc);
            }
            for (int i = 0; i < kScratchStride; i += kVecSizeIO) {
                svec_t inc = readincr_v<kVecSizeIO>(&inStream);
                *(svec_t*)(dotJJ + i) = ::aie::add(*(svec_t*)(dotJJ + i), inc);
            }
            for (int i = 0; i < kScratchStride; i += kVecSizeIO) {
                svec_t inc = readincr_v<kVecSizeIO>(&inStream);
                *(svec_t*)(dotIJ + i) = ::aie::add(*(svec_t*)(dotIJ + i), inc);
            }
        }
        if
            constexpr(!kIsLast) {
                if
                    constexpr(kIsComplex) {
                        for (int p = 0; p < kPairsPerSet; p++) {
                            writeincr(&outStream, TT_DATA{dotII[p], dotJJ[p]});
                            writeincr(&outStream, TT_DATA{phase_real[p], phase_imag[p]});
                        }
                    }
                else {
                    using svec_t = ::aie::vector<TT_DATA, kVecSizeIO>;
                    for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotII + i));
                    for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotJJ + i));
                    for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotIJ + i));
                }
            }
    }
}

template <typename TT_DATA,
          unsigned int kPairsPerSet,
          unsigned int kScratchStride,
          unsigned int kVecSizeIO,
          bool kIsComplex,
          bool kIsLast,
          unsigned int kKernelPos,
          unsigned int kCascLen>
static NOINLINE_DECL void streamPhase1d(output_stream<TT_DATA>& outStream,
                                        input_stream<TT_DATA>& inStream,
                                        float* dotII,
                                        float* dotJJ,
                                        float* dotIJ,
                                        float* phase_real,
                                        float* phase_imag) {
    if
        constexpr(kIsLast) {
            if
                constexpr(kIsComplex) {
                    for (int p = 0; p < kPairsPerSet; p++) {
                        writeincr(&outStream, TT_DATA{dotII[p], dotJJ[p]});
                        writeincr(&outStream, TT_DATA{phase_real[p], phase_imag[p]});
                    }
                }
            else {
                using svec_t = ::aie::vector<TT_DATA, kVecSizeIO>;
                for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotII + i));
                for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotJJ + i));
                for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotIJ + i));
            }
        }
    else {
        if
            constexpr(kIsComplex) {
                for (int p = 0; p < kPairsPerSet; p++) {
                    TT_DATA v1 = readincr(&inStream);
                    dotII[p] = v1.real;
                    dotJJ[p] = v1.imag;
                    TT_DATA v2 = readincr(&inStream);
                    phase_real[p] = v2.real;
                    phase_imag[p] = v2.imag;
                }
            }
        else {
            using svec_t = ::aie::vector<TT_DATA, kVecSizeIO>;
            for (int i = 0; i < kScratchStride; i += kVecSizeIO)
                *(svec_t*)(dotII + i) = readincr_v<kVecSizeIO>(&inStream);
            for (int i = 0; i < kScratchStride; i += kVecSizeIO)
                *(svec_t*)(dotJJ + i) = readincr_v<kVecSizeIO>(&inStream);
            for (int i = 0; i < kScratchStride; i += kVecSizeIO)
                *(svec_t*)(dotIJ + i) = readincr_v<kVecSizeIO>(&inStream);
        }
        if
            constexpr(kKernelPos < kCascLen - 2) {
                if
                    constexpr(kIsComplex) {
                        for (int p = 0; p < kPairsPerSet; p++) {
                            writeincr(&outStream, TT_DATA{dotII[p], dotJJ[p]});
                            writeincr(&outStream, TT_DATA{phase_real[p], phase_imag[p]});
                        }
                    }
                else {
                    using svec_t = ::aie::vector<TT_DATA, kVecSizeIO>;
                    for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotII + i));
                    for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotJJ + i));
                    for (int i = 0; i < kScratchStride; i += kVecSizeIO) writeincr(&outStream, *(svec_t*)(dotIJ + i));
                }
            }
    }
}

//-----------------------------------------------------------------------------------------------------
// Multi-kernel cascaded SVD using Parallel Pairs with per-SET communication
//
// Each kernel holds TP_DIM_ROWS/TP_CASC_LEN rows of U and a full copy of V.
// Communication happens once per set (not per pair):
//   1. Forward accumulation of partial dot products
//   2. Backward broadcast of final dot products
//   3. Each kernel independently computes Givens and applies rotations locally
//
// Arbitrary TP_DIM_COLS: same kSchedCols/kStoreCols split as single kernel.
//-----------------------------------------------------------------------------------------------------

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
NOINLINE_DECL void svdCascadedMain(input_buffer<TT_DATA>& __restrict inWindow,
                                   input_stream<TT_DATA>* inStream,
                                   output_buffer<TT_DATA>& __restrict outUWindow,
                                   output_buffer<T_outSDataType>& __restrict outSWindow,
                                   output_buffer<TT_DATA>& __restrict outVWindow,
                                   output_stream<TT_DATA>* outStream,
                                   float* __restrict scratchBufArg) {
    // Vector configuration
    static constexpr bool kIsComplex = fnIsComplexDataType<TT_DATA>();
    static constexpr unsigned int kVecSizeIO = fnVecSampleNumIO<TT_DATA>();
    static constexpr unsigned int kVecSizeCompRaw = fnVecSampleNum<TT_DATA>();

    // --------------------------------------------------
    // Padding constants for arbitrary TP_DIM_COLS
    // kStoreCols uses kVecSizeIO (256-bit) to keep output port dimensions
    // unchanged. kVecSizeComp uses the native 512-bit width when dimensions allow.
    // --------------------------------------------------
    static constexpr unsigned int kSchedCols = TP_DIM_COLS + (TP_DIM_COLS % 2);
    static constexpr unsigned int kStoreAlign = fnVecStoreAlign<TT_DATA>();
    static constexpr unsigned int kStoreCols = ((kSchedCols + kStoreAlign - 1) / kStoreAlign) * kStoreAlign;

    // Partition and derived dimensions
    static constexpr int kRowsPerKernel = TP_DIM_ROWS / TP_CASC_LEN;
    static constexpr int kLocalInputSize = kRowsPerKernel * TP_DIM_COLS;

    static constexpr unsigned int kVecSizeComp =
        (kStoreCols % kVecSizeCompRaw == 0 && kRowsPerKernel % kVecSizeCompRaw == 0) ? kVecSizeCompRaw : kVecSizeIO;

    static constexpr int kPairsPerSet = kSchedCols / 2;
    static constexpr int kNumSets = kSchedCols - 1;
    static constexpr unsigned int kVecsPerUCol = kRowsPerKernel / kVecSizeComp;
    static constexpr unsigned int kVecsPerVCol = kStoreCols / kVecSizeComp;

    // Kernel position flags
    static constexpr bool isFirst = (TP_KERNEL_POSITION == 0);
    static constexpr bool isLast = (TP_KERNEL_POSITION == TP_CASC_LEN - 1);

    // Type aliases (use compute width)
    using vec_t = ::aie::vector<TT_DATA, kVecSizeComp>;
    using acc_t = ::aie::accum<accfloat, kVecSizeComp>;

    // Pointers to working memory
    // NOTE: V and Vscalar alias the same buffer, so neither may be marked
    // __restrict without invoking undefined behavior.
    vec_t* __restrict U = (vec_t*)outUWindow.data();
    vec_t* V = (vec_t*)outVWindow.data();
    TT_DATA* Vscalar = (TT_DATA*)outVWindow.data();
    T_outSDataType* __restrict S = (T_outSDataType*)outSWindow.data();
    vec_t* __restrict A_in = (vec_t*)inWindow.data();

    // Compile-time pair organization (uses kSchedCols, always even)
    constexpr auto& pairs = ParallelPairSets<kSchedCols>::pairs;

    // --------------------------------------------------
    // Stream I/O helpers: real float values transported via TT_DATA stream.
    // Defined once here and reused across all streaming phases.
    // --------------------------------------------------
    auto streamWrite = [&outStream](float val) {
        if
            constexpr(kIsComplex) writeincr(outStream, TT_DATA{val, 0.0f});
        else
            writeincr(outStream, (TT_DATA)val);
    };
    auto streamRead = [&inStream]() -> float {
        if
            constexpr(kIsComplex) return readincr(inStream).real;
        else
            return (float)readincr(inStream);
    };

    // --------------------------------------------------
    // Initialize: Copy local A partition to U, zero-pad phantom columns
    // --------------------------------------------------
    for (int i = 0; i < kLocalInputSize / kVecSizeComp; i++) {
        U[i] = A_in[i];
    }
    // Zero-pad schedule/storage phantom columns
    for (int col = TP_DIM_COLS; col < (int)kStoreCols; col++) {
        for (int v = 0; v < (int)kVecsPerUCol; v++) {
            U[col * kVecsPerUCol + v] = ::aie::zeros<TT_DATA, kVecSizeComp>();
        }
    }

    // Initialize full V to identity (kStoreCols x kStoreCols)
    // Zero entire V buffer via vectorised store, then set diagonal to 1.
    for (int v = 0; v < (int)(kStoreCols * kStoreCols / kVecSizeComp); v++) {
        V[v] = ::aie::zeros<TT_DATA, kVecSizeComp>();
    }
    for (int col = 0; col < (int)kStoreCols; col++) {
        if
            constexpr(kIsComplex) { Vscalar[col * kStoreCols + col] = TT_DATA{1.0f, 0.0f}; }
        else {
            Vscalar[col * kStoreCols + col] = 1.0f;
        }
    }

    // --------------------------------------------------
    // Working arrays in DATA MEMORY (not stack) to avoid register spilling.
    // --------------------------------------------------
    // Scratch buffer pointers — padded stride for vector stream I/O.
    // No __restrict: all slices of the same scratchBuf.
    static constexpr unsigned int kScratchStride = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kPaddedPairsPerSet;
    float* dotII = scratchBufArg;
    float* dotJJ = dotII + kScratchStride;
    float* dotIJ = dotJJ + kScratchStride;
    float* c_arr = dotIJ + kScratchStride;
    float* s_arr = c_arr + kScratchStride;
    float* phase_real = s_arr + kScratchStride;
    float* phase_imag = phase_real + kScratchStride;

    // --------------------------------------------------
    // Main Jacobi SVD iteration with per-SET communication
    // --------------------------------------------------
    for (int pass = 0; pass < TP_PASSES; pass++) {
        for (int setIdx = 0; setIdx < kNumSets; setIdx++) {
            int setStart = setIdx * kPairsPerSet;

            // ============ PHASE 1a+1b: Combined norms + cross-dot ============
            for (int p = 0; p < kPairsPerSet; p++) {
                int i = pairs[setStart + p][0];
                int j = pairs[setStart + p][1];
                vec_t* iPtr = U + i * kVecsPerUCol;
                vec_t* jPtr = U + j * kVecsPerUCol;

                float raw_real, raw_imag;
                computePairDots<TT_DATA, kVecSizeComp, kIsComplex>(iPtr, jPtr, dotII[p], dotJJ[p], raw_real, raw_imag,
                                                                   kVecsPerUCol);

                if
                    constexpr(kIsComplex) {
                        // Store raw complex dot; magnitude+phase normalised after stream exchange.
                        phase_real[p] = raw_real;
                        phase_imag[p] = raw_imag;
                        dotIJ[p] = 0.0f;
                    }
                else {
                    dotIJ[p] = raw_real;
                }
            }

            // ============ PHASE 1c: Stream forward accumulation ============
            // chess_memory_fence ensures Phase 1 scratch buffer stores commit
            // before Phase 1c reads them. NOINLINE stream phase functions create
            // hard compiler boundaries preventing cross-phase stream scheduling.
            chess_memory_fence();
            streamPhase1c<TT_DATA, kPairsPerSet, kScratchStride, kVecSizeIO, kIsComplex, isFirst, isLast>(
                *outStream, *inStream, dotII, dotJJ, dotIJ, phase_real, phase_imag);

            // ============ PHASE 1d: Backward broadcast of final dots ============
            chess_separator_scheduler();
            streamPhase1d<TT_DATA, kPairsPerSet, kScratchStride, kVecSizeIO, kIsComplex, isLast, TP_KERNEL_POSITION,
                          TP_CASC_LEN>(*outStream, *inStream, dotII, dotJJ, dotIJ, phase_real, phase_imag);
            chess_separator_scheduler();

            // For cfloat: extract magnitude and phase from accumulated complex dot
            if
                constexpr(kIsComplex) {
                    for (int p = 0; p < kPairsPerSet; p++) {
                        float mag =
                            hw_sqrt(hw_add(hw_mul(phase_real[p], phase_real[p]), hw_mul(phase_imag[p], phase_imag[p])));
                        dotIJ[p] = mag;
                        if (hw_lt(kEpsilon, mag)) {
                            float inv_mag = hw_inv(mag);
                            phase_real[p] = hw_mul(phase_real[p], inv_mag);
                            phase_imag[p] = hw_mul(phase_imag[p], inv_mag);
                        } else {
                            phase_real[p] = 1.0f;
                            phase_imag[p] = 0.0f;
                        }
                    }
                }

            // Scheduling barrier between phases.
            chess_separator_scheduler();

            // ============ PHASE 2: Scalar Givens (identical on all kernels) ============
            computeGivensRotations<kPairsPerSet>(dotII, dotJJ, dotIJ, c_arr, s_arr);

            chess_separator_scheduler();

            // ============ PHASE 3: Apply rotations locally (no communication) ============
            applyGivensRotations<TT_DATA, kVecSizeComp, kIsComplex>(U, V, c_arr, s_arr, phase_real, phase_imag, pairs,
                                                                    setStart, kPairsPerSet, kVecsPerUCol, kVecsPerVCol);
            // Ensure Phase 3 V stores are committed before the next set iteration
            // reads V. The cascade kernel is currently protected by the chess_memory_fence()
            // before Phase 1c, but making it explicit here is safer and consistent
            // with the single-kernel fix.
            chess_separator_scheduler();
        } // end setIdx
    }     // end pass

    // --------------------------------------------------
    // Finalize: Compute singular values via stream accumulation
    // Loop over kStoreCols columns (includes schedule/storage phantom columns).
    // --------------------------------------------------
    chess_memory_fence();
    for (int col = 0; col < (int)kStoreCols; col++) {
        vec_t* colPtr = U + col * kVecsPerUCol;
        float localNorm;
        if
            constexpr(kIsComplex) {
                using cacc_t = ::aie::accum<caccfloat, kVecSizeComp>;
                cacc_t cacc = ::aie::mul(::aie::op_conj(colPtr[0]), colPtr[0]);
                for (int v = 1; v < kVecsPerUCol; v++) {
                    cacc = ::aie::mac(cacc, ::aie::op_conj(colPtr[v]), colPtr[v]);
                }
                cfloat norm_c = ::aie::reduce_add(cacc.template to_vector<cfloat>());
                localNorm = norm_c.real;
            }
        else {
            acc_t acc = ::aie::mul_square(colPtr[0]);
            for (int v = 1; v < kVecsPerUCol; v++) {
                acc = ::aie::mac_square(acc, colPtr[v]);
            }
            localNorm = ::aie::reduce_add(acc.template to_vector<float>());
        }

        // Forward accumulate norms
        if
            constexpr(isFirst) { streamWrite(localNorm); }
        else {
            float accNorm = hw_add(localNorm, streamRead());
            if
                constexpr(!isLast) { streamWrite(accNorm); }
            else {
                float sigma = hw_sqrt(accNorm);
                S[col] = sigma;
                streamWrite(sigma);
            }
        }
    }

    // Backward broadcast of singular values
    chess_memory_fence();
    if
        constexpr(!isLast) {
            for (int col = 0; col < (int)kStoreCols; col++) {
                float sigma = streamRead();
                S[col] = sigma;
                if
                    constexpr(TP_KERNEL_POSITION < TP_CASC_LEN - 2) { streamWrite(sigma); }
            }
        }

    // Normalize local U partition (guard against zero sigma from phantom columns)
    for (int col = 0; col < (int)kStoreCols; col++) {
        vec_t* colPtr = U + col * kVecsPerUCol;
        float invSigma = hw_lt(kEpsilon, S[col]) ? hw_inv(S[col]) : 0.0f;
        for (int v = 0; v < kVecsPerUCol; v++) {
            colPtr[v] = ::aie::mul(colPtr[v], invSigma);
        }
    }

    // Sort real columns only; phantom columns are zero and naturally trail any real sigma.
    sortSingularValues<TT_DATA, kVecSizeComp>(S, U, V, TP_DIM_COLS, kVecsPerUCol, kVecsPerVCol);

    // Compact V in-place (see svdMain for rationale).
    if
        constexpr(kStoreCols > TP_DIM_COLS) {
            TT_DATA* Vscalar_out = (TT_DATA*)outVWindow.data();
            for (int col = 0; col < (int)TP_DIM_COLS; col++) {
                for (int row = 0; row < (int)TP_DIM_COLS; row++) {
                    Vscalar_out[col * TP_DIM_COLS + row] = Vscalar_out[col * kStoreCols + row];
                }
            }
        }
}

//-----------------------------------------------------------------------------------------------------
// Entry points for cascaded kernel specializations
//-----------------------------------------------------------------------------------------------------

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
NOINLINE_DECL void
SVDecomposition_First<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION>::svdMain(
    input_buffer<TT_DATA>& __restrict inWindow,
    input_stream<TT_DATA>* inStream,
    output_buffer<TT_DATA>& __restrict outUWindow,
    output_buffer<T_outSDataType>& __restrict outSWindow,
    output_buffer<TT_DATA>& __restrict outVWindow,
    output_stream<TT_DATA>* outStream) {
    svdCascadedMain<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION>(
        inWindow, inStream, outUWindow, outSWindow, outVWindow, outStream, this->scratchBuf);
}

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
NOINLINE_DECL void
SVDecomposition_Middle<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION>::svdMain(
    input_buffer<TT_DATA>& __restrict inWindow,
    input_stream<TT_DATA>* inStream,
    output_buffer<TT_DATA>& __restrict outUWindow,
    output_buffer<T_outSDataType>& __restrict outSWindow,
    output_buffer<TT_DATA>& __restrict outVWindow,
    output_stream<TT_DATA>* outStream) {
    svdCascadedMain<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION>(
        inWindow, inStream, outUWindow, outSWindow, outVWindow, outStream, this->scratchBuf);
}

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KERNEL_POSITION>
NOINLINE_DECL void
SVDecomposition_Last<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION>::svdMain(
    input_buffer<TT_DATA>& __restrict inWindow,
    input_stream<TT_DATA>* inStream,
    output_buffer<TT_DATA>& __restrict outUWindow,
    output_buffer<T_outSDataType>& __restrict outSWindow,
    output_buffer<TT_DATA>& __restrict outVWindow,
    output_stream<TT_DATA>* outStream) {
    svdCascadedMain<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION>(
        inWindow, inStream, outUWindow, outSWindow, outVWindow, outStream, this->scratchBuf);
}

} // namespace svd
} // namespace aie
} // namespace solver
} // namespace xf
