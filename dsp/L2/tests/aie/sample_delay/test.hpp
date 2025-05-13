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

// This file holds the definition of the test harness for the Widget real2complex graph.

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH sample_delay_graph
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
    std::array<input_plio, 1> in;
    std::array<output_plio, 1> out;
    port<input> numSampleDelay_rtp;
    // void testDelay(unsigned int numIter, unsigned int delayVal);

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("DATA_TYPE         = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("WINDOW_VSIZE   = %d \n", WINDOW_VSIZE);
        printf("API            = %d \n", PORT_API);
        printf("MAX_DELAY      = %d \n", MAX_DELAY);
        namespace dsplib = xf::dsp::aie;

        // sample delay sub-graph
        dsplib::sample_delay::UUT_GRAPH<DATA_TYPE, WINDOW_VSIZE, PORT_API, MAX_DELAY> sampleDelayGraph;
        // Make connections
        std::string filenameIn = QUOTE(INPUT_FILE);
        filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(0) + "_0"));
        in[0] = input_plio::create("PLIO_in_" + std::to_string(0), adf::plio_64_bits, filenameIn);
        connect<>(in[0].out[0], sampleDelayGraph.in);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(0) + "_0"));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(sampleDelayGraph.out, out[0].in[0]);
        connect<parameter>(numSampleDelay_rtp, sampleDelayGraph.numSampleDelay); // RTP

#ifdef USING_UUT
#if (SINGLE_BUF == 1 && PORT_API == 0) // Single buffer constraint applies for windows implementations
        single_buffer(sampleDelayGraph.getKernels()[0].in[0]);
        single_buffer(sampleDelayGraph.getKernels()[0].out[0]);
        printf("INFO: Single Buffer Constraint applied to input and output buffers of kernel %d.\n");

#endif
#define CASC_LEN 1
// Report out for AIE Synthesizer QoR harvest
// Nothing to report
#endif
        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
