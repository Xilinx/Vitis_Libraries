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
#ifndef _DSPLIB_WIDGET_CUT_DECK_REF_GRAPH_HPP_
#define _DSPLIB_WIDGET_CUT_DECK_REF_GRAPH_HPP_

/*
This file holds the definition of the Widget API Cast Reference model graph.
*/

#include <adf.h>
#include <vector>
#include "widget_cut_deck_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace deck_cut {
using namespace adf;

template <typename TT_DATA, // type of data input and output
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ENTRY> // does not include header, so refers to payload samples
class widget_cut_deck_ref_graph : public graph {
   public:
    port<input> in;
    port<output> out;

    kernel m_kernel;

    // Constructor
    widget_cut_deck_ref_graph() {
        printf("============================\n");
        printf("== WIDGET API CAST REF Graph\n");
        printf("============================\n");
        printf("TP_POINT_SIZE     = %d\n", TP_POINT_SIZE);
        printf("TP_WINDOW_VSIZE   = %d\n", TP_WINDOW_VSIZE);

        m_kernel = kernel::create_object<widget_cut_deck_ref<TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_ENTRY> >();

        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.
        // Source files
        source(m_kernel) = "widget_cut_deck_ref.cpp";

        // make connections
        connect(in, m_kernel.in[0]);
        dimensions(m_kernel.in[0]) = {TP_WINDOW_VSIZE};

        connect(m_kernel.out[0], out);
        if
            constexpr(TP_ENTRY) { dimensions(m_kernel.out[0]) = {TP_WINDOW_VSIZE}; }
        else {
            dimensions(m_kernel.out[0]) = {TP_WINDOW_VSIZE / 2};
        }
    };
};
}
}
}
}
}
#endif // _DSPLIB_WIDGET_CUT_DECK_REF_GRAPH_HPP_
