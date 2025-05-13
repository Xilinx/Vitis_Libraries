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

/* This file holds the definition of the test harness for the Single Rate
   DDS  reference model graph.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"
#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"
#include "graph_utils.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH dds_mixer_graph
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
    std::array<input_plio, 1> inPO; // broadcast, for now
    std::array<output_plio, P_SSR> out;

    port_conditional_array<input, (USE_PHASE_RELOAD == 1 && PHASE_RELOAD_API == USE_PHASE_RELOAD_API_RTP), P_SSR>
        PhaseRTP;
    port_conditional_array<input, (USE_PHASE_INC_RELOAD == 1), P_SSR> PhaseIncRTP;

    static constexpr unsigned int phaseInc = DDS_PHASE_INC; // single sample phase increment
    static constexpr unsigned int initialPhaseOffset = INITIAL_DDS_OFFSET;
    static constexpr std::array<unsigned int, 32> PhaseRTP_vec = {
        134217728, 268435456, 0, 536870912,  1235645898, 562455525, 0, 2235645898, 0,         125869457, 0, 2546963257,
        759812036, 125525694, 0, 2235645898, 20312534,   12896458,  0, 145571369,  458589620, 369896569, 0, 1569478023,
        125985669, 0,         0, 225583741,  78999580,   11253303,  0, 42558996};
    static constexpr std::array<unsigned int, 32> PhaseIncRTP_vec = {
        1235645898, 562455525, 252102568, 258985748, 0, 1288589647, 0, 2235645898, 0,         125869457, 0, 2546963257,
        20312534,   12896458,  759812036, 125525694, 0, 2235645898, 0, 145571369,  458589620, 369896569, 0, 1569478023,
        78999580,   11253303,  125985669, 0,         0, 225583741,  0, 42558996};

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
        printf("MIXER_MODE                  = %d \n", MIXER_MODE);
        printf("USE_PHASE_RELOAD            = %d \n", USE_PHASE_RELOAD);
        printf("PHASE_RELOAD_API            = %d \n", PHASE_RELOAD_API);
        printf("USE_PHASE_INC_RELOAD        = %d \n", USE_PHASE_INC_RELOAD);
        //        printf("SFDR                        = %d \n", SFDR);
        printf("P_API                       = %d \n", P_API);
        printf("INPUT_WINDOW_VSIZE          = %d \n", INPUT_WINDOW_VSIZE);
        printf("P_SSR                       = %d \n", P_SSR);
        printf("ROUND_MODE                  = %d \n", ROUND_MODE);
        printf("SAT_MODE                    = %d \n", SAT_MODE);
        printf("Mixer Mode      = %d \n", MIXER_MODE);
        if (MIXER_MODE == 0) printf(" ( DDS Only Mode ) \n");
        if (MIXER_MODE == 1) printf(" ( DDS Plus Mixer (1 data input) Mode ) \n");
        if (MIXER_MODE == 2) printf(" ( DDS Plus Symmetrical Mixer (2 data input) Mode ) \n");

        printf("phaseInc is %u or 0x%08X \n", phaseInc, phaseInc);
        printf("initialPhaseOffset is %u or 0x%08X \n", initialPhaseOffset, initialPhaseOffset);

        namespace dsplib = xf::dsp::aie;
        dsplib::mixer::dds_mixer::UUT_GRAPH<DATA_TYPE, INPUT_WINDOW_VSIZE, MIXER_MODE, P_API, P_SSR, ROUND_MODE,
                                            SAT_MODE, USE_PHASE_RELOAD, PHASE_RELOAD_API, USE_PHASE_INC_RELOAD>
            ddsGraph(phaseInc, initialPhaseOffset);

#if (USE_PHASE_RELOAD == 1 && PHASE_RELOAD_API == USE_PHASE_RELOAD_API_IOBUFF)
        std::string filenameInPO = QUOTE(INPUT_FILE_PO);
        // filenameInPO.insert(filenameInPO.length() - 4, ("_" + std::to_string(0) + "_0"));//0 becomes i if SSR'd
        inPO[0] = input_plio::create("PLIO_inPO_" + std::to_string(0), adf::plio_64_bits,
                                     filenameInPO); // 0 becomes i if SSR'd
#endif

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
#if (USE_PHASE_RELOAD == 1)
#if (PHASE_RELOAD_API == USE_PHASE_RELOAD_API_RTP)
            connect<parameter>(PhaseRTP[i], ddsGraph.PhaseRTP[i]);
#else
            connect<>(
                inPO[0].out[0],
                ddsGraph
                    .PhaseRTP[i]); // 0 becomes i if SSR'd - broadcast from single PLIO for update to all SSR kernels
#endif
#endif
#if (MIXER_MODE == 2 || MIXER_MODE == 1)
            in1[i] = input_plio::create("PLIO_in1_" + std::to_string(i), adf::plio_64_bits, filenameIn1);
            connect<>(in1[i].out[0], ddsGraph.in1[i]);
            printf("Connecting ddsGraph.in1[%d] to %s\n", i, filenameIn1.c_str());
#endif
#if (MIXER_MODE == 2)
            in2[i] = input_plio::create("PLIO_in2_" + std::to_string(i), adf::plio_64_bits, filenameIn2);
            connect<>(in2[i].out[0], ddsGraph.in2[i]);
            printf("Connecting ddsGraph.in2[%d] to %s\n", i, filenameIn2.c_str());
#endif

            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_64_bits, filenameOut);
            connect<>(ddsGraph.out[i], out[i].in[0]);
            printf("Connecting ddsGraph.out[%d] to %s\n", i, filenameOut.c_str());

#if (USE_PHASE_INC_RELOAD == 1)
            connect<parameter>(PhaseIncRTP[i], ddsGraph.PhaseIncRTP[i]);
#endif

#ifdef USING_UUT

#if (SINGLE_BUF == 1 && P_API == 0) // Single buffer constraint applies for windows implementations

#if (MIXER_MODE == 2 || MIXER_MODE == 1)
            single_buffer(ddsGraph.getKernels()[i].in[0]);
            printf("INFO: Single Buffer Constraint applied to the input buffer-1 of kernel %d.\n", i);
#endif

#if (MIXER_MODE == 2)
            single_buffer(ddsGraph.getKernels()[i].in[1]);
            printf("INFO: Single Buffer Constraint applied to the input buffer-2 of kernel %d.\n", i);
#endif

            single_buffer(ddsGraph.getKernels()[i].out[0]);
            printf("INFO: Single Buffer Constraint applied to the output buffer of kernel %d.\n", i);
#endif
#endif
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
