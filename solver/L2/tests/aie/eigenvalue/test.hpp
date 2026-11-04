/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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
Test harness graph for the eigenvalue IP core.

USING_UUT=1 (default): instantiates the production eigenvalue_graph using
  the Hessenberg-reduction + shifted/deflated iterative-QR pipeline.

USING_UUT=0: instantiates the reference eigenvalue_ref_graph using a plain
  unshifted QR-algorithm iteration for golden comparison.

Both share the same single-input / single-output PLIO interface.
*/

#include <adf.h>
#include <string>
#include "uut_config.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH eigenvalue_graph
#endif

#if defined(USING_UUT) && USING_UUT == 1
#include "eigenvalue_native_generated_graph/eigenvalue_generated_graph.h"
#else
#include QUOTE(UUT_GRAPH.hpp)
#endif

#ifndef USING_UUT
#define USING_UUT 1
#endif

using namespace adf;

namespace xf {
namespace solver {
namespace aie {
namespace testcase {

class test_graph : public graph {
   public:
    input_plio plio_in;
    output_plio plio_out;

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
        printf("DIM_SIZE              = %d\n", DIM_SIZE);
        printf("PASSES                = %d\n", PASSES);
        printf("CASC_LEN              = %d\n", CASC_LEN);
        printf("NITER                 = %d\n", NITER);
        printf("========================\n");

#if USING_UUT == 1
        // ----------------------------------------------------------------
        // Production UUT: Hessenberg + shifted/deflated QR eigenvalue graph.
        // Template parameters: TT_DATA, TP_DIM, TP_PASSES, TP_CASC_LEN
        // ----------------------------------------------------------------
        eigenvalue_native_generated_graph dut_graph;

        plio_in = input_plio::create("PLIO_in_0", adf::plio_64_bits, QUOTE(INPUT_FILE));
        connect<>(plio_in.out[0], dut_graph.in);

        // Suffix the output file with a "_0_0" kernel/port index, matching
        // the naming used by the multi-kernel cores (cholesky, qrd) even
        // though this core has only one output today -- helper.mk's
        // prep_aie_out/prep_x86_out stitch uut_output_0_0.txt (...through
        // N in the future) back into the plain uut_output.txt used for diff.
        std::string filenameOut = QUOTE(OUTPUT_FILE);
        filenameOut.insert(filenameOut.length() - 4, "_0_0");
        plio_out = output_plio::create("PLIO_out_0", adf::plio_64_bits, filenameOut);
        connect<>(dut_graph.out, plio_out.in[0]);

#else // USING_UUT == 0: reference model

        // ----------------------------------------------------------------
        // Reference: plain unshifted QR-algorithm iteration.
        // Template parameters: TT_DATA, TP_DIM
        // ----------------------------------------------------------------
        xf::solver::aie::eigenvalue::UUT_GRAPH<DATA_TYPE, DIM_SIZE> dut_graph;

        plio_in = input_plio::create("PLIO_in_0", adf::plio_64_bits, QUOTE(INPUT_FILE));
        connect<>(plio_in.out[0], dut_graph.in);

        plio_out = output_plio::create("PLIO_out_0", adf::plio_64_bits, QUOTE(OUTPUT_FILE));
        connect<>(dut_graph.out, plio_out.in[0]);

#endif // USING_UUT
    };
};

} // namespace testcase
} // namespace aie
} // namespace solver
} // namespace xf

#endif // _DSPLIB_TEST_HPP_
