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

// This file holds the definition of the test harness for the Kronecker matrix product.

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
#define UUT_GRAPH kronecker_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

namespace dsplib = xf::dsp::aie;

class test_graph : public graph {
   private:
   public:
    std::array<input_plio, UUT_SSR> inA;
    std::array<input_plio, UUT_SSR> inB;
    std::array<output_plio, UUT_SSR> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== Kronecker test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type A        = ");
        printf(QUOTE(T_DATA_A));
        printf("\n");
        printf("DATA type B       = ");
        printf(QUOTE(T_DATA_B));
        printf("\n");
        printf("DIM_A_ROWS        = %d \n", DIM_A_ROWS);
        printf("DIM_A_COLS        = %d \n", DIM_A_COLS);
        printf("DIM_B_ROWS        = %d \n", DIM_B_ROWS);
        printf("DIM_B_COLS        = %d \n", DIM_B_COLS);
        printf("NUM_FRAMES        = %d \n", NUM_FRAMES);
        printf("API_IO            = %d \n", API_IO);
        printf("SHIFT             = %d \n", SHIFT);
        printf("UUT_SSR           = %d \n", UUT_SSR);
        printf("ROUND_MODE        = %d \n", ROUND_MODE);
        printf("SAT_MODE          = %d \n", SAT_MODE);
        printf("========================\n");

        // Kronecker Matrix Product graph
        dsplib::kronecker::UUT_GRAPH<T_DATA_A, T_DATA_B, DIM_A_ROWS, DIM_A_COLS, DIM_B_ROWS, DIM_B_COLS, NUM_FRAMES,
                                     API_IO, SHIFT, UUT_SSR, ROUND_MODE, SAT_MODE>
            kroneckerGraph;

#ifdef USING_UUT
        for (int i = 0; i < UUT_SSR; i++) {
            // files names
            std::string filenameIn_A = QUOTE(INPUT_FILE_A);
            std::string filenameIn_B = QUOTE(INPUT_FILE_B);
            std::string filenameOut = QUOTE(OUTPUT_FILE);
            filenameIn_A.insert(filenameIn_A.length() - 4, ("_" + std::to_string(i) + "_0"));
            // filenameIn_B.insert(filenameIn_B.length()-4, ("_"+std::to_string(i)+"_0"));
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i)));
            // plio objects
            inA[i] = input_plio::create("PLIO_in_A" + std::to_string(i), adf::plio_64_bits, filenameIn_A);
            inB[i] = input_plio::create("PLIO_in_B" + std::to_string(i), adf::plio_64_bits, filenameIn_B);
            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_64_bits, filenameOut);
            // connections
            connect<>(inA[i].out[0], kroneckerGraph.inA[i]);
            connect<>(inB[i].out[0], kroneckerGraph.inB[i]);
            connect<>(kroneckerGraph.out[i], out[i].in[0]);
        }
#else  // using ref model
        // files names
        std::string filenameIn_A = QUOTE(INPUT_FILE_A);
        std::string filenameIn_B = QUOTE(INPUT_FILE_B);
        std::string filenameOut = QUOTE(OUTPUT_FILE);
        // plio objects
        inA[0] = input_plio::create("PLIO_in_A" + std::to_string(0), adf::plio_64_bits, filenameIn_A);
        inB[0] = input_plio::create("PLIO_in_B" + std::to_string(0), adf::plio_64_bits, filenameIn_B);
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        // connections
        connect<>(inA[0].out[0], kroneckerGraph.inA[0]);
        connect<>(inB[0].out[0], kroneckerGraph.inB[0]);
        connect<>(kroneckerGraph.out[0], out[0].in[0]);
#endif // USING_UUT
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
