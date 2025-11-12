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
#include "cholesky_graph.hpp"

using namespace adf;
namespace cholesky_example {

#define DATA_TYPE_CHOLESKY cfloat
#define DIM_SIZE_CHOLESKY 8 
#define NUM_FRAMES_CHOLESKY 4
#define GRID_DIM_CHOLESKY 1

class test_cholesky: public adf::graph {
   public:
    port<input> in;
    port<output> out;
    xf::solver::aie::cholesky::cholesky_graph<DATA_TYPE_CHOLESKY, DIM_SIZE_CHOLESKY, NUM_FRAMES_CHOLESKY, GRID_DIM_CHOLESKY>
        choleskyGraph;

    test_cholesky() {
        connect<>(in, choleskyGraph.in[0]);
        connect<>(choleskyGraph.out[0], out);
    };
};
};