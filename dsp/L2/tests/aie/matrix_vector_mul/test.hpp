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

/*
This file holds the declaration of the test harness graph class for the
matrix_vector_mul graph class.
*/
#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_static_config.h"
#include "uut_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH matrix_vector_mul_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

class test_graph : public graph {
   private:
   public:
    std::array<input_plio, CASC_LEN * UUT_SSR> inA;
    std::array<input_plio, CASC_LEN * UUT_SSR> inB;
    std::array<output_plio, UUT_SSR> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples A   = %d \n", INPUT_SAMPLES_A);
        printf("Input window A [B]= %lu \n", INPUT_SAMPLES_A * sizeof(DATA_A));
        printf("Input samples B   = %d \n", INPUT_SAMPLES_B);
        printf("Input window B [B]= %lu \n", INPUT_SAMPLES_B * sizeof(DATA_B));
        printf("Output samples  = %d \n", OUTPUT_SAMPLES);
        printf("Shift           = %d \n", SHIFT);
        printf("ROUND_MODE      = %d \n", ROUND_MODE);
        printf("SAT_MODE      = %d \n", SAT_MODE);
        printf("Data type       = ");
        printf(QUOTE(DATA_A) QUOTE(DATA_B));
        printf("\n");
        printf("DIM_A           = %d \n", DIM_A);
        printf("DIM_B      = %d \n", DIM_B);
        printf("\n");
        printf("\n");
        printf("========================\n");

        namespace dsplib = xf::dsp::aie;
        dsplib::blas::matrix_vector_mul::UUT_GRAPH<DATA_A, DATA_B, DIM_A, DIM_B, SHIFT, ROUND_MODE, NUM_FRAMES,
                                                   CASC_LEN, SAT_MODE, UUT_SSR, DIM_A_LEADING>
            matrix_vector_mulGraph;
#ifdef USING_UUT

        for (int ssr = 0; ssr < UUT_SSR; ssr++) {
            for (int casc = 0; casc < CASC_LEN; casc++) {
                std::string filenameInMatrix = QUOTE(INPUT_FILE_A);
                filenameInMatrix.insert(filenameInMatrix.length() - 4,
                                        ("_" + std::to_string(ssr) + "_" + std::to_string(casc)));
                inA[casc + ssr * CASC_LEN] = input_plio::create("PLIO_in_A" + std::to_string(casc + ssr * CASC_LEN),
                                                                adf::plio_64_bits, filenameInMatrix);
                connect<>(inA[casc + ssr * CASC_LEN].out[0], matrix_vector_mulGraph.inA[casc + ssr * CASC_LEN]);

                std::string filenameInVector = QUOTE(INPUT_FILE_B);
                filenameInVector.insert(filenameInVector.length() - 4,
                                        ("_" + std::to_string(ssr)) + "_" + std::to_string(casc));
                inB[casc + ssr * CASC_LEN] = input_plio::create("PLIO_in_B" + std::to_string(casc + ssr * CASC_LEN),
                                                                adf::plio_64_bits, filenameInVector);
                connect<>(inB[casc + ssr * CASC_LEN].out[0], matrix_vector_mulGraph.inB[casc + ssr * CASC_LEN]);
            }
        }
        for (int ssrOut = 0; ssrOut < UUT_SSR; ssrOut++) {
            std::string filenameOut = QUOTE(OUTPUT_FILE);
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(ssrOut)));
            out[ssrOut] = output_plio::create("PLIO_out_" + std::to_string(ssrOut), adf::plio_64_bits, filenameOut);
            connect<>(matrix_vector_mulGraph.out[ssrOut], out[ssrOut].in[0]);
        }

#else // using ref
        std::string filenameInMatrix = QUOTE(INPUT_FILE_A);
        std::string filenameInVector = QUOTE(INPUT_FILE_B);
        // Make connections
        inA[0] = input_plio::create("PLIO_in_A" + std::to_string(0), adf::plio_64_bits, filenameInMatrix);
        inB[0] = input_plio::create("PLIO_in_B" + std::to_string(0), adf::plio_64_bits, filenameInVector);
        connect<>(inA[0].out[0], matrix_vector_mulGraph.inA[0]);
        connect<>(inB[0].out[0], matrix_vector_mulGraph.inB[0]);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        // filenameOut.insert(filenameOut.length()-4, ("_0_0"));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(matrix_vector_mulGraph.out[0], out[0].in[0]);
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
