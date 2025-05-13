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
#ifndef _DSPLIB_FUNC_APPROX_REF_GRAPH_HPP_
#define _DSPLIB_FUNC_APPROX_REF_GRAPH_HPP_

/*
This file holds the definition of the Function Approximation Reference model graph.
*/

#include <adf.h>
#include <vector>
#include "func_approx_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {
using namespace adf;

template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_FUNC_CHOICE>
class func_approx_ref_graph : public graph {
   public:
    port<input> in[1];
    port<output> out[1];

    kernel m_kernel[1];

    // Constructor
    func_approx_ref_graph() {
        printf("============================\n");
        printf("== FUNC_APPROX REF Graph\n");
        printf("============================\n");
        printf("WINDOW_VSIZE      = %d\n", TP_WINDOW_VSIZE);

        m_kernel[0] =
            kernel::create_object<func_approx_ref<TT_DATA, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE,
                                                  TP_WINDOW_VSIZE, TP_SHIFT, TP_RND, TP_SAT, TP_FUNC_CHOICE> >();
        // Specify mapping constraints
        runtime<ratio>(m_kernel[0]) = 0.8; // Nominal figure. The real figure requires knowledge of the sample rate.
        // Source files
        source(m_kernel[0]) = "func_approx_ref.cpp";

        // make connections
        connect<>(in[0], m_kernel[0].in[0]);
        dimensions(m_kernel[0].in[0]) = {TP_WINDOW_VSIZE};
        connect<>(m_kernel[0].out[0], out[0]);
        dimensions(m_kernel[0].out[0]) = {TP_WINDOW_VSIZE};
    };
};
}
}
}
}
#endif // _DSPLIB_FUNC_APPROX_REF_GRAPH_HPP_
