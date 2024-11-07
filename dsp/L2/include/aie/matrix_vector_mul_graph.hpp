/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_MATRIX_VECTOR_MUL_GRAPH_HPP_
#define _DSPLIB_MATRIX_VECTOR_MUL_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the matrix_vector_mul function library element.
*/

/**
 * @file matrix_vector_mul_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"
#include "matrix_vector_mul.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {
/**
 * @defgroup matrix_vector_mul_graph General Matrix-Vector Multiply (GEMV)
 *
 * Matrix-Vector Multiply/GEMV (General Matrix-Vector Multiply) solution.
 *
 */
//--------------------------------------------------------------------------------------------------
// matrix_vector_mul_graph template
//--------------------------------------------------------------------------------------------------

/**
  * @cond NOCOMMENTS
  */

// Start of recursive kernel creation for cascaded matrix_vector_mul kernels
// This is the specialization for kernels in the middle of the cascade.
template <int kPos,
          typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN * TP_SSR]) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;

        for (int ssr = 0; ssr < TP_SSR; ssr++) {
            mat_vec_mulKernels[kPos - 1 + ssr * TP_CASC_LEN] = kernel::create_object<
                matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A / TP_SSR, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                  TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, true, true> >();
        }
        create_casc_kernel_recur<kPos - 1, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                 TP_NUM_FRAMES, TP_CASC_LEN, TP_SSR>::create(mat_vec_mulKernels);
    }
};

// Recursive fft kernel creation
// This is the specialization for the end of recursion (first kernel in cascade)
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR>
class create_casc_kernel_recur<1,
                               TT_DATA_A,
                               TT_DATA_B,
                               TP_DIM_A,
                               TP_DIM_B,
                               TP_SHIFT,
                               TP_RND,
                               TP_SAT,
                               TP_NUM_FRAMES,
                               TP_CASC_LEN,
                               TP_SSR> {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN * TP_SSR]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;

        for (int ssr = 0; ssr < TP_SSR; ssr++) {
            mat_vec_mulKernels[0 + (ssr * TP_CASC_LEN)] = kernel::create_object<
                matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A / TP_SSR, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                  TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, false, true> >();
        }
    }
};

// matrix_vector_mul Kernel creation, entry to recursion, also end of cascade.
template <int kPos,
          typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR>
class create_casc_kernel {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN * TP_SSR]) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        for (int ssr = 0; ssr < TP_SSR; ssr++) {
            mat_vec_mulKernels[kPos - 1 + (TP_CASC_LEN * ssr)] = kernel::create_object<
                matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A / TP_SSR, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                  TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, true, false> >();
        }

        create_casc_kernel_recur<kPos - 1, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                 TP_NUM_FRAMES, TP_CASC_LEN, TP_SSR>::create(mat_vec_mulKernels);
    }
};

// matrix_vector_mul Kernel creation, Specialization for CASC_LEN=1
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR>
class create_casc_kernel<1,
                         TT_DATA_A,
                         TT_DATA_B,
                         TP_DIM_A,
                         TP_DIM_B,
                         TP_SHIFT,
                         TP_RND,
                         TP_SAT,
                         TP_NUM_FRAMES,
                         TP_CASC_LEN,
                         TP_SSR> {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN * TP_SSR]) {
        for (int ssr = 0; ssr < TP_SSR; ssr++) {
            static constexpr unsigned int TP_KERNEL_POSITION = 0;
            mat_vec_mulKernels[ssr] = kernel::create_object<
                matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A / TP_SSR, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                  TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, false, false> >();
        }
    }
};

/**
  * @endcond
  */

