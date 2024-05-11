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
#ifndef _DSPLIB_HADAMARD_REF_GRAPH_HPP_
#define _DSPLIB_HADAMARD_REF_GRAPH_HPP_

/*
This file holds the definition of the Hadamard Reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "hadamard_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace hadamard {
using namespace adf;

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class hadamard_ref_graph : public graph {
   public:
    static constexpr unsigned int kSamplesInVectOutData =
        TP_API == 0 ? 256 / 8 / (vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffWin)
                    : (128 / 8 / (vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffStream));
    static constexpr int DataSizePerSSR = TP_DIM / TP_SSR;
    static constexpr int paddedDataSize = CEIL(DataSizePerSSR, kSamplesInVectOutData);
    static constexpr int kWindowVsize = (TP_NUM_FRAMES * paddedDataSize);
    static constexpr int kPtSize = paddedDataSize;

    port<input> inA[TP_SSR];
    port<input> inB[TP_SSR];
    port<output> out[TP_SSR];

    kernel m_kernels[TP_SSR];

    // Constructor
    hadamard_ref_graph() {
        printf("============================\n");
        printf("== HADAMARD REF Graph\n");
        printf("============================\n");
        printf("TP_DIM               = %d\n", TP_DIM);
        printf("TP_NUM_FRAMES        = %d\n", TP_NUM_FRAMES);
        printf("TP_SHIFT             = %d\n", TP_SHIFT);
        printf("TP_API               = %d\n", TP_API);
        printf("TP_SSR               = %d\n", TP_SSR);
        printf("TP_RND               = %d\n", TP_RND);
        printf("TP_SAT               = %d\n", TP_SAT);

        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] = kernel::create_object<hadamard_ref<TT_DATA_A, TT_DATA_B, kPtSize, TP_NUM_FRAMES, TP_SHIFT,
                                                              TP_API, TP_SSR, TP_RND, TP_SAT> >();

            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.1; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "hadamard_ref.cpp";

            // make connections
            connect(inA[i], m_kernels[i].in[0]);
            connect(inB[i], m_kernels[i].in[1]);
            dimensions(m_kernels[i].in[0]) = {kWindowVsize};
            dimensions(m_kernels[i].in[1]) = {kWindowVsize};
            connect(m_kernels[i].out[0], out[i]);
            dimensions(m_kernels[i].out[0]) = {kWindowVsize};
        }
    };
};
}
}
}
}
#endif // _DSPLIB_HADAMARD_REF_GRAPH_HPP_
