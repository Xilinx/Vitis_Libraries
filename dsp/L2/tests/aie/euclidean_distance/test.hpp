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

#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

// This file holds the definition of the test harness for the euclidean_distance graph.

#include <adf.h>
#include <vector>
#include <array>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH euclidean_distance_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
namespace dsplib = xf::dsp::aie;

class test_graph : public graph {
   public:
    std::array<input_plio, 1> inWindowP;
    std::array<input_plio, 1> inWindowQ;
    std::array<output_plio, 1> outWindow;

    // Constructor
    test_graph() {
        printf("=============================================================\n");
        printf("   euclidean_distance test.hpp parameters:    ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("=============================================================\n");
        printf("Data type of Point P, Q and Output of ED.  = ");
        printf(QUOTE(DATA));
        printf("\n");
        printf("Data type of euclidean_distance Output.  = ");
        printf(QUOTE(DATA_OUT));
        printf("\n");
        printf("LEN                 = %d \n", LEN);
        printf("DIM                 = %d \n", DIM);
        printf("API                   = %d \n", API_IO);
        printf("RND                   = %d \n", RND);
        printf("SAT                   = %d \n", SAT);
        printf("IS_OUTPUT_SQUARED     = %d \n", IS_OUTPUT_SQUARED);

        // euclidean_distance sub-graph
        dsplib::euclidean_distance::UUT_GRAPH<DATA, DATA_OUT, LEN, DIM, API_IO, RND, SAT, IS_OUTPUT_SQUARED>
            euclidean_distanceGraph;

        // Make connections:
        std::string inpFile_Point_P = QUOTE(INPUT_FILE_P);
        std::string inpFile_Point_Q = QUOTE(INPUT_FILE_Q);
        inWindowP[0] = input_plio::create("PLIO_inpFile_Point_P", adf::plio_64_bits, inpFile_Point_P);
        inWindowQ[0] = input_plio::create("PLIO_inpFile_Point_Q", adf::plio_64_bits, inpFile_Point_Q);
        connect<>(inWindowP[0].out[0], euclidean_distanceGraph.inWindowP);
        connect<>(inWindowQ[0].out[0], euclidean_distanceGraph.inWindowQ);

        std::string OutFile = QUOTE(OUTPUT_FILE);
        outWindow[0] = output_plio::create("PLIO_OutFile", adf::plio_64_bits, OutFile);
        connect<>(euclidean_distanceGraph.outWindow, outWindow[0].in[0]);

        printf("==================================\n");
        printf("======= End of sub-graph ======\n");
        printf("==================================\n");
    };
};

} // namespace euclidean_distance
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_TEST_HPP_
