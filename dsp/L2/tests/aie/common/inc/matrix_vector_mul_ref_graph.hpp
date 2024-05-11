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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_REF_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_REF_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include "matrix_vector_mul_ref.hpp"
#include "fir_ref_utils.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

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
class matrix_vector_mul_ref_graph : public graph {
   public:
    // std::array<port<input>, 1> inA[1];
    // std::array<port<output>, 1> inB[1];
    // std::array<port<output>, 1> out;
    port<input> inA[1];
    port<input> inB[1];
    port<output> out[1];

    // FIR Kernel
    kernel m_matrix_vector_mulKernel;

    // Constructor
    matrix_vector_mul_ref_graph() {
        // constexpr int windowSizeA = TP_NUM_FRAMES*TP_DIM_A*TP_DIM_B;
        // constexpr int windowSizeB = TP_NUM_FRAMES*TP_DIM_B;
        // constexpr int windowSizeOut = TP_NUM_FRAMES*TP_DIM_A;

        // Create MATRIX_VECTOR_MUL class
        printf("\nMatrix Vector Multiply Ref\n");
        m_matrix_vector_mulKernel =
            kernel::create_object<matrix_vector_mul_ref<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_DIM_A_LEADING,
                                                        TP_SHIFT, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_CASC_LEN> >();

        connect<>(inA[0], m_matrix_vector_mulKernel.in[0]);
        dimensions(m_matrix_vector_mulKernel.in[0]) = {TP_NUM_FRAMES * TP_DIM_A * TP_DIM_B};
        connect<>(inB[0], m_matrix_vector_mulKernel.in[1]);
        dimensions(m_matrix_vector_mulKernel.in[1]) = {TP_NUM_FRAMES * TP_DIM_B};

        connect<>(m_matrix_vector_mulKernel.out[0], out[0]);
        dimensions(m_matrix_vector_mulKernel.out[0]) = {TP_NUM_FRAMES * TP_DIM_A};

        runtime<ratio>(m_matrix_vector_mulKernel) = 0.8;

        // Source files
        source(m_matrix_vector_mulKernel) = "matrix_vector_mul_ref.cpp";
        headers(m_matrix_vector_mulKernel) = {"matrix_vector_mul_ref.hpp"};
        printf("== Graph window specialization exit\n");
    };
};
}
}
}
}
}
#endif // _DSPLIB_matrix_vector_mul_REF_GRAPH_HPP_
