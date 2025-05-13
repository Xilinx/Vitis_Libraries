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
#ifndef _DSPLIB_MATRIX_MULT_GRAPH_HPP_
#define _DSPLIB_MATRIX_MULT_GRAPH_HPP_

// This file holds the definition of the matrix mult graph class
/**
 * @file matrix_mult_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"

#include "matrix_mult_tiling_scheme.hpp"
#include "matrix_mult.hpp"
#include "matrix_mult_untiler.hpp"
#include "matrix_mult_tiler.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

using namespace adf;

/**
 * @defgroup gemm_graph General Matrix Multiply (GEMM)
 *
 * Matrix Multiply/GEMM (General Matrix Multiply) solution.
 *
 */

/**
 * @brief matrix_mult performs a General Matrix Multiply (GEMM), taking two input matrices of configurable dimensions
 *and data type.
 *
 * These are the templates to configure the Matrix Multiply graph class.
 *
 * @ingroup gemm_graph
 *
 * @tparam TT_DATA_A describes the type of individual data samples input of
 *         Matrix A to the gemm function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_DATA_B describes the type of individual data samples input of
 *         Matrix B to the gemm function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 *         The following rules apply:
 *         - must be an integer type if TT_DATA_A is an integer type
 *         - must be a float type if TT_DATA_A is a float type.
 * @tparam TP_DIM_A is an unsigned integer which describes the number of elements
 *          along the unique dimension (rows) of Matrix A.
 * @tparam TP_DIM_AB is an unsigned integer which describes the number of elements
 *          along the common dimension of Matrix A (columns) and Matrix B (rows).
 * @tparam TP_DIM_B is an unsigned integer which describes the number of elements
 *          along the unique dimension (columns) of Matrix B.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         product terms before each output. ``TP_SHIFT`` must be in the range 0 to 59 (61 for AIE1).
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. \n
 *         Although, TP_RND accepts unsigned integer values descriptive macros are recommended where
 *         - rnd_floor      = Truncate LSB, always round down (towards negative infinity).
 *         - rnd_ceil       = Always round up (towards positive infinity).
 *         - rnd_sym_floor  = Truncate LSB, always round towards 0.
 *         - rnd_sym_ceil   = Always round up towards infinity.
 *         - rnd_pos_inf    = Round halfway towards positive infinity.
 *         - rnd_neg_inf    = Round halfway towards negative infinity.
 *         - rnd_sym_inf    = Round halfway towards infinity (away from zero).
 *         - rnd_sym_zero   = Round halfway towards zero (away from infinity).
 *         - rnd_conv_even  = Round halfway towards nearest even number.
 *         - rnd_conv_odd   = Round halfway towards nearest odd number. \n
 *         No rounding is performed on ceil or floor mode variants. \n
 *         Other modes round to the nearest integer. They differ only in how
 *         they round for values of 0.5. \n
 *         \n
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML and AIE-MLv2 device.
 *\n
 * @tparam TP_DIM_A_LEADING describes the scheme in which the data should be stored
 *         in memory. ROW_MAJOR = 0, COL_MAJOR = 1. Note, a COL_MAJOR matrix can be
 *         transposed to become a ROW_MAJOR matrix.
 * @tparam TP_DIM_B_LEADING describes the scheme in which the data should be stored
 *         in memory. ROW_MAJOR = 0, COL_MAJOR = 1.
 * @tparam TP_DIM_OUT_LEADING describes the scheme in which the data should be stored
 *         in memory. ROW_MAJOR = 0, COL_MAJOR = 1.
 * @tparam TP_ADD_TILING_A describes wether or not to add an additional kernel to
 *          rearrange the matrix samples into their required position. \n Setting this
 *          option to 0 indicates that the re-arrangement will be done externally to
 *          the AIE matrix multiply graph.
 * @tparam TP_ADD_TILING_B describes wether or not to add an additional kernel to
 *          rearrange the matrix samples into their required position. \n Setting this
 *          option to 0 indicates that the re-arrangement will be done externally to
 *          the AIE matrix multiply graph.
 * @tparam TP_ADD_DETILING_OUT describes wether or not to add an additional kernel to
 *          rearrange the matrix samples into their required position. \n Setting this
 *          option to 0 indicates that the re-arrangement will be done externally to
 *          the AIE matrix multiply graph.
 * @tparam TP_INPUT_WINDOW_VSIZE_A describes the number of samples in the window API
 *         used for input to Matrix A. \n It must be of size TP_DIM_A*TP_DIM_AB.
 * @tparam TP_INPUT_WINDOW_VSIZE_B describes the number of samples in the window API
 *         used for input to Matrix B. \n It must be of size TP_DIM_B*TP_DIM_AB
 *         Note, the output window will be of size TP_DIM_A * TP_DIM_B.
 * @tparam TP_CASC_LEN describes the number of AIE kernels the matrix multiplication will be divided into in series. \n
 *         TP_CASC_LEN splits the operation over shared dimension TP_DIM_AB, where each kernel
 *         utilizes the cascade stream to pass partial accumulation results to
 *         the next kernel. In effect, dot(A,B) + C. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 * @tparam TP_SSR describes the number of kernels (or cascaded kernel chains) that will compute the matrix
 *         multiplication in parallel.
 *         Each SSR rank will receive an equal sized split (along the unique dimension) of Matrix A data. \n
 *         There is no splitting of the Matrix B data when TP_SSR > 1 (only split when TP_CASC_LEN > 1).
 *         The Matrix B inputs across a
 *         chain of cascaded kernels will be the same across all SSR ranks
 * @tparam TT_OUT_DATA describes the type of data samples in the output matrix. \n
 *         It must be int16, cint16, int32, cint32, float, or cfloat.
 *         ``TT_OUT_DATA`` must also satisfy the following rules:
 *         - Complex types are only supported when either ``TT_DATA_A`` or ``TT_DATA_B`` is also complex.
 *         - ``TT_OUT_DATA`` must be same or greater precision than both of the input data types,
 *         e.g. 32-bit ``TT_OUT_DATA``, when ``TT_DATA_A`` and ``TT_DATA_B`` is 16-bit.
 *         - ``TT_OUT_DATA`` must be an integer type if ``TT_DATA_A`` and ``TT_DATA_B`` are integer types
 *         - ``TT_OUT_DATA`` must be a float type if ``TT_DATA_A`` and ``TT_DATA_B`` are float types.
**/

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_ADD_TILING_A = 1,
          unsigned int TP_ADD_TILING_B = 1,
          unsigned int TP_ADD_DETILING_OUT = 1,
          unsigned int TP_INPUT_WINDOW_VSIZE_A = TP_DIM_A* TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B = TP_DIM_B* TP_DIM_AB,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_SAT = 1,
          unsigned int TP_SSR = 1,
          typename TT_OUT_DATA = outType_t<TT_DATA_A, TT_DATA_B> >
