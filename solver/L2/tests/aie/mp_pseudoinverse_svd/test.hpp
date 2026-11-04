/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
Test harness graph for the mp_pseudoinverse_svd IP core.

USING_UUT=1 (default): instantiates the production mp_pseudoinverse_svd_graph
  using the SVD + V*S^+*U^H computation pipeline.

USING_UUT=0: instantiates the reference mp_pseudoinverse_svd_ref_graph
  using a scalar Gauss-Jordan (A^H*A)^-1*A^H computation for golden comparison.

Both share the same single-input / single-output PLIO interface.
*/

#include <adf.h>
#include "uut_config.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH mp_pseudoinverse_svd_graph
#endif
#ifndef PASSES_SSR
#define PASSES_SSR 1
#endif

#if defined(USING_UUT) && USING_UUT == 1
#include "mp_pseudoinverse_svd_native_generated_graph/mp_pseudoinverse_svd_generated_graph.h"
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
        printf("DIM_ROWS              = %d\n", DIM_ROWS);
        printf("DIM_COLS              = %d\n", DIM_COLS);
        printf("PASSES                = %d\n", PASSES);
        printf("PASSES_SSR            = %d\n", PASSES_SSR);
        printf("CASC_LEN              = %d\n", CASC_LEN);
        printf("NITER                 = %d\n", NITER);
        printf("========================\n");

#if USING_UUT == 1
        // ----------------------------------------------------------------
        // Production UUT: SVD-based pseudoinverse graph.
        // Template parameters: TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_PASSES, TP_CASC_LEN, TP_PASSES_SSR
        // ----------------------------------------------------------------
        mp_pseudoinverse_svd_native_generated_graph dut_graph;

        plio_in = input_plio::create("PLIO_in_0", adf::plio_64_bits, QUOTE(INPUT_FILE));
        connect<>(plio_in.out[0], dut_graph.in);

        plio_out = output_plio::create("PLIO_out_0", adf::plio_64_bits, QUOTE(OUTPUT_FILE));
        connect<>(dut_graph.out, plio_out.in[0]);

#else // USING_UUT == 0: reference model

        // ----------------------------------------------------------------
        // Reference: scalar Gauss-Jordan pseudoinverse.
        // Template parameters: TT_DATA, TP_DIM_ROWS, TP_DIM_COLS
        // ----------------------------------------------------------------
        xf::solver::aie::mp_pseudoinverse_svd::UUT_GRAPH<DATA_TYPE, DIM_ROWS, DIM_COLS> dut_graph;

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
