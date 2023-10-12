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
#include "aie/cholesky_float_decomposition_graph.hpp"

using namespace adf;

#define DIM 64

std::string matA = "matA_data";
std::string matA_file = "data/matA_" + std::to_string(DIM) + ".txt";
std::string matL = "matL_data";
std::string matL_file = "data/matL_" + std::to_string(DIM) + ".txt";
std::string outMatL_file = "data/outMatL_" + std::to_string(DIM) + ".txt";
std::string gldMatL_file = "data/gldMatL_" + std::to_string(DIM) + ".txt";

class TopGraph : public graph {
   public:
    input_plio in;
    output_plio out;
    xf::solver::CholeskyGraph<DIM> G;

    TopGraph() {
        in = input_plio::create("Column_12_TO_AIE", plio_32_bits, matA_file);
        out = output_plio::create("Column_28_FROM_AIE", plio_32_bits, outMatL_file);
        connect<>(in.out[0], G.matA_data);
        connect<>(G.matL_data, out.in[0]);
    }
};

#endif /**********__GRAPH_H__**********/
