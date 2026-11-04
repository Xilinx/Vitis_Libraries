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

// This file holds the definition of the test harness for the transpose graph.

#include <adf.h>
#include <array>
#include <string>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"
#include "device_defs.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH transpose_graph
#endif

#ifdef USING_UUT
#include "transpose_native_generated_graph/transpose_generated_graph.h"
#else
#include QUOTE(UUT_GRAPH.hpp)
#endif

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

namespace dsplib = xf::dsp::aie;

#ifndef UUT_SSR
#define UUT_SSR 1
#endif

class test_graph : public graph {
   private:
   public:
    static constexpr int kNumKernels = UUT_SSR * CASC_LEN;
    std::array<input_plio, kNumKernels> in;
    std::array<output_plio, kNumKernels> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== transpose test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("T_DATA            = ");
        printf(QUOTE(T_DATA));
        printf("\n");
        printf("DIM_ROWS          = %d \n", DIM_ROWS);
        printf("DIM_COLS          = %d \n", DIM_COLS);
        printf("UUT_SSR           = %d \n", UUT_SSR);
        printf("CASC_LEN          = %d \n", CASC_LEN);
        printf("SPLIT_MODE        = %d \n", SPLIT_MODE);
        printf("CONJUGATE         = %d \n", CONJUGATE);
        printf("AIE_VARIANT       = %d \n", AIE_VARIANT);
        printf("========================\n");

// transpose graph
#ifdef USING_UUT
        transpose_native_generated_graph transposeGraph;
#else
        dsplib::transpose::UUT_GRAPH<T_DATA, DIM_ROWS, DIM_COLS, UUT_SSR, CASC_LEN, SPLIT_MODE, CONJUGATE>
            transposeGraph;
#endif

#ifdef USING_UUT
        for (int ssrIdx = 0; ssrIdx < UUT_SSR; ssrIdx++) {
            for (int cascIdx = 0; cascIdx < CASC_LEN; cascIdx++) {
                int kp = ssrIdx * CASC_LEN + cascIdx;
                // file names - one padded, per-kernel file per port, matching
                // transpose_partition_pad.pl's <name>_<ssrIdx>_<cascIdx><ext> naming convention.
                std::string filenameIn = QUOTE(INPUT_FILE);
                std::string filenameOut = QUOTE(OUTPUT_FILE);
                filenameIn.insert(filenameIn.length() - 4,
                                  ("_" + std::to_string(ssrIdx) + "_" + std::to_string(cascIdx)));
                filenameOut.insert(filenameOut.length() - 4,
                                   ("_" + std::to_string(ssrIdx) + "_" + std::to_string(cascIdx)));
                // plio objects
                in[kp] = input_plio::create("PLIO_in_" + std::to_string(kp), adf::plio_64_bits, filenameIn);
                out[kp] = output_plio::create("PLIO_out_" + std::to_string(kp), adf::plio_64_bits, filenameOut);
                // connections
                connect<>(in[kp].out[0], transposeGraph.in[kp]);
                connect<>(transposeGraph.out[kp], out[kp].in[0]);
#if (SINGLE_BUF == 1)
                single_buffer(transposeGraph.transposeGraph.getKernels()[kp].in[0]);
                single_buffer(transposeGraph.transposeGraph.getKernels()[kp].out[0]);
                printf("INFO: Single Buffer Constraint applied to kernel (%d,%d).\n", ssrIdx, cascIdx);
#endif
            }
        }
#else  // using ref model - single port, full unpadded matrix
        std::string filenameIn = QUOTE(INPUT_FILE);
        std::string filenameOut = QUOTE(OUTPUT_FILE);
        in[0] = input_plio::create("PLIO_in_0", adf::plio_64_bits, filenameIn);
        out[0] = output_plio::create("PLIO_out_0", adf::plio_64_bits, filenameOut);
        connect<>(in[0].out[0], transposeGraph.in[0]);
        connect<>(transposeGraph.out[0], out[0].in[0]);
#endif // USING_UUT
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
