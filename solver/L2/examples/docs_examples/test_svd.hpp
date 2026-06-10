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
#include <adf.h>
#include "svd_graph.hpp"

using namespace adf;
namespace svd_example {

#define DATA_TYPE_SVD float
#define DIM_ROWS_SVD 8
#define DIM_COLS_SVD 4
#define PASSES_SVD 4
#define CASC_LEN_SVD 1

class test_svd : public adf::graph {
   public:
    port<input> in;
    port<output> outU;
    port<output> outS;
    port<output> outV;
    xf::solver::aie::svd::svd_graph<DATA_TYPE_SVD, DIM_ROWS_SVD, DIM_COLS_SVD, PASSES_SVD, CASC_LEN_SVD> svdGraph;

    test_svd() {
        connect<>(in, svdGraph.in[0]);
        connect<>(svdGraph.outU[0], outU);
        connect<>(svdGraph.outS[0], outS);
        connect<>(svdGraph.outV[0], outV);
    };
};
};
