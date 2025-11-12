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
#ifndef _DSPLIB_CUMSUM_REF_GRAPH_HPP_
#define _DSPLIB_CUMSUM_REF_GRAPH_HPP_

/*
This file holds the definition of the Cumsum Reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "cumsum_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace cumsum {
using namespace adf;

template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_MODE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class cumsum_ref_graph : public graph {
   public:
    static constexpr int TP_SSR = 1;
    static constexpr int kMemWidth = TP_MODE == 2 ? __MAX_READ_WRITE__ * 2 : __MAX_READ_WRITE__;
    static constexpr int kVectLen = kMemWidth / 8 / sizeof(TT_OUT_DATA);
    static constexpr int kWindowVsize = CEIL(TP_DIM_A, kVectLen) * TP_DIM_B * TP_NUM_FRAMES;

    port<input> in[TP_SSR];
    port<output> out[TP_SSR];

    kernel m_kernels[TP_SSR];

    // Constructor
    cumsum_ref_graph() {
        printf("============================\n");
        printf("== CUMSUM REF Graph\n");
        printf("============================\n");
        printf("TP_DIM_A             = %d\n", TP_DIM_A);
        printf("TP_DIM_B             = %d\n", TP_DIM_B);
        printf("TP_NUM_FRAMES        = %d\n", TP_NUM_FRAMES);
        printf("TP_MODE              = %d\n", TP_MODE);
        printf("TP_SHIFT             = %d\n", TP_SHIFT);
        printf("TP_RND               = %d\n", TP_RND);
        printf("TP_SAT               = %d\n", TP_SAT);
        printf("kWindowVsize         = %d\n", kWindowVsize);

        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] = kernel::create_object<cumsum_ref<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES,
                                                            TP_MODE, TP_SHIFT, TP_RND, TP_SAT> >();
            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.9; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "cumsum_ref.cpp";

            // make connections
            connect(in[i], m_kernels[i].in[0]);
            dimensions(m_kernels[i].in[0]) = {kWindowVsize};
            connect(m_kernels[i].out[0], out[i]);
            dimensions(m_kernels[i].out[0]) = {kWindowVsize};
        }
    };
};
}
}
}
}
#endif // _DSPLIB_CUMSUM_REF_GRAPH_HPP_
