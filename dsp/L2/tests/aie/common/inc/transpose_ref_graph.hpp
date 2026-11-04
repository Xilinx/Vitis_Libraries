/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_TRANSPOSE_REF_GRAPH_HPP_
#define _DSPLIB_TRANSPOSE_REF_GRAPH_HPP_

/*
This file holds the definition of the transpose reference model graph. It always uses a single
kernel over the full, logical TP_DIM_ROWS x TP_DIM_COLS matrix, regardless of the UUT's
TP_SSR/TP_CASC_LEN/TP_SPLIT_MODE - the test bench's transpose_partition_pad.pl script is
responsible for reassembling the (possibly multi-kernel, padded) UUT output into the same
layout before the two are diffed.

Like the UUT graph, this kernel's iobuffers are padded up to the device's memory access width
on both axes (see fnTposePad() in transpose_utils.hpp) - not just as a convention match, but
because an AIE iobuffer's byte size is required to be a multiple of 4 bytes, which an arbitrary,
un-padded TP_DIM_ROWS x TP_DIM_COLS buffer cannot generally guarantee. The test bench pads this
kernel's input and strips its output the same way (transpose_partition_pad.pl's
--pad-single/--strip-single modes) before comparing against the UUT.

TP_SSR/TP_CASC_LEN/TP_SPLIT_MODE are accepted purely for template-argument compatibility with
transpose_graph (test.hpp instantiates whichever graph UUT_GRAPH points to with the same
argument list) and otherwise unused here.
*/

#include <adf.h>
#include "transpose_kernel_ref.hpp"
#include "transpose_utils.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace transpose {

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_SSR = 1,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_SPLIT_MODE = 0,
          unsigned int TP_CONJUGATE = 0>
class transpose_ref_graph : public graph {
   private:
    static constexpr unsigned int kPadRows = fnTposePad<TT_DATA>(TP_DIM_ROWS);
    static constexpr unsigned int kPadCols = fnTposePad<TT_DATA>(TP_DIM_COLS);

   public:
    port<input> in[1];
    port<output> out[1];
    kernel m_kernel;

    // Constructor
    transpose_ref_graph() {
        printf("============================\n");
        printf("== TRANSPOSE REF Graph\n");
        printf("============================\n");
        printf("TP_DIM_ROWS          = %d\n", TP_DIM_ROWS);
        printf("TP_DIM_COLS          = %d\n", TP_DIM_COLS);
        printf("TP_CONJUGATE         = %d\n", TP_CONJUGATE);

        m_kernel = kernel::create_object<transpose_kernel_ref<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_CONJUGATE> >();

        connect(in[0], m_kernel.in[0]);
        dimensions(m_kernel.in[0]) = {kPadRows * kPadCols};
        connect(m_kernel.out[0], out[0]);
        dimensions(m_kernel.out[0]) = {kPadCols * kPadRows};

        runtime<ratio>(m_kernel) = 0.8; // nominal figure
        source(m_kernel) = "transpose_kernel_ref.cpp";
    };
};
}
}
}
}

#endif // _DSPLIB_TRANSPOSE_REF_GRAPH_HPP_
