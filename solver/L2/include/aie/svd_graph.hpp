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
#ifndef _DSPLIB_SVD_GRAPH_HPP_
#define _DSPLIB_SVD_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the SVD function library element.
*/
/**
 * @file svd_graph.hpp
 *
 **/

#include <adf.h>
#include <type_traits>
#include <vector>
#include "graph_utils.hpp"
#include "device_defs.h"
#include "svd.hpp"

using namespace adf;
namespace xf {
namespace solver {
namespace aie {
namespace svd {

/**
  * @cond NOCOMMENTS
  */

// Recursive kernel creation for cascaded svd kernels
template <int cascPos,
          typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIAG_INV>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&svdKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = cascPos - 1;
        static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
        alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);
        svdKernels[cascPos - 1] =
            kernel::create_object<SVDecomposition_Middle<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN,
                                                         TP_DIAG_INV, TP_KERNEL_POSITION> >(scratchBuf);
        create_casc_kernel_recur<cascPos - 1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN,
                                 TP_DIAG_INV>::create(svdKernels);
    }
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIAG_INV>
class create_casc_kernel_recur<1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_DIAG_INV> {
   public:
    static void create(kernel (&svdKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
        alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);
        svdKernels[0] =
            kernel::create_object<SVDecomposition_First<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN,
                                                        TP_DIAG_INV, TP_KERNEL_POSITION> >(scratchBuf);
    }
};

template <int cascPos,
          typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIAG_INV>
class create_casc_kernel {
   public:
    static void create(kernel (&svdKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = cascPos - 1;
        static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
        alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);
        svdKernels[cascPos - 1] =
            kernel::create_object<SVDecomposition_Last<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN,
                                                       TP_DIAG_INV, TP_KERNEL_POSITION> >(scratchBuf);
        create_casc_kernel_recur<cascPos - 1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN,
                                 TP_DIAG_INV>::create(svdKernels);
    }
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIAG_INV>
class create_casc_kernel<1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_DIAG_INV> {
   public:
    static void create(kernel (&svdKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
        alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);
        svdKernels[0] = kernel::create_object<SVDecomposition<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN,
                                                              TP_DIAG_INV, TP_KERNEL_POSITION> >(scratchBuf);
    }
};

template <int linearPos,
          typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PASSES_SSR,
          unsigned int TP_DIAG_INV>
struct create_2d_svd_kernels {
    static void create(kernel (&svdKernels)[TP_PASSES_SSR][TP_CASC_LEN]) {
        if
            constexpr(linearPos < (int)(TP_PASSES_SSR * TP_CASC_LEN)) {
                static constexpr unsigned int kPassPos = linearPos / TP_CASC_LEN;
                static constexpr unsigned int kCascPos = linearPos % TP_CASC_LEN;
                // Normally the passes are divided evenly across all stages. The
                // one permitted over-subscription value, TP_PASSES + 1, assigns
                // one pass to each of the first TP_PASSES stages and zero passes
                // to the final stage, making it a dedicated finalizer.
                static constexpr bool kFinalizeOnlyStage = (TP_PASSES_SSR == TP_PASSES + 1) && (kPassPos == TP_PASSES);
                static constexpr unsigned int kStagePasses =
                    TP_PASSES_SSR == TP_PASSES + 1 ? (kFinalizeOnlyStage ? 0 : 1) : TP_PASSES / TP_PASSES_SSR;
                static constexpr bool kFinalStage = (kPassPos == TP_PASSES_SSR - 1);
                static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
                alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);

                if
                    constexpr(TP_CASC_LEN == 1) {
                        if
                            constexpr(kPassPos == 0) {
                                svdKernels[kPassPos][0] = kernel::create_object<SVDecomposition_2D_RawSingle<
                                    TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, kStagePasses, TP_DIAG_INV, kFinalStage> >(
                                    scratchBuf);
                            }
                        else {
                            svdKernels[kPassPos][0] = kernel::create_object<SVDecomposition_2D_StateSingle<
                                TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, kStagePasses, TP_DIAG_INV, kFinalStage> >(
                                scratchBuf);
                        }
                    }
                else {
                    if
                        constexpr(kPassPos == 0) {
                            svdKernels[kPassPos][kCascPos] = kernel::create_object<
                                SVDecomposition_2D_RawCasc<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, kStagePasses, TP_CASC_LEN,
                                                           kCascPos, TP_DIAG_INV, kFinalStage> >(scratchBuf);
                        }
                    else {
                        svdKernels[kPassPos][kCascPos] = kernel::create_object<
                            SVDecomposition_2D_StateCasc<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, kStagePasses, TP_CASC_LEN,
                                                         kCascPos, TP_DIAG_INV, kFinalStage> >(scratchBuf);
                    }
                }
                create_2d_svd_kernels<linearPos + 1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN,
                                      TP_PASSES_SSR, TP_DIAG_INV>::create(svdKernels);
            }
    }
};
/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// svd_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @defgroup svd_graph SVD function
 *
 * Singular Value Decomposition function graph class
 **/

/**
 * @ingroup svd_graph
 * @brief svd performs the decomposition of a matrix into three matrices
 *        (one diagonal, two orthonormal) such that M = USV^H.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function.
 *         This is a typename and must be one of the following: \n
 *         float, cfloat.
 * @tparam TP_DIM_ROWS describes the number of rows in the input matrix.
 * @tparam TP_DIM_COLS describes the number of columns in the input matrix.
 * @tparam TP_PASSES describes the number of Jacobi sweep passes (for convergence).
 * @tparam TP_CASC_LEN selects the number of kernels the SVD will be split over in series.
 * @tparam TP_PASSES_SSR selects the number of pass-pipeline stages.
 *         For values in the range [1, TP_PASSES], TP_PASSES must be evenly
 *         divisible by TP_PASSES_SSR and each stage performs
 *         TP_PASSES / TP_PASSES_SSR Jacobi passes. The additional value
 *         TP_PASSES + 1 is also legal. In that mode the first TP_PASSES
 *         stages perform one Jacobi pass each and the last stage performs no
 *         Jacobi pass; it is dedicated to singular-value extraction, U
 *         normalization, U/S/V sorting, and V compaction. This extra stage is
 *         intended as a final throughput optimization after the Jacobi loop
 *         has already been maximally distributed with
 *         TP_PASSES_SSR = TP_PASSES.
 * @tparam TP_DIAG_INV selects singular values (0, default) or reciprocal singular values (1).
 **/
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PASSES_SSR = 1,
          unsigned int TP_DIAG_INV = 0>
class svd_graph : public graph {
   public:
    static constexpr bool kIsComplex = fnIsComplexDataType<TT_DATA>();
    // kVecSize: 256-bit IO width for port sizing, legality checks, and stream operations.
    // Fixed at 256-bit on all variants — x86sim for AIE22 does not support 512-bit streams.
    static constexpr unsigned int kVecSize = fnVecSampleNumIO<TT_DATA>();

