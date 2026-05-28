/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
          unsigned int TP_CASC_LEN>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&svdKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = cascPos - 1;
        static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
        alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);
        svdKernels[cascPos - 1] = kernel::create_object<
            SVDecomposition_Middle<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION> >(
            scratchBuf);
        create_casc_kernel_recur<cascPos - 1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN>::create(
            svdKernels);
    }
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN>
class create_casc_kernel_recur<1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN> {
   public:
    static void create(kernel (&svdKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
        alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);
        svdKernels[0] = kernel::create_object<
            SVDecomposition_First<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION> >(
            scratchBuf);
    }
};

template <int cascPos,
          typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN>
class create_casc_kernel {
   public:
    static void create(kernel (&svdKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = cascPos - 1;
        static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
        alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);
        svdKernels[cascPos - 1] = kernel::create_object<
            SVDecomposition_Last<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION> >(
            scratchBuf);
        create_casc_kernel_recur<cascPos - 1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN>::create(
            svdKernels);
    }
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN>
class create_casc_kernel<1, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN> {
   public:
    static void create(kernel (&svdKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        static constexpr unsigned int kScratchSize = SvdScratchSize<TT_DATA, TP_DIM_COLS>::kSize;
        alignas(__ALIGN_BYTE_SIZE__) std::vector<float> scratchBuf(kScratchSize);
        svdKernels[0] = kernel::create_object<
            SVDecomposition<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_KERNEL_POSITION> >(
            scratchBuf);
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
 **/
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_PASSES,
          unsigned int TP_CASC_LEN>
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
    static constexpr int kInWindowVsize   = TP_DIM_ROWS * TP_DIM_COLS;
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
    static constexpr unsigned int kEstimatedKernelDataBytes = kLocalInWindowBytes + kLocalUWindowBytes +
                                                              kLocalSWindowBytes + kLocalVWindowBytes +
                                                              kLocalScratchBytes + kStackBytes;

    static_assert(std::is_same<TT_DATA, float>::value || std::is_same<TT_DATA, cfloat>::value,
                  "ERROR: TT_DATA must be either float or cfloat.");
    static_assert(TP_DIM_COLS >= 2 && TP_DIM_COLS <= 128, "ERROR: TP_DIM_COLS must be in the range [2, 128].");
    static_assert(TP_PASSES >= 1 && TP_PASSES <= 10, "ERROR: TP_PASSES must be in the range [1, 10].");
    static_assert(TP_CASC_LEN >= 1 && TP_CASC_LEN <= 16, "ERROR: TP_CASC_LEN must be in the range [1, 16].");
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
     * The output S vector (singular values, on the diagonal). Each kernel outputs the full singular value vector.
     **/
    port<output> outS[TP_CASC_LEN];
    /**
     * The output V matrix (orthonormal, right singular vectors). Each kernel outputs the full V matrix.
     **/
    port<output> outV[TP_CASC_LEN];

    /**
     * The array of kernels mapped onto AIE tiles.
     **/
    kernel m_svdKernel[TP_CASC_LEN];

    /**
    * Access function to get pointer to kernel (or first kernel in a chained configuration).
    * No arguments required.
    **/
    kernel* getKernels() { return m_svdKernel; };

    /**
     * @brief This is the constructor function for the svd graph.
     **/
    svd_graph() {
        create_casc_kernel<TP_CASC_LEN, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN>::create(m_svdKernel);

        for (int k = 0; k < TP_CASC_LEN; k++) {
            connect(in[k], m_svdKernel[k].in[0]);
            dimensions(m_svdKernel[k].in[0]) = {kInWindowVsize / TP_CASC_LEN};

            connect(m_svdKernel[k].out[0], outU[k]);
            dimensions(m_svdKernel[k].out[0]) = {kOutUWindowVsize / TP_CASC_LEN};
            connect(m_svdKernel[k].out[1], outS[k]);
            dimensions(m_svdKernel[k].out[1]) = {kOutSWindowVsize};
            connect(m_svdKernel[k].out[2], outV[k]);
            dimensions(m_svdKernel[k].out[2]) = {kOutVWindowVsize};

            runtime<ratio>(m_svdKernel[k]) = 0.9;
            source(m_svdKernel[k]) = "svd.cpp";
            headers(m_svdKernel[k]) = {"svd.hpp"};
            stack_size(m_svdKernel[k]) = kStackBytes;

            // Use single_buffer when internal kernel buffer exceeds half of data memory.
            // Use internal sizes (kStoreCols-based) not output port sizes (TP_DIM_COLS-based).
            if (kLocalUWindowBytes > __DATA_MEM_BYTES__ / 2) {
                single_buffer(m_svdKernel[k].out[0]);
            }
            if (kLocalVWindowBytes > __DATA_MEM_BYTES__ / 2) {
                single_buffer(m_svdKernel[k].out[2]);
            }
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

            auto conn_last_to_first = connect<stream>(m_svdKernel[TP_CASC_LEN - 1].out[3], m_svdKernel[0].in[1]);
            fifo_depth(conn_last_to_first) = fifoDepthVal;

            for (int k = 0; k < TP_CASC_LEN - 1; k++) {
                auto conn = connect<stream>(m_svdKernel[k].out[3], m_svdKernel[k + 1].in[1]);
                fifo_depth(conn) = fifoDepthVal;
            }
        }
    };
};

} // namespace svd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _DSPLIB_SVD_GRAPH_HPP_
