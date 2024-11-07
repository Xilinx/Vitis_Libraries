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

// This file holds the definition of the test harness for the outer_tensor graph.

#include <adf.h>
#include <vector>
#include <array>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#include "device_defs.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH outer_tensor_graph
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
    std::array<input_plio, UUT_SSR> inA;
    std::array<input_plio, UUT_SSR> inB;
    std::array<output_plio, UUT_SSR> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== Outer Tensor test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type  A       = ");
        printf(QUOTE(T_DATA_A));
        printf("\n");
        printf("Data type  B       = ");
        printf(QUOTE(T_DATA_B));
        printf("\n");
        printf("DIM_SIZE_A        = %d \n", DIM_SIZE_A);
        printf("DIM_SIZE_B        = %d \n", DIM_SIZE_B);
        printf("NUM_FRAMES        = %d \n", NUM_FRAMES);
        printf("SHIFT             = %d \n", SHIFT);
        printf("API_IO            = %d \n", API_IO);
        printf("SSR               = %d \n", UUT_SSR);
        printf("ROUND_MODE        = %d \n", ROUND_MODE);
        printf("SAT_MODE          = %d \n", SAT_MODE);
        printf("========================\n");

#ifdef USING_UUT
        // Outer Tensor sub-graph
        dsplib::outer_tensor::UUT_GRAPH<T_DATA_A, T_DATA_B, DIM_SIZE_A, DIM_SIZE_B, NUM_FRAMES, SHIFT, API_IO, UUT_SSR,
                                        ROUND_MODE, SAT_MODE>
            outer_tensorGraph;

        // Make connections
        for (int i = 0; i < UUT_SSR; i++) {
            std::string filenameInA = QUOTE(INPUT_FILE_A);
            std::string filenameInB = QUOTE(INPUT_FILE_B);
            filenameInA.insert(filenameInA.length() - 4, ("_" + std::to_string(i) + "_0"));
            // filenameInB.insert(filenameInB.length()-4, ("_"+std::to_string(i)+"_0"));
            inA[i] = input_plio::create("PLIO_inA_" + std::to_string(i), adf::plio_64_bits, filenameInA);
            inB[i] = input_plio::create("PLIO_inB_" + std::to_string(i), adf::plio_64_bits, filenameInB);
            connect<>(inA[i].out[0], outer_tensorGraph.inA[i]);
            connect<>(inB[i].out[0], outer_tensorGraph.inB[i]);

            std::string filenameOut = QUOTE(OUTPUT_FILE);
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i)));
            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_64_bits, filenameOut);
            connect<>(outer_tensorGraph.out[i], out[i].in[0]);
        }
#else
        // Outer Tensor sub-graph
        dsplib::outer_tensor::UUT_GRAPH<T_DATA_A, T_DATA_B, DIM_SIZE_A, DIM_SIZE_B, NUM_FRAMES, SHIFT, API_IO, 1,
                                        ROUND_MODE, SAT_MODE>
            outer_tensorGraph;

        std::string filenameInA = QUOTE(INPUT_FILE_A);
        std::string filenameInB = QUOTE(INPUT_FILE_B);

        inA[0] = input_plio::create("PLIO_inA_" + std::to_string(0), adf::plio_64_bits, filenameInA);
        inB[0] = input_plio::create("PLIO_inB_" + std::to_string(0), adf::plio_64_bits, filenameInB);
        connect<>(inA[0].out[0], outer_tensorGraph.inA[0]);
        connect<>(inB[0].out[0], outer_tensorGraph.inB[0]);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(outer_tensorGraph.out[0], out[0].in[0]);
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_