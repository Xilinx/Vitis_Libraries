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
#ifndef _DSPLIB_KRONECKER_REF_GRAPH_HPP_
#define _DSPLIB_KRONECKER_REF_GRAPH_HPP_

/*
This file holds the definition of the Kronecker reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "kronecker_ref.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace kronecker {

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A_ROWS,
          unsigned int TP_DIM_A_COLS,
          unsigned int TP_DIM_B_ROWS,
          unsigned int TP_DIM_B_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_API,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR = 1,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
class kronecker_ref_graph : public graph {
   public:
    using out_t = outTypeMult_t<TT_DATA_A, TT_DATA_B>;
    static constexpr int sizeMatA = TP_DIM_A_ROWS * TP_DIM_A_COLS;
    static constexpr int sizeMatB = TP_DIM_B_ROWS * TP_DIM_B_COLS;
    static constexpr int sizeMatOut = sizeMatA * sizeMatB;
    static constexpr int kWindowVsizeA = (sizeMatA * TP_NUM_FRAMES);
    static constexpr int kWindowVsizeB = (sizeMatB * TP_NUM_FRAMES);
    static constexpr int outVecSize = 256 / 8 / sizeof(out_t);
    static constexpr int kWindowVsizeOut = CEIL((sizeMatOut * TP_NUM_FRAMES), outVecSize);

    port<input> inA[1];
    port<input> inB[1];
    port<output> out[1];
    kernel m_kernel;

    // Constructor
    kronecker_ref_graph() {
        printf("============================\n");
        printf("== KRONECKER REF Graph\n");
        printf("============================\n");
        printf("TP_DIM_A_ROWS        = %d\n", TP_DIM_A_ROWS);
        printf("TP_DIM_A_COLS        = %d\n", TP_DIM_A_COLS);
        printf("TP_DIM_B_ROWS        = %d\n", TP_DIM_B_ROWS);
        printf("TP_DIM_B_COLS        = %d\n", TP_DIM_B_COLS);
        printf("TP_NUM_FRAMES        = %d\n", TP_NUM_FRAMES);
        printf("TP_SHIFT             = %d\n", TP_SHIFT);
        printf("TP_API               = %d\n", TP_API);
        printf("TP_SSR               = %d\n", TP_SSR);
        printf("TP_RND               = %d\n", TP_RND);
        printf("TP_SAT               = %d\n", TP_SAT);

        m_kernel =
            kernel::create_object<kronecker_ref<TT_DATA_A, TT_DATA_B, TP_DIM_A_ROWS, TP_DIM_A_COLS, TP_DIM_B_ROWS,
                                                TP_DIM_B_COLS, TP_NUM_FRAMES, TP_API, TP_SHIFT, TP_RND, TP_SAT> >();

        // Make connections and set dimensions
        connect(inA[0], m_kernel.in[0]);
        dimensions(m_kernel.in[0]) = {kWindowVsizeA};
        connect(inB[0], m_kernel.in[1]);
        dimensions(m_kernel.in[1]) = {kWindowVsizeB};
        connect(m_kernel.out[0], out[0]);
        dimensions(m_kernel.out[0]) = {kWindowVsizeOut};
        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.8; // nominal figure
        // Source file(s)
        source(m_kernel) = "kronecker_ref.cpp";
    };
};
} // namespace kronecker
} // namespace aie
} // namespace xf
} // namespace dsp

#endif // _DSPLIB_KRONECKER_REF_GRAPH_HPP_
