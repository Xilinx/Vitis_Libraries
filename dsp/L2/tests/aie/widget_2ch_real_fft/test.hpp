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

// This file holds the definition of the test harness for the widget_2ch_real_fft graph.

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
#define UUT_GRAPH widget_2ch_real_fft_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
namespace dsplib = xf::dsp::aie;

class test_graph : public graph {
   public:
    std::array<input_plio, 1> in;
    std::array<output_plio, 1> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== Widget 2-Channel Real FFT test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type         = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("POINT_SIZE        = %d \n", POINT_SIZE);
        printf("WINDOW_VSIZE      = %d \n", WINDOW_VSIZE);
        printf("========================\n");

#ifdef USING_UUT
        // widget_2ch_real_fft_graph sub-graph
        dsplib::widget_2ch_real_fft::UUT_GRAPH<DATA_TYPE, POINT_SIZE, WINDOW_VSIZE> widget_2ch_real_fftGraph;

        // Make connections
        std::string filenameIn = QUOTE(INPUT_FILE);
        in[0] = input_plio::create("PLIO_in_" + std::to_string(0), adf::plio_64_bits, filenameIn);
        connect<>(in[0].out[0], widget_2ch_real_fftGraph.in[0]);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(0)));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(widget_2ch_real_fftGraph.out[0], out[0].in[0]);
#else
        // widget_2ch_real_fft_graph sub-graph
        dsplib::widget_2ch_real_fft::UUT_GRAPH<DATA_TYPE, POINT_SIZE, WINDOW_VSIZE> widget_2ch_real_fftGraph;

        std::string filenameIn = QUOTE(INPUT_FILE);
        in[0] = input_plio::create("PLIO_in_" + std::to_string(0), adf::plio_64_bits, filenameIn);
        connect<>(in[0].out[0], widget_2ch_real_fftGraph.in);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(widget_2ch_real_fftGraph.out, out[0].in[0]);
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_