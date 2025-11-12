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
#ifndef _SOLVERLIB_QRD_REF_GRAPH_HPP_
#define _SOLVERLIB_QRD_REF_GRAPH_HPP_

/*
This file holds the definition of the Hadamard Reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "qrd_ref.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace qrd {

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_A_LEADING,
          unsigned int TP_DIM_Q_LEADING,
          unsigned int TP_DIM_R_LEADING>
class qrd_ref_graph : public graph {
   public:

    static constexpr int kWindowVsizeQ = (TP_NUM_FRAMES * TP_DIM_COLS * TP_DIM_ROWS);
    static constexpr int kWindowVsizeR = (TP_NUM_FRAMES * TP_DIM_COLS * TP_DIM_COLS);

    port<input> inA;
    port<output> outQ;
    port<output> outR;

    kernel m_kernels;

    // Constructor
    qrd_ref_graph() {
        printf("============================\n");
        printf("== QRD REF Graph\n");
        printf("============================\n");
        printf("TP_DIM_ROWS          = %d\n", TP_DIM_ROWS);
        printf("TP_DIM_COLS          = %d\n", TP_DIM_COLS);
        printf("TP_NUM_FRAMES        = %d\n", TP_NUM_FRAMES);
        printf("TP_CASC_LEN          = %d\n", TP_CASC_LEN);
        printf("TP_DIM_A_LEADING     = %d\n", TP_DIM_A_LEADING);
        printf("TP_DIM_Q_LEADING     = %d\n", TP_DIM_Q_LEADING);
        printf("TP_DIM_R_LEADING     = %d\n", TP_DIM_R_LEADING);
        printf("kWindowVsizeQ        = %d\n", kWindowVsizeQ);
        printf("kWindowVsizeR        = %d\n", kWindowVsizeR);

        m_kernels = kernel::create_object<qrd_ref<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN> >();

        // Specify mapping constraints
        runtime<ratio>(m_kernels) = 0.1; // Nominal figure. The real figure requires knowledge of the sample
                                            // rate.
        // Source files
        source(m_kernels) = "qrd_ref.cpp";

        // make connections
        connect(inA, m_kernels.in[0]);
        dimensions(m_kernels.in[0]) = {kWindowVsizeQ};

        connect(m_kernels.out[0], outQ);
        connect(m_kernels.out[1], outR);
        dimensions(m_kernels.out[0]) = {kWindowVsizeQ};
        dimensions(m_kernels.out[1]) = {kWindowVsizeR};

        if (TP_DIM_A_LEADING == 1){
        write_access(m_kernels.in[0]) =
                adf::tiling(
                {       .buffer_dimension = {TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES},
                        .tiling_dimension = {1, TP_DIM_COLS, 1},
                        .offset = {0, 0, 0},
                        .tile_traversal = {
                        {.dimension = 0, .stride = 1, .wrap = TP_DIM_ROWS},
                        {.dimension = 1, .stride = TP_DIM_COLS, .wrap = 1},
                        {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                }
                });
        };


        if (TP_DIM_Q_LEADING == 1){
        read_access(m_kernels.out[0]) =
                adf::tiling(
                {       .buffer_dimension = {TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES},
                        .tiling_dimension = {1, TP_DIM_COLS, 1},
                        .offset = {0, 0, 0},
                        .tile_traversal = {
                        {.dimension = 0, .stride = 1, .wrap = TP_DIM_ROWS},
                        {.dimension = 1, .stride = TP_DIM_COLS, .wrap = 1},
                        {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                }
                });
        };

        if (TP_DIM_R_LEADING == 1){
        read_access(m_kernels.out[1]) =
                adf::tiling(
                {       .buffer_dimension = {TP_DIM_COLS, TP_DIM_COLS, TP_NUM_FRAMES},
                        .tiling_dimension = {1, TP_DIM_COLS, 1},
                        .offset = {0, 0, 0},
                        .tile_traversal = {
                        {.dimension = 0, .stride = 1, .wrap = TP_DIM_COLS},
                        {.dimension = 1, .stride = TP_DIM_COLS, .wrap = 1},
                        {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                }
                });
        };


    }; // end of constructor
}; // end of class qrd_ref_graph

} // namespace qrd
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _SOLVERLIB_QRD_REF_GRAPH_HPP_
