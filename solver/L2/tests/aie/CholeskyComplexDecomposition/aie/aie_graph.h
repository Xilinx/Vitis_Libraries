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
#include "aie/cholesky_complex_decomposition_graph.hpp"

using namespace adf;

#define DIM 64

std::string matA_real_file = "data/matA_real_" + std::to_string(DIM) + ".txt";
std::string matA_imag_file = "data/matA_imag_" + std::to_string(DIM) + ".txt";
std::string matL_real_file = "data/matL_real_" + std::to_string(DIM) + ".txt";
std::string matL_imag_file = "data/matL_imag_" + std::to_string(DIM) + ".txt";
std::string outMatL_real_file = "data/outMatL_real_" + std::to_string(DIM) + ".txt";
std::string outMatL_imag_file = "data/outMatL_imag_" + std::to_string(DIM) + ".txt";
std::string gldMatL_real_file = "data/gldMatL_real_" + std::to_string(DIM) + ".txt";
std::string gldMatL_imag_file = "data/gldMatL_imag_" + std::to_string(DIM) + ".txt";

class TopGraph : public graph {
   public:
    input_plio in_0;
    input_plio in_1;
    output_plio out_0;
    output_plio out_1;
    xf::solver::CholeskyGraph<DIM> G;

    TopGraph() {
        in_0 = input_plio::create("Column_12_TO_AIE", plio_32_bits, matA_real_file);
        in_1 = input_plio::create("Column_13_TO_AIE", plio_32_bits, matA_imag_file);
        out_0 = output_plio::create("Column_28_FROM_AIE", plio_32_bits, outMatL_real_file);
        out_1 = output_plio::create("Column_29_FROM_AIE", plio_32_bits, outMatL_imag_file);
        connect<>(in_0.out[0], G.matA_real_data);
        connect<>(in_1.out[0], G.matA_imag_data);
        connect<>(G.matL_real_data, out_0.in[0]);
        connect<>(G.matL_imag_data, out_1.in[0]);
    }
};

#endif /**********__GRAPH_H__**********/
