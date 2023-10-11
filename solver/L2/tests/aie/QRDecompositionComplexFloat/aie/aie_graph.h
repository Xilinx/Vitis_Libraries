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
#include "qrd_graph.hpp"
#include "aie_graph_params.h"

using namespace adf;

class TopGraph : public graph {
   public:
    input_plio in_0;
    input_plio in_1;
    output_plio out_0;
    output_plio out_1;
    xf::solver::QRDComplexFloat<test_param::col_num, test_param::row_num, test_param::rep_num> G;

    TopGraph(std::string A_0_filename,
             std::string A_1_filename,
             std::string Res_0_filename,
             std::string Res_1_filename) {
        in_0 = input_plio::create("Column_12_TO_AIE", plio_128_bits, A_0_filename, 1000);
        in_1 = input_plio::create("Column_13_TO_AIE", plio_128_bits, A_1_filename, 1000);
        out_0 = output_plio::create("Column_28_FROM_AIE", plio_128_bits, Res_0_filename, 1000);
        out_1 = output_plio::create("Column_29_FROM_AIE", plio_128_bits, Res_1_filename, 1000);
        connect<>(in_0.out[0], G.in_0);
        connect<>(in_1.out[0], G.in_1);
        connect<>(G.out_0, out_0.in[0]);
        connect<>(G.out_1, out_1.in[0]);
    }
};

#endif /**********__GRAPH_H__**********/
