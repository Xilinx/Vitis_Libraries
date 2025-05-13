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
#include <adf.h>
#include "kronecker_graph.hpp"

using namespace adf;
namespace kronecker_example {

#define TT_DATA_A_KMP int32
#define TT_DATA_B_KMP int32
#define TP_DIM_A_ROWS_KMP 16
#define TP_DIM_A_COLS_KMP 4
#define TP_DIM_B_ROWS_KMP 16
#define TP_DIM_B_COLS_KMP 4
#define TP_NUM_FRAMES_KMP 2
#define TP_API_KMP 1
#define TP_SHIFT_KMP 0
#define TP_SSR_KMP 1
#define TP_RND_KMP 0
#define TP_SAT_KMP 1

class test_kronecker : public adf::graph {
   public:
    port<input> inA;
    port<input> inB;
    port<output> out;
    xf::dsp::aie::kronecker::kronecker_graph<TT_DATA_A_KMP,
                                             TT_DATA_B_KMP,
                                             TP_DIM_A_ROWS_KMP,
                                             TP_DIM_A_COLS_KMP,
                                             TP_DIM_B_ROWS_KMP,
                                             TP_DIM_B_COLS_KMP,
                                             TP_NUM_FRAMES_KMP,
                                             TP_API_KMP,
                                             TP_SHIFT_KMP,
                                             TP_SSR_KMP,
                                             TP_RND_KMP,
                                             TP_SAT_KMP>
        kroneckerGraph;

    test_kronecker() {
        connect<>(inA, kroneckerGraph.inA[0]);
        connect<>(inB, kroneckerGraph.inB[0]);
        connect<>(kroneckerGraph.out[0], out);
    };
};
};