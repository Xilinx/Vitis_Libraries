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
#ifndef _DSPLIB_BITONIC_SORT_REF_GRAPH_HPP_
#define _DSPLIB_BITONIC_SORT_REF_GRAPH_HPP_

/*
This file holds the definition of the Bitonic Sort Reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "bitonic_sort_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {
using namespace adf;

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_SSR = 1>
class bitonic_sort_ref_graph : public graph {
   public:
    static constexpr int kDim = TP_DIM;

    port<input> in;
    port<output> out;

    kernel m_kernel;

    // Constructor
    bitonic_sort_ref_graph() {
        printf("============================\n");
        printf("== BITONIC_SORT REF Graph\n");
        printf("============================\n");
        printf("TP_DIM                  = %d\n", TP_DIM);
        printf("TP_NUM_FRAMES           = %d\n", TP_NUM_FRAMES);
        printf("TP_ASCENDING            = %d\n", TP_ASCENDING);
        printf("TP_CASC_LEN             = %d\n", TP_CASC_LEN);

        m_kernel = kernel::create_object<bitonic_sort_ref<TT_DATA, kDim, TP_NUM_FRAMES, TP_ASCENDING, 1, 1> >();

        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.9; // Nominal figure. The real figure requires knowledge of the sample
                                        // rate.
        // Source files
        source(m_kernel) = "bitonic_sort_ref.cpp";

        // make connections
        connect(in, m_kernel.in[0]);
        dimensions(m_kernel.in[0]) = {kDim * TP_NUM_FRAMES};
        connect(m_kernel.out[0], out);
        dimensions(m_kernel.out[0]) = {kDim * TP_NUM_FRAMES};
    };
};
}
}
}
}
#endif // _DSPLIB_BITONIC_SORT_REF_GRAPH_HPP_