class matrix_mult_graph : public graph {
   public:
    /**
     * The input A data to the function. This input is a window of samples of
     * TT_DATA_A type. The number of samples in the window is
     * described by TP_INPUT_WINDOW_VSIZE_A, which is
     * derived from (TP_DIM_A / TP_SSR) * (TP_DIM_AB / TP_CASC_LEN).
     * There are TP_CASC_LEN * TP_SSR input A ports.
     **/
    port<input> inA[TP_CASC_LEN * TP_SSR];

    /**
     * The input B data to the function. This input is a window of samples of
     * TT_DATA_B type. The number of samples in the window is
     * described by TP_INPUT_WINDOW_VSIZE_B, which is
     * derived from (TP_DIM_AB / TP_CASC_LEN) * TP_DIM_B.
     * There are TP_CASC_LEN * TP_SSR input B ports.
     **/
    port<input> inB[TP_CASC_LEN * TP_SSR];

    /**
     * A window API of
     * (TP_DIM_A / TP_SSR) * TP_DIM_B samples of a derived output type.
     * There are TP_SSR output ports.
     **/
    port<output> out[TP_SSR];

    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     * There will be TP_SSR number of parallel cascade chains of length TP_CASC_LEN.
     **/
    kernel m_MatmultKernels[TP_CASC_LEN * TP_SSR];

