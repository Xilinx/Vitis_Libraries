/*
 * Copyright 2022 Xilinx, Inc.
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

/* This file holds the definition of the test harness for the Single Rate
   DDS  reference model graph.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH dds_mixer_graph
#endif
// Force reference model to ignore SSR and only use array length 1
#ifdef USING_UUT
#define P_SSR UUT_SSR
#else
#define P_SSR 1
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
    std::array<input_plio, P_SSR> in1;
    std::array<input_plio, P_SSR> in2;
    std::array<output_plio, P_SSR> out;

    static constexpr unsigned int phaseInc = DDS_PHASE_INC; // single sample phase increment
    static constexpr unsigned int initialPhaseOffset = INITIAL_DDS_OFFSET;
    // Constructor
    test_graph() {
        printf("========================\n");
        printf("TEST.HPP STARTED\n");
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples   = %d \n", INPUT_SAMPLES);
        printf("Output samples  = %d \n", OUTPUT_SAMPLES);
        printf("Data type       = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");

        printf("Mixer Mode      = %d \n", MIXER_MODE);
        if (MIXER_MODE == 0) printf(" ( DDS Only Mode ) \n");
        if (MIXER_MODE == 1) printf(" ( DDS Plus Mixer (1 data input) Mode ) \n");
        if (MIXER_MODE == 2) printf(" ( DDS Plus Symmetrical Mixer (2 data input) Mode ) \n");

        printf("phaseInc is %u or 0x%08X \n", phaseInc, phaseInc);
        printf("initialPhaseOffset is %u or 0x%08X \n", initialPhaseOffset, initialPhaseOffset);

        namespace dsplib = xf::dsp::aie;
        dsplib::mixer::dds_mixer::UUT_GRAPH<DATA_TYPE, INPUT_WINDOW_VSIZE, MIXER_MODE, P_API, P_SSR> ddsGraph(
            phaseInc, initialPhaseOffset);

        for (unsigned int i = 0; i < P_SSR; ++i) {
            std::string filenameOut = QUOTE(OUTPUT_FILE);
            std::string filenameIn1 = QUOTE(INPUT_FILE);
            std::string filenameIn2 = QUOTE(INPUT_FILE2);
// Reference model doesn't need SSR index
#ifdef USING_UUT
            // Insert SSR index into filename before extension (.txt)
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i) + "_0"));
            filenameIn1.insert(filenameIn1.length() - 4, ("_" + std::to_string(i) + "_0"));
            filenameIn2.insert(filenameIn2.length() - 4, ("_" + std::to_string(i) + "_0"));
#endif

#if (MIXER_MODE == 2 || MIXER_MODE == 1)
            in1[i] = input_plio::create("PLIO_in1_" + std::to_string(i), adf::plio_32_bits, filenameIn1);
            connect<>(in1[i].out[0], ddsGraph.in1[i]);
            printf("Connecting ddsGraph.in1[%d] to %s\n", i, filenameIn1.c_str());
#endif
#if (MIXER_MODE == 2)
            in2[i] = input_plio::create("PLIO_in2_" + std::to_string(i), adf::plio_32_bits, filenameIn2);
            connect<>(in2[i].out[0], ddsGraph.in2[i]);
            printf("Connecting ddsGraph.in2[%d] to %s\n", i, filenameIn2.c_str());
#endif

            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_32_bits, filenameOut);
            connect<>(ddsGraph.out[i], out[i].in[0]);
            printf("Connecting ddsGraph.out[%d] to %s\n", i, filenameOut.c_str());
        }

        printf("TEST.HPP COMPLETED\n");
        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