    // kSchedCols: minimum even column count needed by the Jacobi schedule.
    // kStoreCols: next multiple of kStoreAlign >= kSchedCols for V column alignment.
    // kStoreAlign is 256-bit on AIE1/AIE-ML but 512-bit on AIE22. The wider alignment
    // forces kVecSizeComp=native (v16float) on AIE22, avoiding a chess compiler (aie2ps)
    // defect where 256-bit V column stores produce duplicate columns. Stream operations
    // still use kVecSizeIO=256-bit so x86sim remains functional.
    static constexpr unsigned int kStoreAlign = fnVecStoreAlign<TT_DATA>();
    static constexpr unsigned int kSchedCols = TP_DIM_COLS + (TP_DIM_COLS % 2);
    static constexpr unsigned int kStoreCols = ((kSchedCols + kStoreAlign - 1) / kStoreAlign) * kStoreAlign;
    static constexpr unsigned int kRowsPerKernel = TP_DIM_ROWS / TP_CASC_LEN;
    static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;

    // Buffer sizes.
    static constexpr int kInWindowVsize = TP_DIM_ROWS * TP_DIM_COLS;
    static constexpr int kOutUWindowVsize = TP_DIM_ROWS * kStoreCols;
    static constexpr int kOutSWindowVsize = kStoreCols;
    static constexpr int kOutVWindowVsize = kStoreCols * kStoreCols;
    static constexpr unsigned int kLocalInWindowBytes = (kInWindowVsize / TP_CASC_LEN) * sizeof(TT_DATA);
    static constexpr unsigned int kLocalUWindowBytes = (kOutUWindowVsize / TP_CASC_LEN) * sizeof(TT_DATA);
    static constexpr unsigned int kLocalSWindowBytes = kOutSWindowVsize * sizeof(T_outSDataType);
    static constexpr unsigned int kLocalVWindowBytes = kOutVWindowVsize * sizeof(TT_DATA);
    static constexpr unsigned int kLocalScratchBytes = kScratchSize * sizeof(float);
    static constexpr unsigned int kStackBytes = 4096;
    // Note: ADF double-buffers I/O windows by default (2x per buffer), except where single_buffer()
    // is applied below for large buffers. This estimate uses single-buffer sizes as a lower bound;
    // actual DM usage may be up to ~2x for configs where single_buffer is not triggered.
    static constexpr unsigned int kLegacyKernelDataBytes =
        kLocalInWindowBytes + kLocalUWindowBytes + kLocalSWindowBytes + kLocalVWindowBytes + kLocalScratchBytes;
    static constexpr unsigned int kStagedKernelDataBytes =
        2 * (kLocalUWindowBytes + kLocalSWindowBytes + kLocalVWindowBytes) + kLocalScratchBytes;
    static constexpr unsigned int kEstimatedKernelDataBytes =
        TP_PASSES_SSR == 1 ? kLegacyKernelDataBytes : kStagedKernelDataBytes;

