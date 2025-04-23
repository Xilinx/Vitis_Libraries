/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <adf.h>
#include "cholesky_complex_decomposition_flexible_graph.hpp"
#include "aie_graph_params.h"

using namespace adf;

class TopGraph : public graph {
   public:
    input_plio in;
    output_plio out;
    xf::solver::CholeskyFlexibleGraph<DIM, CoreNum, BlkNum> G;

    TopGraph() {
        in = input_plio::create("PLIO_i", plio_128_bits, fin);
        out = output_plio::create("PLIO_o", plio_128_bits, fout);
        connect<>(in.out[0], G.sig_i);
        connect<>(G.sig_o, out.in[0]);
    }
};

#endif /**********__GRAPH_H__**********/