    /**
     * The kernels that that will be created when output tiling is enabled (``TP_ADD_DETILING_OUT = 1``) for each SSR
     *rank.
     **/
    kernel untiler[TP_SSR];

    /**
     * The array of kernels that will be created when tiling on input A is enabled (``TP_ADD_TILING_A = 1``).
     * Kernels will pre-process and sent the data through cascade interface to corresponding: ``m_MatmultKernels``.
     **/
    kernel tilerA[TP_CASC_LEN * TP_SSR];

    /**
     * The array of kernels that will be created when tiling on input A is enabled (``TP_ADD_TILING_A = 1``).
     * Kernels will pre-process and sent the data through cascade interface to corresponding: ``m_MatmultKernels``.
     **/
    kernel tilerB[TP_CASC_LEN * TP_SSR];

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/

    kernel* getKernels() { return m_MatmultKernels; };

    // Empty type for a fallback to avoid redundant instantiations from the compiler in x86
    /**
     * @cond NOCOMMENTS
     */
    struct no_kernel {};

    // Flag request for Tiler/Untiler with COL_MAJOR
    static constexpr int isTilingReq =
        (TP_ADD_TILING_A == 1 || TP_ADD_TILING_B == 1 || TP_ADD_DETILING_OUT == 1) ? 1 : 0;
    static constexpr int isTilingAReq = (TP_ADD_TILING_A == 1) ? 1 : 0;
    static constexpr int isTilingBReq = (TP_ADD_TILING_B == 1) ? 1 : 0;
    static constexpr int isTilingOutReq = (TP_ADD_DETILING_OUT == 1) ? 1 : 0;
    // Tiling kernels are not supported in on devices that use 512-bit vector read/writes.
    static constexpr int isTilingSupported = (__ALIGN_BYTE_SIZE__ == 64) ? 0 : 1;

    // static_assert(isTilingSupported == 1 || isTilingReq == 0, "Tiling is not supported on this device.");
    static_assert(isTilingSupported == 1 || isTilingAReq == 0, "Tiling (TP_ADD_TILING_A == 1) is not supported.");
    static_assert(isTilingSupported == 1 || isTilingBReq == 0, "Tiling (TP_ADD_TILING_B == 1) is not supported.");
    static_assert(isTilingSupported == 1 || isTilingOutReq == 0, "Tiling (TP_ADD_DETILING_OUT == 1) is not supported.");

    static_assert(TP_DIM_AB % TP_CASC_LEN == 0, "TP_DIM_AB needs to be a multiple of TP_CASC_LEN");
    template <bool cascIn, bool cascOut>
    using matMultCasc = matrix_mult<TT_DATA_A,
                                    TT_DATA_B,
                                    TT_OUT_DATA,
                                    (TP_DIM_A / TP_SSR),
                                    (TP_DIM_AB / TP_CASC_LEN),
                                    TP_DIM_B,
                                    TP_SHIFT,
                                    TP_RND,
                                    TP_SAT,
                                    TP_DIM_A_LEADING,
                                    TP_DIM_B_LEADING,
                                    TP_DIM_OUT_LEADING,
                                    (TP_INPUT_WINDOW_VSIZE_A / (TP_SSR * TP_CASC_LEN)),
                                    (TP_INPUT_WINDOW_VSIZE_B / TP_CASC_LEN),
                                    cascIn,
                                    cascOut>;
    // avoid redundant instances of unused templates -- Fixes x86sim linker error without resorting to recursive
    // template meta programming
    using onlyMatMult = typename std::conditional<(TP_CASC_LEN == 1), matMultCasc<false, false>, no_kernel>::type;
    using firstMatMult = typename std::conditional<(TP_CASC_LEN > 1), matMultCasc<false, true>, onlyMatMult>::type;
    using lastMatMult = typename std::conditional<(TP_CASC_LEN > 1), matMultCasc<true, false>, firstMatMult>::type;
    using middleMatMult = typename std::conditional<(TP_CASC_LEN > 2), matMultCasc<true, true>, lastMatMult>::type;
    // AIE_API tiling scheme in use - single configuration for each data type.
    // Tiling scheme doesn't change vs cascade or not.
    static constexpr tilingStruct tilingScheme = middleMatMult::getTilingScheme();