    static_assert(std::is_same<TT_DATA, float>::value || std::is_same<TT_DATA, cfloat>::value,
                  "ERROR: TT_DATA must be either float or cfloat.");
    static_assert(TP_DIM_COLS >= 2 && TP_DIM_COLS <= 128, "ERROR: TP_DIM_COLS must be in the range [2, 128].");
    static_assert(TP_PASSES >= 1 && TP_PASSES <= 10, "ERROR: TP_PASSES must be in the range [1, 10].");
    static_assert(TP_PASSES_SSR >= 1 && TP_PASSES_SSR <= TP_PASSES + 1,
                  "ERROR: TP_PASSES_SSR must be in the range [1, TP_PASSES + 1].");
    static_assert(TP_PASSES_SSR == TP_PASSES + 1 || TP_PASSES % TP_PASSES_SSR == 0,
                  "ERROR: TP_PASSES_SSR must divide TP_PASSES, or equal TP_PASSES + 1 "
                  "to create a dedicated finalize-only stage.");
    static_assert(TP_CASC_LEN >= 1 && TP_CASC_LEN <= 16, "ERROR: TP_CASC_LEN must be in the range [1, 16].");
    static_assert(TP_DIAG_INV == 0 || TP_DIAG_INV == 1, "ERROR: TP_DIAG_INV must be either 0 or 1.");
    static_assert(TP_DIM_ROWS >= kVecSize, "ERROR: TP_DIM_ROWS must be at least one internal vector.");
    static_assert(TP_DIM_ROWS % kVecSize == 0, "ERROR: TP_DIM_ROWS must be a multiple of the internal vector size.");
    static_assert(TP_DIM_ROWS % TP_CASC_LEN == 0, "ERROR: TP_DIM_ROWS must be divisible by TP_CASC_LEN.");
    static_assert(kRowsPerKernel % kVecSize == 0,
                  "ERROR: TP_DIM_ROWS / TP_CASC_LEN must be a multiple of the internal vector size.");
    static_assert(kEstimatedKernelDataBytes <= __DATA_MEM_BYTES__,
                  "ERROR: Estimated per-kernel data memory footprint exceeds __DATA_MEM_BYTES__. "
                  "Reduce TP_DIM_ROWS, TP_DIM_COLS, or TP_CASC_LEN.");

    /**
     * The input data to the function. Each kernel receives a unique partition of the input matrix rows.
     **/
    port<input> in[TP_CASC_LEN];

    /**
     * The output U matrix (orthonormal, left singular vectors). Each kernel outputs its row partition.
     **/
    port<output> outU[TP_CASC_LEN];
    /**
     * The output S vector: singular values when TP_DIAG_INV=0, or reciprocal singular values when TP_DIAG_INV=1. Each
     *kernel outputs the full vector.
     **/
    port<output> outS[TP_CASC_LEN];
    /**
     * The output V matrix (orthonormal, right singular vectors). Each kernel outputs the full V matrix.
     **/
    port<output> outV[TP_CASC_LEN];

    /**
     * The array of kernels mapped onto AIE tiles.
     **/
    kernel m_svdKernel[TP_PASSES_SSR][TP_CASC_LEN];

    /**
    * Access function to get pointer to kernel (or first kernel in a chained configuration).
    * No arguments required.
    **/
    kernel* getKernels() { return &m_svdKernel[0][0]; };

