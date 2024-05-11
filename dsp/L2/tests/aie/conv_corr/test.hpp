/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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

// This file holds the definition of the test harness for the conv_corr graph.

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
#define UUT_GRAPH conv_corr_graph
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
    std::array<input_plio, 1> inWindowF;
    std::array<input_plio, 1> inWindowG;
    std::array<output_plio, 1> outWindow;

    // Constructor
    test_graph() {
        printf("=============================================================\n");
        printf("   conv_corr test.hpp parameters:    ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("=============================================================\n");
        printf("Data type of F Sig.  = ");
        printf(QUOTE(DATA_F));
        printf("\n");
        printf("Data type of G Sig.  = ");
        printf(QUOTE(DATA_G));
        printf("\n");
        printf("Data type of Conv/Corr Output.  = ");
        printf(QUOTE(DATA_OUT));
        printf("\n");
        printf("FUNCT_TYPE      = %d \n", FUNCT_TYPE);
        printf("COMPUTE_MODE   = %d \n", COMPUTE_MODE);
        printf("F_LEN          = %d \n", F_LEN);
        printf("G_LEN          = %d \n", G_LEN);
        printf("SHIFT          = %d \n", SHIFT);
        printf("API            = %d \n", API_PORT);
        printf("RND            = %d \n", RND);
        printf("SAT            = %d \n", SAT);

        // Conv_Corr sub-graph
        dsplib::conv_corr::UUT_GRAPH<DATA_F, DATA_G, DATA_OUT, FUNCT_TYPE, COMPUTE_MODE, F_LEN, G_LEN, SHIFT, API_PORT,
                                     RND, SAT>
            conv_corrGraph;

        // Make connections:
        std::string inpFile_F_Sig = QUOTE(INPUT_FILE_F);
        std::string inpFile_G_Sig = QUOTE(INPUT_FILE_G);
        inWindowF[0] = input_plio::create("PLIO_inpFile_F_Sig", adf::plio_32_bits, inpFile_F_Sig);
        inWindowG[0] = input_plio::create("PLIO_inpFile_G_Sig", adf::plio_32_bits, inpFile_G_Sig);
        connect<>(inWindowF[0].out[0], conv_corrGraph.inWindowF);
        connect<>(inWindowG[0].out[0], conv_corrGraph.inWindowG);

        std::string OutFile = QUOTE(OUTPUT_FILE);
        outWindow[0] = output_plio::create("PLIO_OutFile", adf::plio_32_bits, OutFile);
        connect<>(conv_corrGraph.outWindow, outWindow[0].in[0]);

        printf("==================================\n");
        printf("======= End of sub-graph ======\n");
        printf("==================================\n");
    };
};

} // namespace conv_corr
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_TEST_HPP_
