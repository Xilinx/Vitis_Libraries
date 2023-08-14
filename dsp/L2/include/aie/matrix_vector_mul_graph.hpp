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
the MATRIX_VECTOR_MUL function library element.
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
namespace matrix_vector_mul {

// Start of recursive kernel creation for cascaded matrix_vector_mul kernels
// This is the specialization for kernels in the middle of the cascade.
template <int kPos,
          typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
class create_casc_kernel_recur {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        mat_vec_mulKernels[kPos - 1] =
            kernel::create_object<matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND,
                                                    TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, true, true> >();

        create_casc_kernel_recur<kPos - 1, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_NUM_FRAMES,
                                 TP_CASC_LEN>::create(mat_vec_mulKernels);
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
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
class create_casc_kernel_recur<1,
                               TT_DATA_A,
                               TT_DATA_B,
                               TP_DIM_A,
                               TP_DIM_B,
                               TP_SHIFT,
                               TP_RND,
                               TP_NUM_FRAMES,
                               TP_CASC_LEN> {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        mat_vec_mulKernels[0] =
            kernel::create_object<matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND,
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
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
class create_casc_kernel {
   public:
    static void create(kernel (&mat_vec_mulKernels)[TP_CASC_LEN]) {
        static constexpr unsigned int TP_KERNEL_POSITION = kPos - 1;
        mat_vec_mulKernels[kPos - 1] =
            kernel::create_object<matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND,
                                                    TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, true, false> >();

        create_casc_kernel_recur<kPos - 1, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_NUM_FRAMES,
                                 TP_CASC_LEN>::create(mat_vec_mulKernels);
    }
};

// matrix_vector_mul Kernel creation, Specialization for CASC_LEN=1
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
class create_casc_kernel<1, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_NUM_FRAMES, TP_CASC_LEN> {
   public:
    static void create(kernel (&mat_vec_mulKernels)[1]) {
        static constexpr unsigned int TP_KERNEL_POSITION = 0;
        mat_vec_mulKernels[0] =
            kernel::create_object<matrix_vector_mul<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND,
                                                    TP_NUM_FRAMES, TP_CASC_LEN, TP_KERNEL_POSITION, false, false> >();
    }
};

//--------------------------------------------------------------------------------------------------
// matrix_vector_mul_graph template
//--------------------------------------------------------------------------------------------------

// TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_NUM_FRAMES, TP_CASC_LEN
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
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

    port<input> inA[TP_CASC_LEN];
    port<input> inB[TP_CASC_LEN];

    port<output> out[1];

    matrix_vector_mul_graph() {
        // Create kernel classes
        create_casc_kernel<TP_CASC_LEN, TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_SHIFT, TP_RND, TP_NUM_FRAMES,
                           TP_CASC_LEN>::create(m_mat_vec_mulKernels);

        constexpr int windowSizeA = TP_NUM_FRAMES * TP_DIM_A * TP_DIM_B;
        constexpr int windowSizeB = TP_NUM_FRAMES * TP_DIM_B;
        constexpr int windowSizeOut = TP_NUM_FRAMES * TP_DIM_A;

        // create object for each kernel in cascade
        for (int cascNum = 0; cascNum < TP_CASC_LEN; cascNum++) {
            // connect cascaded kernels
            if (cascNum >= 1 && TP_CASC_LEN > 1) {
                connect<cascade>(m_mat_vec_mulKernels[cascNum - 1].out[0], m_mat_vec_mulKernels[cascNum].in[1]);
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
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_MATRIX_VECTOR_MUL_GRAPH_HPP_
