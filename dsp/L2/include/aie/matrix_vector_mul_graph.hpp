/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
 * @file matrix_vector_graph.hpp
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
          unsigned int TP_CASC_LEN>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        mat_vec_mulKernels[kPos - 1] =
            kernel::create_object<matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                                    TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, true, true> >();

        create_casc_kernel_recur<kPos - 1, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                 TP_NUM_FRAMES, TP_CASC_LEN>::create(mat_vec_mulKernels);
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
          unsigned int TP_CASC_LEN>
class create_casc_kernel_recur<1,
                               TT_DATA_A,
                               TT_DATA_B,
                               TP_DIM_A,
                               TP_DIM_B,
                               TP_SHIFT,
                               TP_RND,
                               TP_SAT,
                               TP_NUM_FRAMES,
                               TP_CASC_LEN> {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        mat_vec_mulKernels[0] =
            kernel::create_object<matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                                    TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, false, true> >();
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
          unsigned int TP_CASC_LEN>
class create_casc_kernel {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        mat_vec_mulKernels[kPos - 1] =
            kernel::create_object<matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                                    TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, true, false> >();

        create_casc_kernel_recur<kPos - 1, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                 TP_NUM_FRAMES, TP_CASC_LEN>::create(mat_vec_mulKernels);
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
          unsigned int TP_CASC_LEN>
class create_casc_kernel<1,
                         TT_DATA_A,
                         TT_DATA_B,
                         TP_DIM_A,
                         TP_DIM_B,
                         TP_SHIFT,
                         TP_RND,
                         TP_SAT,
                         TP_NUM_FRAMES,
                         TP_CASC_LEN> {
   public:
    static void create(kernel (&mat_vec_mulKernels)[1]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        mat_vec_mulKernels[0] =
            kernel::create_object<matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                                                    TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, false, false> >();
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
 * @tparam TT_DATA_A describes the data type of the input samples of Matrix A.
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_DATA_B describes the data type of the input samples of Vector B.
 *         This is a typename and must be one
 *         of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TP_DIM_A is an unsigned integer which describes the number of elements
 *         along the unique dimension (rows) of Matrix A.
 * @tparam TP_DIM_B is an unsigned integer which describes the number of elements
 *          in Vector B and the number of columns in Matrix A.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         FIR terms before output. \n TP_SHIFT must be in the range 0 to 61.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. Although, TP_RND accepts unsigned integer values
 *         descriptive macros are recommended where
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
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML device. \n
 * @tparam TP_NUM_FRAMES describes the number of batches of input data that will be processed per iteration. \n
 * @tparam TP_CASC_LEN describes the number of AIE processors to split the operation
 *         over.  \n This allows resource to be traded for higher performance.
 *         TP_CASC_LEN must be in the range 1 (default) to 40.
 * @tparam TP_SAT describes the selection of saturation to be applied during the
 *         shift down stage of processing. TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 **/
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SAT>
class matrix_vector_mul_graph : public graph {
   public:
    /**
     * The chain of kernels that will be created and mapped onto AIE tiles.
     * Number of kernels (``TP_CASC_LEN``) will be connected with each other in series via a cascade interface.
     **/
    kernel m_mat_vec_mulKernels[TP_CASC_LEN];

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return m_mat_vec_mulKernels; };

    /**
     * Input to the function, Matrix A. This must be stored in a column major format (each column of data is
     * stored contigiously in memory). The dimensions of the matrix are specified by template parameters TP_DIM_A
     *(number of rows),
     * and TP_DIM_B (number of columns). \n
     * TP_DIM_A must be a multiple of 256 / 8 / sizeof(TT_DATA_A), and TP_DIM_B must be a multiple
     * of 256 / 8 / sizeof(TT_DATA_B). \n
     * The matrix can be zero-padded to achieve this requirement. \n
     * The number of samples to the Matrix A iobuffer will be TP_DIM_A * TP_DIM_B * TP_NUM_FRAMES.
     **/
    port<input> inA[TP_CASC_LEN];

    /**
     * Input to the function, Vector B.
     * The dimensions of the vector are specified by template
     * parameter TP_DIM_B (equal to number of columns in Matrix A). \n
     * TP_DIM_B must be a multiple of 256 / 8 / sizeof(TT_DATA_B). \n
     * The vector can be zero-padded to achieve this requirement. \n
     * The number of samples to the Vector B iobuffer will be TP_DIM_B * TP_NUM_FRAMES.
     **/
    port<input> inB[TP_CASC_LEN];

    /**
     * The output data of the function. For cascaded designs, this is located at the end of the cascaded kernel chain.
     * The output type will depend on the type of the matrix and vector (TT_DATA_A and TT_DATA_B).
     * The vector result of the matrix-vector multiplication will be the size of TP_DIM_A.
     * The number of samples to the Output iobuffer will be TP_DIM_A * TP_NUM_FRAMES.
     *
     **/
    port<output> out[1];

    /**
     * @brief This is the constructor function for the Matrix Vector Multiply graph.
     * Constructor has no arguments.
     **/
    matrix_vector_mul_graph() {
        // Create kernel classes
        create_casc_kernel<TP_CASC_LEN, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_SAT,
                           TP_NUM_FRAMES, TP_CASC_LEN>::create(m_mat_vec_mulKernels);

        constexpr int windowSizeA = TP_NUM_FRAMES * TP_DIM_A * (TP_DIM_B / TP_CASC_LEN);
        constexpr int windowSizeB = TP_NUM_FRAMES * (TP_DIM_B / TP_CASC_LEN);
        constexpr int windowSizeOut = TP_NUM_FRAMES * TP_DIM_A;

        // create object for each kernel in cascade
        for (int cascNum = 0; cascNum < TP_CASC_LEN; cascNum++) {
            // connect cascaded kernels
            if (cascNum >= 1 && TP_CASC_LEN > 1) {
                connect<cascade>(m_mat_vec_mulKernels[cascNum - 1].out[0], m_mat_vec_mulKernels[cascNum].in[2]);
            }
            // // connect input data to each kernel
            connect(inA[cascNum], m_mat_vec_mulKernels[cascNum].in[0]);
            connect(inB[cascNum], m_mat_vec_mulKernels[cascNum].in[1]);
            dimensions(m_mat_vec_mulKernels[cascNum].in[0]) = {windowSizeA};
            dimensions(m_mat_vec_mulKernels[cascNum].in[1]) = {windowSizeB};

            // Specify mapping constraints
            runtime<ratio>(m_mat_vec_mulKernels[cascNum]) = 0.8;
            // Source files
            source(m_mat_vec_mulKernels[cascNum]) = "matrix_vector_mul.cpp";
            headers(m_mat_vec_mulKernels[cascNum]) = {"matrix_vector_mul.hpp"};
        }

        // connect final kernel output to output of the graph
        connect(m_mat_vec_mulKernels[(TP_CASC_LEN - 1)].out[0], out[0]);
        dimensions(m_mat_vec_mulKernels[(TP_CASC_LEN - 1)].out[0]) = {windowSizeOut};
    };
};

} // namespace matrix_vector_mul
} // namespace blas
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_MATRIX_VECTOR_MUL_GRAPH_HPP_
