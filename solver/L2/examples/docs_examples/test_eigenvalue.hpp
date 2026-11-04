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
#include "eigenvalue_graph.hpp"

using namespace adf;
namespace eigenvalue_example {

#define DATA_TYPE_EIGENVALUE cfloat
#define DIM_SIZE_EIGENVALUE 8
#define PASSES_EIGENVALUE 30
#define CASC_LEN_EIGENVALUE 1

class test_eigenvalue : public adf::graph {
   public:
    port<input> in;
    port<output> out;
    xf::solver::aie::eigenvalue::
        eigenvalue_graph<DATA_TYPE_EIGENVALUE, DIM_SIZE_EIGENVALUE, PASSES_EIGENVALUE, CASC_LEN_EIGENVALUE>
            eigenvalueGraph;

    test_eigenvalue() {
        connect<>(in, eigenvalueGraph.in);
        connect<>(eigenvalueGraph.out, out);
    };
};
};
