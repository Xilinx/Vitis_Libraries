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
#ifndef _DSPLIB_OUTER_TENSOR_REF_GRAPH_HPP_
#define _DSPLIB_OUTER_TENSOR_REF_GRAPH_HPP_

/*
This file holds the definition of the Outer Tensor Reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "outer_tensor_traits.hpp" //needed for output window size calc
#include "outer_tensor_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace outer_tensor {
using namespace adf;

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class outer_tensor_ref_graph : public graph {
   public:
    static constexpr int kDimA = TP_DIM_A / TP_SSR;
    static constexpr int kDimB = TP_DIM_B;
    static constexpr int kvecSampleNumOut = 256 / 8 / vectByte<TT_DATA_A, TT_DATA_B>().val_byteOut;
    static constexpr int kWindowVsize = TP_DIM_A * CEIL(TP_DIM_B, kvecSampleNumOut) / TP_SSR;

    port<input> inA[TP_SSR];
    port<input> inB[TP_SSR];
    port<output> out[TP_SSR];

    kernel m_kernels[TP_SSR];

    // Constructor
    outer_tensor_ref_graph() {
        printf("============================\n");
        printf("== OUTER_TENSOR REF Graph\n");
        printf("============================\n");
        printf("TP_DIM_A                = %d\n", TP_DIM_A);
        printf("TP_DIM_B                = %d\n", TP_DIM_B);
        printf("TP_NUM_FRAMES           = %d\n", TP_NUM_FRAMES);
        printf("TP_SHIFT                = %d\n", TP_SHIFT);
        printf("TP_API                  = %d\n", TP_API);
        printf("TP_SSR                  = %d\n", TP_SSR);
        printf("TP_RND                  = %d\n", TP_RND);
        printf("TP_SAT                  = %d\n", TP_SAT);

        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] = kernel::create_object<outer_tensor_ref<TT_DATA_A, TT_DATA_B, kDimA, kDimB, TP_NUM_FRAMES,
                                                                  TP_SHIFT, TP_API, TP_SSR, TP_RND, TP_SAT> >();

            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.9; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "outer_tensor_ref.cpp";

            // make connections
            connect(inA[i], m_kernels[i].in[0]);
            connect(inB[i], m_kernels[i].in[1]);
            dimensions(m_kernels[i].in[0]) = {kDimA * TP_NUM_FRAMES};
            dimensions(m_kernels[i].in[1]) = {kDimB * TP_NUM_FRAMES};
            connect(m_kernels[i].out[0], out[i]);
            dimensions(m_kernels[i].out[0]) = {kWindowVsize * TP_NUM_FRAMES};
        }
    };
};
}
}
}
}
#endif // _DSPLIB_OUTER_TENSOR_REF_GRAPH_HPP_
