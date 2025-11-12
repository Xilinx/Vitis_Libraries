/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_TEST_HPP_
#define _SOLVERLIB_TEST_HPP_

// This file holds the definition of the test harness for the qrd graph.

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
#define UUT_GRAPH qrd_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace testcase {
namespace solverlib = xf::solver::aie;

class test_graph : public graph {
   public:
    std::array<input_plio, CASC_LEN> inA;
    std::array<output_plio, CASC_LEN> outQ;
    std::array<output_plio, CASC_LEN> outR;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== QRD test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type      = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("DIM_ROWS          = %d \n", DIM_ROWS);
        printf("DIM_COLS          = %d \n", DIM_COLS);
        printf("NUM_FRAMES        = %d \n", NUM_FRAMES);
        printf("CASC_LEN          = %d \n", CASC_LEN);
        printf("DIM_A_LEADING     = %d \n", DIM_A_LEADING);
        printf("DIM_Q_LEADING     = %d \n", DIM_Q_LEADING);
        printf("DIM_R_LEADING     = %d \n", DIM_R_LEADING);

        // Hadamard sub-graph
        solverlib::qrd::UUT_GRAPH<DATA_TYPE, 
                                DIM_ROWS, 
                                DIM_COLS, 
                                NUM_FRAMES, 
                                CASC_LEN,
                                DIM_A_LEADING,
                                DIM_Q_LEADING,
                                DIM_R_LEADING>
            qrdGraph;

// Make connections
#ifdef USING_UUT
        for (int i = 0; i < CASC_LEN; i++) {
            std::string filenameInA = QUOTE(INPUT_FILE_A);
            filenameInA.insert(filenameInA.length() - 4, ("_" + std::to_string(i)));
            inA[i] = input_plio::create("PLIO_inA_" + std::to_string(i), adf::plio_64_bits, filenameInA);
            connect<>(inA[i].out[0], qrdGraph.inA[i]);

            std::string filenameOutQ = QUOTE(OUTPUT_FILE);
            std::string filenameOutR = QUOTE(OUTPUT_FILE2);
            filenameOutQ.insert(filenameOutQ.length() - 4, ("_" + std::to_string(i)));
            filenameOutR.insert(filenameOutR.length() - 4, ("_" + std::to_string(i)));
            outQ[i] = output_plio::create("PLIO_outQ_" + std::to_string(i), adf::plio_64_bits, filenameOutQ);
            outR[i] = output_plio::create("PLIO_outR_" + std::to_string(i), adf::plio_64_bits, filenameOutR);
            connect<>(qrdGraph.outQ[i], outQ[i].in[0]);
            connect<>(qrdGraph.outR[i], outR[i].in[0]);
        }
#else
        std::string filenameInA = QUOTE(INPUT_FILE_A);
        inA[0] = input_plio::create("PLIO_inA_" + std::to_string(0), adf::plio_64_bits, filenameInA);
        connect<>(inA[0].out[0], qrdGraph.inA);

        std::string filenameOutQ = QUOTE(OUTPUT_FILE);
        std::string filenameOutR = QUOTE(OUTPUT_FILE2);
        outQ[0] = output_plio::create("PLIO_outQ_" + std::to_string(0), adf::plio_64_bits, filenameOutQ);
        outR[0] = output_plio::create("PLIO_outR_" + std::to_string(0), adf::plio_64_bits, filenameOutR);
        connect<>(qrdGraph.outQ, outQ[0].in[0]);
        connect<>(qrdGraph.outR, outR[0].in[0]);

#endif
        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
