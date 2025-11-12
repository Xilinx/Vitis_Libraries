/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_CHOLESKY_REF_GRAPH_HPP_
#define _DSPLIB_CHOLESKY_REF_GRAPH_HPP_

/*
This file holds the definition of the Bitonic Sort Reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "cholesky_ref.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace cholesky {
using namespace adf;

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES>
class cholesky_ref_graph : public graph {
   public:

    port<input> in;
    port<output> out;

    kernel m_kernel;

    // Constructor
    cholesky_ref_graph() {
        printf("============================\n");
        printf("== CHOLESKY REF Graph\n");
        printf("============================\n");
        printf("TP_DIM                  = %d\n", TP_DIM);
        printf("TP_NUM_FRAMES           = %d\n", TP_NUM_FRAMES);

        m_kernel = kernel::create_object<cholesky_ref<TT_DATA, TP_DIM, TP_NUM_FRAMES>>();

        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.9; // Nominal figure. The real figure requires knowledge of the sample
                                            // rate.
        // Source files
        source(m_kernel) = "cholesky_ref.cpp";

        // make connections
        connect(in, m_kernel.in[0]);
        dimensions(m_kernel.in[0]) = {TP_DIM * TP_DIM * TP_NUM_FRAMES};
        connect(m_kernel.out[0], out);
        dimensions(m_kernel.out[0]) = {TP_DIM * TP_DIM * TP_NUM_FRAMES};
    };
};
}
}
}
}
#endif // _DSPLIB_CHOLESKY_REF_GRAPH_HPP_
