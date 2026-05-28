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
#include "substitution_graph.hpp"

using namespace adf;
namespace substitution_example {

#define DATA_TYPE_SUBSTITUTION cfloat
#define DIM_SIZE_SUBSTITUTION 8
#define SUBST_TYPE_SUBSTITUTION 1
#define L_LEADING_SUBSTITUTION 0
#define GRID_DIM_SUBSTITUTION 1

class test_substitution: public adf::graph {
   public:
    port<input> L_in;
    port<input> y_in;
    port<output> x_out;
    xf::solver::aie::substitution::substitution_graph<DATA_TYPE_SUBSTITUTION, DIM_SIZE_SUBSTITUTION, L_LEADING_SUBSTITUTION, GRID_DIM_SUBSTITUTION>
        substitutionGraph;

    test_substitution() {
        connect<>(L_in, substitutionGraph.L_in[0]);
        connect<>(y_in, substitutionGraph.y_in[0]);
        connect<>(substitutionGraph.x_out[0], x_out);
    };
};
};