    /**
     * @brief This is the constructor function for the svd graph.
     **/
    svd_graph() {
        create_2d_svd_kernels<0, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_PASSES_SSR,
                              TP_DIAG_INV>::create(m_svdKernel);

        for (int p = 0; p < TP_PASSES_SSR; p++) {
            for (int k = 0; k < TP_CASC_LEN; k++) {
                if (p == 0) {
                    dimensions(m_svdKernel[p][k].in[0]) = {kInWindowVsize / TP_CASC_LEN};
                    if (kLocalInWindowBytes > (__DATA_MEM_BYTES__ / 2)) {
                        single_buffer(m_svdKernel[p][k].in[0]);
                    }
                } else {
                    dimensions(m_svdKernel[p][k].in[0]) = {kOutUWindowVsize / TP_CASC_LEN};
                    if (kLocalUWindowBytes > (__DATA_MEM_BYTES__ / 2)) {
                        single_buffer(m_svdKernel[p][k].in[0]);
                    }
                    dimensions(m_svdKernel[p][k].in[1]) = {kOutSWindowVsize};
                    if (kLocalSWindowBytes > (__DATA_MEM_BYTES__ / 2)) {
                        single_buffer(m_svdKernel[p][k].in[1]);
                    }
                    dimensions(m_svdKernel[p][k].in[2]) = {kOutVWindowVsize};
                    if (kLocalVWindowBytes > (__DATA_MEM_BYTES__ / 2)) {
                        single_buffer(m_svdKernel[p][k].in[2]);
                    }
                }
                dimensions(m_svdKernel[p][k].out[0]) = {kOutUWindowVsize / TP_CASC_LEN};
                if (kLocalUWindowBytes > (__DATA_MEM_BYTES__ / 2)) {
                    single_buffer(m_svdKernel[p][k].out[0]);
                }
                dimensions(m_svdKernel[p][k].out[1]) = {kOutSWindowVsize};
                if (kLocalSWindowBytes > (__DATA_MEM_BYTES__ / 2)) {
                    single_buffer(m_svdKernel[p][k].out[1]);
                }
                dimensions(m_svdKernel[p][k].out[2]) = {kOutVWindowVsize};
                if (kLocalVWindowBytes > (__DATA_MEM_BYTES__ / 2)) {
                    single_buffer(m_svdKernel[p][k].out[2]);
                }

                runtime<ratio>(m_svdKernel[p][k]) = 0.9;
                source(m_svdKernel[p][k]) = "svd.cpp";
                headers(m_svdKernel[p][k]) = {"svd.hpp"};
                stack_size(m_svdKernel[p][k]) = kStackBytes;
            }
        }

        for (int k = 0; k < TP_CASC_LEN; k++) {
            connect(in[k], m_svdKernel[0][k].in[0]);
            for (int p = 0; p < TP_PASSES_SSR - 1; p++) {
                connect(m_svdKernel[p][k].out[0], m_svdKernel[p + 1][k].in[0]);
                connect(m_svdKernel[p][k].out[1], m_svdKernel[p + 1][k].in[1]);
                connect(m_svdKernel[p][k].out[2], m_svdKernel[p + 1][k].in[2]);
            }
            connect(m_svdKernel[TP_PASSES_SSR - 1][k].out[0], outU[k]);
            connect(m_svdKernel[TP_PASSES_SSR - 1][k].out[1], outS[k]);
            connect(m_svdKernel[TP_PASSES_SSR - 1][k].out[2], outV[k]);
        }

        // Stream connections for cascaded mode (ring topology)
        if (TP_CASC_LEN > 1) {
            // FIFO depth: buffer max data in-flight at any moment.
            // Float path uses vector stream writes with padded stride (kPaddedPairsPerSet
            // values per slice). cfloat uses scalar writes (kPairsPerSet per slice).
            // Finalize broadcasts kStoreCols sigma values.
            static constexpr int kPaddedPairs = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kPaddedPairsPerSet;
            // cfloat: 2 packed cfloat writes per pair (dotII+dotJJ, phase_r+phase_i).
            // float: 3 padded-stride slices of kPaddedPairs values each.
            static constexpr int kSetPayload = kIsComplex ? 2 * (kSchedCols / 2) : 3 * kPaddedPairs;
            static constexpr int kMaxInFlight = (kSetPayload > (int)kStoreCols) ? kSetPayload : (int)kStoreCols;
            static constexpr int fifoDepthVal = kMaxInFlight * 2;

            for (int p = 0; p < TP_PASSES_SSR; p++) {
                const int streamInPort = p == 0 ? 1 : 3;
                auto conn_last_to_first =
                    connect<stream>(m_svdKernel[p][TP_CASC_LEN - 1].out[3], m_svdKernel[p][0].in[streamInPort]);
                fifo_depth(conn_last_to_first) = fifoDepthVal;

                for (int k = 0; k < TP_CASC_LEN - 1; k++) {
                    auto conn = connect<stream>(m_svdKernel[p][k].out[3], m_svdKernel[p][k + 1].in[streamInPort]);
                    fifo_depth(conn) = fifoDepthVal;
                }
            }
        }
    };
};

} // namespace svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _DSPLIB_SVD_GRAPH_HPP_