/**
 * @ingroup matrix_vector_mul_graph
 * @brief matrix_vector_mul performs the General Matrix Vector Multiplier (GEMV) which multiplies a matrix input
 *        with a vector input of configurable data types and dimensions.
 *
 * These are the templates to configure the matrix vector multiplier:
 * @tparam TT_DATA_A describes the data type of the input samples of Matrix A. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_DATA_B describes the data type of the input samples of Vector B. \n
 *         \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TP_DIM_A is an unsigned integer which describes the number of elements
 *         along the unique dimension (rows) of Matrix A.
 * @tparam TP_DIM_B is an unsigned integer which describes the number of elements
 *          in Vector B and the number of columns in Matrix A.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         FIR terms before output. \n ``TP_SHIFT`` must be in the range 0 to 59 (61 for AIE1).
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
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML device. \n
 * @tparam TP_NUM_FRAMES describes the number of batches of input data that will be processed per iteration. \n
 * @tparam TP_CASC_LEN describes the number of AIE kernels the matrix-vector multiplication will be divided into in
 *series. \n
 *         Each kernel will receive a an equal sized split (along the common dimension) of the matrix and vector,
 *         and will pass the partial computation of the output to the next kernel in the chain via the cascade stream.
 *         TP_CASC_LEN must be in the range 1 (default) to 16.
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 * @tparam TP_SSR describes the number of kernels (or cascaded kernel chains) that will compute the matrix-vector
 *         multiplication in parallel.
 *         Each SSR rank will receive an equal sized split (along the unique dimension) of Matrix A data. \n
 *         There is no splitting of the vector data when TP_SSR > 1 (only split when TP_CASC_LEN > 1).
 *         The Vector B inputs across a
 *         chain of cascaded kernels will be the same across all SSR ranks.
 * @tparam TP_DIM_A_LEADING describes the leading dimension of the Matrix A data.
 *         If TP_DIM_A_LEADING=1, the columns of the matrix are contiguous in memory. This is the only supported order
 *         of Matrix A input data when doing the computation. \n
 *         However, if TP_DIM_A_LEADING=0, the rows of the matrix input are contiguous in memory and will be transposed
 *         at the input ports for each kernel using DMA Buffer Descriptors.
 *         This feature is currently only supported when TT_DATA_A is cint16, int32 or float, and NUM_FRAMES=1. \n
 *         If TT_DATA_A is int16, cint32 or cfloat or NUM_FRAMES > 1, the input matrix data must be transposed outwith
 *         the graph port connection to a column major order, and TP_DIM_A_LEADING must be set to 1. \n
 **/
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SAT,
          unsigned int TP_SSR,
          unsigned int TP_DIM_A_LEADING>
