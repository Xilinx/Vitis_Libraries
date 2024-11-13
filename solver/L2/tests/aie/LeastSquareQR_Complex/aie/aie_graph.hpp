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

/**
 * @file aie_graph.hpp
 * @brief This file captures the definition of the `L2` graph level class 
 **/


#ifndef _XF_SOLVER_LSTTEST_GRAPH_HPP_
#define _XF_SOLVER_LSTTEST_GRAPH_HPP_

#include "lstqr_graph.hpp"
#include "back_substitution_graph.hpp"
#include "transform_graph.hpp"
#include "aie_graph_params.h"

using namespace adf;

class TestGraph: public adf::graph {
    public: 
        input_plio plin0;
        input_plio plin1;
        output_plio plout0;
        output_plio plout1;

        input_port column_id0[N];
        //input_port column_id1[N];

        xf::solver::LSTQR_Graph<M, N, M, K> lstqrGraph;
        //xf::solver::Transform_Graph<M, N, M,  K> transformGraph;
        //xf::solver::BackSubstitution_Graph<M, N, M,  K> backSubstitutionGraph;

        TestGraph(std::string fin0, 
                  std::string fin1,
                  std::string fout0,
                  std::string fout1){
            plin0 = input_plio::create("in0", plio_128_bits, fin0, 1000);
            plin1 = input_plio::create("in1", plio_128_bits, fin1, 1000);
            plout0 = output_plio::create("out0", plio_128_bits, fout0, 1000);
            plout1 = output_plio::create("out1", plio_128_bits, fout1, 1000);

            connect<stream>(plin0.out[0], lstqrGraph.din0);
            connect<stream>(plin1.out[0], lstqrGraph.din1);


            connect<stream>(lstqrGraph.dout0, plout0.in[0]);
            connect<stream>(lstqrGraph.dout1, plout1.in[0]);

            //connect<stream>(lstqrGraph.dout0, transformGraph.din0);
            //connect<stream>(lstqrGraph.dout1, transformGraph.din1);

            //connect<stream>(transformGraph.dout0, backSubstitutionGraph.din0); 
            //connect<stream>(transformGraph.dout1,  backSubstitutionGraph.din1);

            //connect<stream>(backSubstitutionGraph.dout0, plout0.in[0]);
            //connect<stream>(backSubstitutionGraph.dout1, plout1.in[0]);

            for(int j=0; j<N; j++){
                connect<parameter>(column_id0[j], lstqrGraph.column_id[j]);
                //connect<parameter>(column_id1[j], backSubstitutionGraph.column_id[j]);
            }
        }
};


#endif
