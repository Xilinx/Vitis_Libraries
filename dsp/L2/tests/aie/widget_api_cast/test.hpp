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

// This file holds the definition of the test harness for the Widget API Cast graph.

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH widget_api_cast_graph
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
    std::array<input_plio, NUM_INPUTS> in;
    std::array<output_plio, NUM_OUTPUT_CLONES> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== Widget test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type         = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        switch (IN_API) {
            case 0:
                printf("Input API           = window\n");
                break;
            case 1:
                printf("Input API           = stream\n");
                break;
            default:
                printf("Input API unrecognised = %d\n", IN_API);
                break;
        };
        switch (OUT_API) {
            case 0:
                printf("Output API          = window\n");
                break;
            case 1:
                printf("Output API          = stream\n");
                break;
            default:
                printf("Output API unrecognised = %d\n", OUT_API);
                break;
        };
        printf("NUM_INPUTS        = %d \n", NUM_INPUTS);
        printf("WINDOW_VSIZE      = %d \n", WINDOW_VSIZE);
        printf("NUM_OUTPUT_CLONES = %d \n", NUM_OUTPUT_CLONES);
        printf("PATTERN           = %d \n", PATTERN);
        printf("HEADER_BYTES      = %d \n", HEADER_BYTES);
        namespace dsplib = xf::dsp::aie;

        // Widget sub-graph
        dsplib::widget::api_cast::UUT_GRAPH<DATA_TYPE, IN_API, OUT_API, NUM_INPUTS, WINDOW_VSIZE, NUM_OUTPUT_CLONES,
                                            PATTERN, HEADER_BYTES>
            widgetGraph;

        // Make connections
        for (int i = 0; i < NUM_INPUTS; i++) {
            std::string filenameIn = QUOTE(INPUT_FILE);
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(i) + "_0"));
            in[i] = input_plio::create("PLIO_in_" + std::to_string(i), adf::plio_64_bits, filenameIn);
            connect<>(in[i].out[0], widgetGraph.in[i]);
        }
        for (int i = 0; i < NUM_OUTPUT_CLONES; i++) {
            std::string filenameOut = QUOTE(OUTPUT_FILE);
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i) + "_0"));
            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_64_bits, filenameOut);
            connect<>(widgetGraph.out[i], out[i].in[0]);
        }

#ifdef USING_UUT
        // For cases which use 3 or 4 output windows, contention and poor QoR is seen if the processor is at the edge of
        // the array since
        // the windows then share banks and contention occurs.
        location<kernel>(*widgetGraph.getKernels()) = tile(1, 1);
#endif
    };
};
}
}
}
}; // end of namespace

#endif // _DSPLIB_TEST_HPP_