    // Forward compatible for batch window processing.
    static constexpr unsigned int dimAPerKernel = (TP_INPUT_WINDOW_VSIZE_A / TP_DIM_AB);
    static constexpr unsigned int dimBPerKernel = (TP_INPUT_WINDOW_VSIZE_B / TP_DIM_AB);

    using TilerClassA = tilerKernelClass<tilingScheme.Atile,
                                         tilingScheme.ABtile,
                                         (dimAPerKernel / TP_SSR),
                                         (TP_DIM_AB / TP_CASC_LEN),
                                         TP_DIM_A_LEADING,
                                         TT_DATA_A>;
    using TilerClassB = tilerKernelClass<tilingScheme.ABtile,
                                         tilingScheme.Btile,
                                         (TP_DIM_AB / TP_CASC_LEN),
                                         dimBPerKernel,
                                         TP_DIM_B_LEADING,
                                         TT_DATA_B>;
    using DetilerClassOut = untilerKernelClass<tilingScheme.Atile,
                                               tilingScheme.Btile,
                                               (dimAPerKernel / TP_SSR),
                                               dimBPerKernel,
                                               TP_DIM_OUT_LEADING,
                                               TT_OUT_DATA>;

    static constexpr bool isRedundantTilerA =
        (((TP_DIM_AB / TP_CASC_LEN) <= tilingScheme.ABtile) && (TP_DIM_A_LEADING == ROW_MAJOR));
    static constexpr bool isRedundantTilerB =
        ((dimBPerKernel <= tilingScheme.Btile) && (TP_DIM_B_LEADING == ROW_MAJOR));
    static constexpr bool isRedundantTilerOut =
        ((dimBPerKernel <= tilingScheme.Btile) && (TP_DIM_OUT_LEADING == ROW_MAJOR));

