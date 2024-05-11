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

// This file holds the header for the test harness of the matrix mult graph class

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

// The following macro allows this test harness to be used
// to stimulate the UUT (kernel code for this library element)
// or its reference model by makefile directive.
#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH matrix_mult_graph
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
#ifdef USING_UUT // _PL_MOVER sets P_CASC_LEN=1
    std::array<input_plio, P_CASC_LEN> inA;
    std::array<input_plio, P_CASC_LEN> inB;
#else
    std::array<input_plio, 1> inA;
    std::array<input_plio, 1> inB;
#endif
    std::array<output_plio, 1> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples A   = %d \n", P_INPUT_SAMPLES_A);
        printf("Input window A [B]= %lu \n", P_INPUT_SAMPLES_A * sizeof(T_DATA_A));
        printf("Input samples B   = %d \n", P_INPUT_SAMPLES_B);
        printf("Input window B [B]= %lu \n", P_INPUT_SAMPLES_B * sizeof(T_DATA_B));
        printf("Output samples  = %d \n", P_OUTPUT_SAMPLES);
        printf("Shift           = %d \n", P_SHIFT);
        printf("P_ROUND_MODE      = %d \n", P_ROUND_MODE);
        printf("P_SAT_MODE      = %d \n", P_SAT_MODE);

        printf("Data type       = ");
        printf(QUOTE(T_DATA_A) QUOTE(T_DATA_B));
        printf("\n");
        printf("\n");
        printf("========================\n");

        namespace dsplib = xf::dsp::aie;

        dsplib::blas::matrix_mult::UUT_GRAPH<T_DATA_A, T_DATA_B, P_DIM_A, P_DIM_AB, P_DIM_B, P_SHIFT, P_ROUND_MODE,
                                             P_DIM_A_LEADING, P_DIM_B_LEADING, P_DIM_OUT_LEADING, P_ADD_TILING_A,
                                             P_ADD_TILING_B, P_ADD_DETILING_OUT, P_INPUT_WINDOW_VSIZE_A,
                                             P_INPUT_WINDOW_VSIZE_B, P_CASC_LEN, P_SAT_MODE>
            mmultGraph;

        // Make connections
        std::string filenameA = QUOTE(INPUT_FILE_A);
        std::string filenameB = QUOTE(INPUT_FILE_B);
        std::string filenameOut = QUOTE(OUTPUT_FILE);

#ifdef USING_UUT
#ifdef USING_PL_MOVER // sets P_CASC_LEN=1

        inA[0] = input_plio::create("DataIn1", adf::plio_32_bits, filenameA);
        inB[0] = input_plio::create("DataIn2", adf::plio_32_bits, filenameB);
        connect<>(inA[0].out[0], mmultGraph.inA[0]);
        connect<>(inB[0].out[0], mmultGraph.inB[0]);

#else
        for (int i = 0; i < P_CASC_LEN; i++) {
            filenameA.insert(filenameA.length() - 4, ("_" + std::to_string(i)));
            filenameB.insert(filenameB.length() - 4, ("_" + std::to_string(i)));
            inA[i] = input_plio::create("PLIO_in_A" + std::to_string(i), adf::plio_32_bits, filenameA);
            inB[i] = input_plio::create("PLIO_in_B" + std::to_string(i), adf::plio_32_bits, filenameB);
            connect<>(inA[i].out[0], mmultGraph.inA[i]);
            connect<>(inB[i].out[0], mmultGraph.inB[i]);
        }
#endif
#else // using reference model
        inA[0] = input_plio::create("PLIO_in_A" + std::to_string(0), adf::plio_32_bits, filenameA);
        inB[0] = input_plio::create("PLIO_in_B" + std::to_string(0), adf::plio_32_bits, filenameB);
        connect<>(inA[0].out[0], mmultGraph.inA[0]);
        connect<>(inB[0].out[0], mmultGraph.inB[0]);
#endif

#ifdef USING_PL_MOVER
        out[0] = output_plio::create("DataOut1", adf::plio_32_bits, filenameOut);
#else
        filenameOut.insert(filenameOut.length() - 4, ("_0_0"));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_32_bits, filenameOut);
#endif
        connect<>(mmultGraph.out[0], out[0].in[0]);
    };
};
}
}
}
};
#endif // _DSPLIB_TEST_HPP_
