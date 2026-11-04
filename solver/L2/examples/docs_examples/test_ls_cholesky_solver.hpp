/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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
#include "ls_cholesky_solver_graph.hpp"

using namespace adf;
namespace ls_cholesky_solver_example {

#define DATA_TYPE_LS_CHOLESKY_SOLVER cfloat
#define DIM_SIZE_LS_CHOLESKY_SOLVER 8
#define NUM_FRAMES_LS_CHOLESKY_SOLVER 1
#define GRID_DIM_LS_CHOLESKY_SOLVER 1
#define CASC_LEN_LS_CHOLESKY_SOLVER 1

class test_ls_cholesky_solver : public adf::graph {
   public:
    port<input> A_in;
    port<input> y_in;
    port<output> x_out;
    xf::solver::aie::ls_cholesky_solver::ls_cholesky_solver_graph<DATA_TYPE_LS_CHOLESKY_SOLVER,
                                                                  DIM_SIZE_LS_CHOLESKY_SOLVER,
                                                                  NUM_FRAMES_LS_CHOLESKY_SOLVER,
                                                                  GRID_DIM_LS_CHOLESKY_SOLVER,
                                                                  CASC_LEN_LS_CHOLESKY_SOLVER>
        lsCholeskySolverGraph;

    test_ls_cholesky_solver() {
        connect<>(A_in, lsCholeskySolverGraph.A_in[0]);
        connect<>(y_in, lsCholeskySolverGraph.y_in[0]);
        connect<>(lsCholeskySolverGraph.x_out[0], x_out);
    };
};
};
