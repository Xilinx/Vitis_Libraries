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

/*
This file holds the declaration of the test harness graph class for the
svd graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH svd_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace testcase {

class test_graph : public graph {
   public:
    std::array<input_plio, CASC_LEN> in;
    std::array<output_plio, CASC_LEN> outU;
    std::array<output_plio, CASC_LEN> outS;
    std::array<output_plio, CASC_LEN> outV;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type             = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("Dimension (rows)            = %d \n", DIM_ROWS);
        printf("Dimension (columns)            = %d \n", DIM_COLS);
        printf("Passes            = %d \n", PASSES);
        printf("Cascade len           = %d \n", CASC_LEN);
        printf("Number of inputs      = %d \n", CASC_LEN);
        printf("Number of outputs     = %d (U/S/V x CASC_LEN) \n", 3 * CASC_LEN);

        printf("PARAMETERS OF TEST:\n-------------------\n");
        printf("STIM_TYPE            = %d \n", STIM_TYPE);
        printf("NITER                = %d \n", NITER);

        printf("========================\n");

        xf::solver::aie::svd::UUT_GRAPH<DATA_TYPE, DIM_ROWS, DIM_COLS, PASSES, CASC_LEN> svd_graph;

        for (int i = 0; i < CASC_LEN; i++) {
            std::string filenameIn = QUOTE(INPUT_FILE);
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(i) + "_0"));
            in[i] = input_plio::create("PLIO_in_" + std::to_string(i), adf::plio_64_bits, filenameIn);
            connect<>(in[i].out[0], svd_graph.in[i]);

            std::string filenameOutU = QUOTE(OUTPUT_FILE_U);
            std::string filenameOutS = QUOTE(OUTPUT_FILE_S);
            std::string filenameOutV = QUOTE(OUTPUT_FILE_V);
            filenameOutU.insert(filenameOutU.length() - 4, ("_0_" + std::to_string(i)));
            filenameOutS.insert(filenameOutS.length() - 4, ("_0_" + std::to_string(i)));
            filenameOutV.insert(filenameOutV.length() - 4, ("_0_" + std::to_string(i)));
            outU[i] = output_plio::create("PLIO_outU_" + std::to_string(i), adf::plio_64_bits, filenameOutU);
            outS[i] = output_plio::create("PLIO_outS_" + std::to_string(i), adf::plio_64_bits, filenameOutS);
            outV[i] = output_plio::create("PLIO_outV_" + std::to_string(i), adf::plio_64_bits, filenameOutV);
            connect<>(svd_graph.outU[i], outU[i].in[0]);
            connect<>(svd_graph.outS[i], outS[i].in[0]);
            connect<>(svd_graph.outV[i], outV[i].in[0]);
        }
    };
};
} // namespace testcase
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _DSPLIB_TEST_HPP_
