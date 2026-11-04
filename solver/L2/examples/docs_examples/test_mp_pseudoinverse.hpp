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
#include "mp_pseudoinverse_svd_graph.hpp"

using namespace adf;
namespace mp_pseudoinverse_example {

#define DATA_TYPE_MP_PSEUDOINVERSE float
#define DIM_ROWS_MP_PSEUDOINVERSE 8
#define DIM_COLS_MP_PSEUDOINVERSE 4
#define PASSES_MP_PSEUDOINVERSE 4
#define CASC_LEN_MP_PSEUDOINVERSE 1

class test_mp_pseudoinverse : public adf::graph {
   public:
    port<input> in;
    port<output> out;
    xf::solver::aie::mp_pseudoinverse_svd::mp_pseudoinverse_svd_graph<DATA_TYPE_MP_PSEUDOINVERSE,
                                                                      DIM_ROWS_MP_PSEUDOINVERSE,
                                                                      DIM_COLS_MP_PSEUDOINVERSE,
                                                                      PASSES_MP_PSEUDOINVERSE,
                                                                      CASC_LEN_MP_PSEUDOINVERSE>
        mpPseudoinverseGraph;

    test_mp_pseudoinverse() {
        connect<>(in, mpPseudoinverseGraph.in);
        connect<>(mpPseudoinverseGraph.out, out);
    };
};
};