    /**
     * @endcond
     */
    /**
     * @brief This is the constructor function for the Matrix Multiply graph.
     **/
    matrix_mult_graph() {
        if (isRedundantTilerA && TP_ADD_TILING_A == 1) {
            printf(
                "WARNING: TP_ADD_TILING_A is true, but P_DIM_AB is small enough that tiling is not necessary for this "
                "configuration. TP_ADD_TILING_A will be ignored. \n");
        }
        if (isRedundantTilerB && TP_ADD_TILING_B == 1) {
            printf(
                "WARNING: TP_ADD_TILING_B is true, but P_DIM_B is small enough that tiling is not necessary for this "
                "configuration. TP_ADD_TILING_B will be ignored. \n");
        }
        if (isRedundantTilerOut && TP_ADD_DETILING_OUT == 1) {
            printf(
                "WARNING: TP_ADD_DETILING_OUT is true, but P_DIM_B is small enough that detiling is not necessary for "
                "this configuration. TP_ADD_DETILING_OUT will be ignored. \n");
        }
        printf("\n");

        for (int ssrRank = 0; ssrRank < TP_SSR; ssrRank++) {
            // make input connections
            for (int cascRank = 0; cascRank < TP_CASC_LEN; cascRank++) {
                int kernelNum = (ssrRank * TP_CASC_LEN) + cascRank;
                if (cascRank >= 1 && cascRank < (TP_CASC_LEN - 1)) {
                    // both cascade
                    m_MatmultKernels[kernelNum] = kernel::create_object<middleMatMult>();
                } else if (cascRank >= 1 && cascRank >= (TP_CASC_LEN - 1)) {
                    // last kernel
                    m_MatmultKernels[kernelNum] = kernel::create_object<lastMatMult>();
                } else {
                    // first kernel
                    m_MatmultKernels[kernelNum] = kernel::create_object<firstMatMult>();
                }
                if (cascRank >= 1) {
                    connect<cascade>(m_MatmultKernels[kernelNum - 1].out[0], m_MatmultKernels[kernelNum].in[2]);
                }
                // TODO, different window sizes for end kernel if the window size doesn't evenly split by CASC_LEN.

                if
                    constexpr(!isRedundantTilerA && TP_ADD_TILING_A) {
                        tilerA[kernelNum] = kernel::create_object<TilerClassA>();
                        connect<>(inA[kernelNum], tilerA[kernelNum].in[0]);
                        dimensions(tilerA[kernelNum].in[0]) = {TP_INPUT_WINDOW_VSIZE_A / (TP_SSR * TP_CASC_LEN)};

                        connect<>(tilerA[kernelNum].out[0], m_MatmultKernels[kernelNum].in[0]);
                        dimensions(tilerA[kernelNum].out[0]) = {TP_INPUT_WINDOW_VSIZE_A / (TP_SSR * TP_CASC_LEN)};
                        dimensions(m_MatmultKernels[kernelNum].in[0]) = {TP_INPUT_WINDOW_VSIZE_A /
                                                                         (TP_SSR * TP_CASC_LEN)};
                    }
                else {
                    connect<>(inA[kernelNum], m_MatmultKernels[kernelNum].in[0]);
                    dimensions(m_MatmultKernels[kernelNum].in[0]) = {TP_INPUT_WINDOW_VSIZE_A / (TP_SSR * TP_CASC_LEN)};
                }

                if
                    constexpr(!isRedundantTilerB && TP_ADD_TILING_B) {
                        tilerB[kernelNum] = kernel::create_object<TilerClassB>();
                        connect<>(inB[kernelNum], tilerB[kernelNum].in[0]);
                        dimensions(tilerB[kernelNum].in[0]) = {TP_INPUT_WINDOW_VSIZE_B / TP_CASC_LEN};
                        connect<>(tilerB[kernelNum].out[0], m_MatmultKernels[kernelNum].in[1]);
                        dimensions(tilerB[kernelNum].out[0]) = {TP_INPUT_WINDOW_VSIZE_B / TP_CASC_LEN};
                        dimensions(m_MatmultKernels[kernelNum].in[1]) = {TP_INPUT_WINDOW_VSIZE_B / TP_CASC_LEN};
                    }
                else {
                    connect<>(inB[kernelNum], m_MatmultKernels[kernelNum].in[1]);
                    dimensions(m_MatmultKernels[kernelNum].in[1]) = {TP_INPUT_WINDOW_VSIZE_B / TP_CASC_LEN};
                }
                // Specify mapping constraints - Can be overriden in parent graph.
                runtime<ratio>(m_MatmultKernels[kernelNum]) = 0.8;
                runtime<ratio>(tilerA[kernelNum]) = 0.4;
                runtime<ratio>(tilerB[kernelNum]) = 0.4;
                // Source files
                source(m_MatmultKernels[kernelNum]) = "matrix_mult.cpp";
                source(tilerA[kernelNum]) = "matrix_mult_tiler.cpp";
                source(tilerB[kernelNum]) = "matrix_mult_tiler.cpp";
            }
            int kernelOutNum = (ssrRank * TP_CASC_LEN) + TP_CASC_LEN - 1;
            if
                constexpr(!isRedundantTilerOut && TP_ADD_DETILING_OUT) {
                    untiler[ssrRank] = kernel::create_object<DetilerClassOut>();
                    connect<>(m_MatmultKernels[kernelOutNum].out[0], untiler[ssrRank].in[0]);
                    dimensions(m_MatmultKernels[kernelOutNum].out[0]) = {(dimAPerKernel / TP_SSR) * dimBPerKernel};
                    dimensions(untiler[ssrRank].in[0]) = {(dimAPerKernel / TP_SSR) * dimBPerKernel};
                    connect<>(untiler[ssrRank].out[0], out[ssrRank]);
                    dimensions(untiler[ssrRank].out[0]) = {(dimAPerKernel / TP_SSR) * dimBPerKernel};
                }
            else {
                connect<>(m_MatmultKernels[kernelOutNum].out[0], out[ssrRank]);
                dimensions(m_MatmultKernels[kernelOutNum].out[0]) = {(dimAPerKernel / TP_SSR) * dimBPerKernel};
            }
            runtime<ratio>(untiler[ssrRank]) = 0.4;
            source(untiler[ssrRank]) = "matrix_mult_untiler.cpp";
        }
    }
};
}
}
}
}
}

#endif //_DSPLIB_MATRIX_MULT_GRAPH_HPP_