class matrix_vector_mul_graph : public graph {
   public:
    /**
     * The kernels that will be created and mapped onto AIE tiles.
     * The size of C will determine the length of the number of kernels connected with each other in series via a
     *cascade interface.
     * There will be (``TP_SSR``) number cascaded kernel chains computed in parallel. Therefore, there will be
     *(``TP_CASC_LEN``) * (``TP_SSR``)  total kernels.
     **/
    kernel m_mat_vec_mulKernels[TP_CASC_LEN * TP_SSR];

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return m_mat_vec_mulKernels; };

    /**
     * Input to the function, Matrix A. This should stored in a column major format (TP_DIM_A_LEADING_A = 1) where each
     *column of data is
     * stored contiguously in memory. Row major format (TP_DIM_A_LEADING = 0) will be transposed using DMA buffer
     *descriptors.
     * However, this is only supported when TT_DATA_A is cint16, int32, or float and NUM_FRAMES = 1. Configurations with
     *other TT_DATA_A types and NUM_FRAMES > 1,
     * are not supported by the DMA transpose feature and must be input in column major format (TP_DIM_A_LEADING=1). \n
     * The dimensions of the matrix are specified by template parameters TP_DIM_A (number of rows),
     * and TP_DIM_B (number of columns). \n
     * TP_DIM_A must be a multiple of (256 / 8 / sizeof(TT_DATA_A)) * TP_SSR, and TP_DIM_B must be a multiple
     * of (256 / 8 / sizeof(TT_DATA_B)) * TP_CASC_LEN. \n
     * The matrix data can be zero-padded to achieve this requirement. \n
     * The number of samples to each Matrix A iobuffer will be (TP_DIM_A / TP_SSR) * (TP_DIM_B / TP_CASC_LEN) *
     *TP_NUM_FRAMES.
     **/
    port<input> inA[TP_CASC_LEN * TP_SSR];

    /**
     * Input to the function, Vector B.
     * The dimensions of the vector are specified by template
     * parameter TP_DIM_B (equal to number of columns in Matrix A). \n
     * TP_DIM_B must be a multiple of (256 / 8 / sizeof(TT_DATA_B)) * TP_CASC_LEN. \n
     * The vector data can be zero-padded to achieve this requirement. \n
     * The number of samples to the Vector B iobuffer will be (TP_DIM_B / TP_CASC_LEN) * TP_NUM_FRAMES.
     **/
    port<input> inB[TP_CASC_LEN * TP_SSR];

    /**
     * The output data of the function. For cascaded designs, this is located at the end of the cascaded kernel chain.
     *The number of output ports will be equal to the number of SSR ranks (``TP_SSR``). \n
     * The output type will depend on the type of the matrix and vector (TT_DATA_A and TT_DATA_B).
     * The vector result of the matrix-vector multiplication will be the size of TP_DIM_A.
     * The number of samples to the Output iobuffer will be (TP_DIM_A / TP_CASC_LEN) * TP_NUM_FRAMES.
     *
     **/
    port<output> out[TP_SSR];

    /**
     * @brief This is the constructor function for the Matrix Vector Multiply graph.
     * Constructor has no arguments.
     **/
    matrix_vector_mul_graph() {
        static_assert(TP_DIM_A_LEADING == 1 || !std::is_same<TT_DATA_A, int16>::value,
                      "ERROR: TP_DIM_A_LEADING = 1 (Row major data) is not supported when TT_DATA_A is int16.");
        static_assert(TP_DIM_A_LEADING == 1 || !std::is_same<TT_DATA_A, cint32>::value,
                      "ERROR: TP_DIM_A_LEADING = 1 (Row major data) is not supported when TT_DATA_A is cint32.");
        static_assert(TP_DIM_A_LEADING == 1 || !std::is_same<TT_DATA_A, cfloat>::value,
                      "ERROR: TP_DIM_A_LEADING = 1 (Row major data) is not supported when TT_DATA_A is cfloat.");
        static_assert(
            TP_DIM_A_LEADING == 1 || TP_NUM_FRAMES == 1,
            "ERROR: TP_DIM_A_LEADING = 1 (Row major data) is not supported for batch processing (TP_NUM_FRAMES > 1)");
        create_casc_kernel<TP_CASC_LEN, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                           TP_NUM_FRAMES, TP_CASC_LEN, TP_SSR>::create(m_mat_vec_mulKernels);

        constexpr int dimAPerKernel = TP_DIM_A / TP_SSR;
        constexpr int dimBPerKernel = TP_DIM_B / TP_CASC_LEN;

        constexpr int windowSizeA = TP_NUM_FRAMES * dimAPerKernel * dimBPerKernel;
        constexpr int windowSizeB = TP_NUM_FRAMES * dimBPerKernel;
        constexpr int windowSizeOut = TP_NUM_FRAMES * dimAPerKernel;

        for (int ssrRank = 0; ssrRank < TP_SSR; ssrRank++) {
            // Create kernel classes
            for (int cascNum = 0; cascNum < TP_CASC_LEN; cascNum++) {
                // connect cascaded kernels
                if (cascNum >= 1 && TP_CASC_LEN > 1) {
                    connect<cascade>(m_mat_vec_mulKernels[(cascNum - 1) + (ssrRank * TP_CASC_LEN)].out[0],
                                     m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)].in[2]);
                }
                // connect matrix input data to each kernel (DMA buffer port transpose if Matrix is in row major format)
                connect(inA[cascNum + (ssrRank * TP_CASC_LEN)],
                        m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)].in[0]);
                dimensions(m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)].in[0]) = {windowSizeA};
                // Row major data will be transposed using DMA buffer descriptors
                if
                    constexpr(!TP_DIM_A_LEADING) {
                        write_access(m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)].in[0]) =
                            adf::tiling({.buffer_dimension = {(dimAPerKernel), (dimBPerKernel)},
                                         .tiling_dimension = {1, dimBPerKernel},
                                         .offset = {0, 0},
                                         .tile_traversal = {{.dimension = 0, .stride = 1, .wrap = dimAPerKernel},
                                                            {.dimension = 1, .stride = dimBPerKernel, .wrap = 1}}});
                    }
                // connect vector input data to each kernel (no transpose needed)
                connect(inB[cascNum + (ssrRank * TP_CASC_LEN)],
                        m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)].in[1]);
                dimensions(m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)].in[1]) = {windowSizeB};

                // Specify mapping constraints
                runtime<ratio>(m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)]) = 0.8;
                // Source files
                source(m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)]) = "matrix_vector_mul.cpp";
                headers(m_mat_vec_mulKernels[cascNum + (ssrRank * TP_CASC_LEN)]) = {"matrix_vector_mul.hpp"};
            }

            // connect final kernel output to output of the graph
            connect(m_mat_vec_mulKernels[(TP_CASC_LEN - 1) + (ssrRank * TP_CASC_LEN)].out[0], out[ssrRank]);
            dimensions(m_mat_vec_mulKernels[(TP_CASC_LEN - 1) + (ssrRank * TP_CASC_LEN)].out[0]) = {windowSizeOut};
        }
    };
};

} // namespace matrix_vector_mul
} // namespace blas
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_MATRIX_VECTOR_MUL_GRAPH_HPP_
