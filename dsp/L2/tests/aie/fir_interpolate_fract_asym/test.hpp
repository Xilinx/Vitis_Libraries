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

/*
This file holds the declaraion of the test harness graph class for the
fir_interpolate_fract_asym graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_interpolate_fract_asym_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
namespace dsplib = xf::dsp::aie;

template <unsigned int ssr, unsigned int dual, typename plioType>
void createPLIOFileConnections(std::array<plioType, ssr*(dual + 1)>& plioPorts, std::string filename) {
    for (unsigned int ssrIdx = 0; ssrIdx < ssr; ++ssrIdx) {
        for (unsigned int dualIdx = 0; dualIdx < (dual + 1); ++dualIdx) {
            std::string filenameInternal = filename;

#if (NUM_OUTPUTS == 2 && PORT_API == 0)
            if (dual == 1 && dualIdx == 1) {
                filenameInternal.insert(filenameInternal.length() - 4, ("_clone"));
            } else {
#ifdef USING_UUT
                // Insert SSR index and dual stream index into filename before extension (.txt)
                filenameInternal.insert(filenameInternal.length() - 4,
                                        ("_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx)));
#endif
            }
#elif defined(USING_UUT)
            // Insert SSR index and dual stream index into filename before extension (.txt)
            filenameInternal.insert(filenameInternal.length() - 4,
                                    ("_" + std::to_string(ssrIdx) + "_" + std::to_string(dualIdx)));
#endif
            plioPorts[ssrIdx * (dual + 1) + dualIdx] =
                plioType::create("PLIO_" + filenameInternal, adf::plio_32_bits, filenameInternal);
        }
    }
}

class test_graph : public graph {
   private:
    // The taps array holds the coefficient values to be passed as input to UUT
    COEFF_TYPE taps[FIR_LEN];

   public:
#ifndef P_SSR
    static constexpr int P_SSR = 1; // until SSR is supported
#endif
#ifdef USING_UUT
    static constexpr int DUAL_INPUT_SAMPLES = (PORT_API == 1) && (DUAL_IP == 1) ? 1 : 0;
#else
    static constexpr int DUAL_INPUT_SAMPLES = 0;
#endif
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in; // 0? dual_ip - not supported by sr_sym
    std::array<output_plio, P_SSR * NUM_OUTPUTS> out;          // NUM_OUTPUTS forces to 1 for ref
#if (USE_COEFF_RELOAD == 1)                                    // Single kernel, reloadable coefficients
    port<input> coeff;
#endif

    COEFF_TYPE m_taps[2][FIR_LEN];
    std::vector<COEFF_TYPE> m_taps_v;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Input samples   = %d \n", INPUT_SAMPLES);
        printf("Input window [B]= %lu \n", INPUT_SAMPLES * sizeof(DATA_TYPE));
        printf("Input margin    = %lu \n", INPUT_MARGIN(FIR_LEN, DATA_TYPE));
        printf("Output samples  = %d \n", OUTPUT_SAMPLES);
        printf("FIR Length      = %d \n", FIR_LEN);
        printf("Shift           = %d \n", SHIFT);
        printf("ROUND_MODE      = %d \n", ROUND_MODE);
        printf("INTERPOLATE_FACTOR = %d \n", INTERPOLATE_FACTOR);
        printf("DECIMATE_FACTOR = %d \n", DECIMATE_FACTOR);
        printf("USE_COEFF_RELOAD = %d \n", USE_COEFF_RELOAD);
        printf("CASC_LEN        = %d \n", CASC_LEN);
        printf("NUM_OUTPUTS     = %d \n", NUM_OUTPUTS);
        printf("Data type       = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("Coeff type      = ");
        printf(QUOTE(COEFF_TYPE));
        printf("\n");
        namespace dsplib = xf::dsp::aie;
        printf("========================\n");
        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        // COEFF_FILE comes from pre processor args.
        test_stim<COEFF_TYPE, FIR_LEN, 0> taps_gen(QUOTE(COEFF_FILE));
        srand(115552);
        int error_tap =
            rand() %
            FIR_LEN; // Randomly selects a single coefficient to be changed in second coefficient array to test reload
#ifdef _DSPLIB_FIR_DEBUG_ADL_
        error_tap = FIR_LEN - 1; // Always overwrite the last coeff only.
#endif                           // _DSPLIB_FIR_DEBUG_ADL_
        for (int j = 0; j < 2; j++) {
            taps_gen.prepSeed(COEFF_SEED);
            taps_gen.gen(STIM_TYPE, taps);
            for (int i = 0; i < FIR_LEN; i++) {
                m_taps[j][i] = taps[i];
                if (i == error_tap && j == 1) {
                    m_taps[j][i] = addError(m_taps[j][i]);
                }
            }
        }
        // Copy taps from C++ array into std::vector
        for (int i = 0; i < FIR_LEN; i++) {
            m_taps_v.push_back(m_taps[0][i]);
        }

// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        dsplib::fir::interpolate_fract_asym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, INTERPOLATE_FACTOR,
                                                       DECIMATE_FACTOR, SHIFT, ROUND_MODE, INPUT_SAMPLES, CASC_LEN,
                                                       USE_COEFF_RELOAD_TRUE, NUM_OUTPUTS>
            firGraph;
#else // Static coefficients
        dsplib::fir::interpolate_fract_asym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, INTERPOLATE_FACTOR,
                                                       DECIMATE_FACTOR, SHIFT, ROUND_MODE, INPUT_SAMPLES, CASC_LEN,
                                                       USE_COEFF_RELOAD_FALSE, NUM_OUTPUTS>
            firGraph(m_taps_v);
#endif

        // Make connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(
            in, QUOTE(INPUT_FILE)); // fir_sr_sym does not support dual input
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE));

        // Size of window in Bytes.
        connect<>(in[0].out[0], firGraph.in);
#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1)
        // Chained connections mutually explusive with multiple outputs.
        dsplib::fir::interpolate_fract_asym::UUT_GRAPH<DATA_TYPE, COEFF_TYPE, FIR_LEN, INTERPOLATE_FACTOR,
                                                       DECIMATE_FACTOR, SHIFT, ROUND_MODE,
                                                       INPUT_SAMPLES * INTERPOLATE_FACTOR, CASC_LEN>
            firGraph2(m_taps_v);
        connect<>(firGraph.out, firGraph2.in);
        connect<>(firGraph2.out, out[0].in[0]);
#else
        connect<>(firGraph.out, out[0].in[0]);
#if (NUM_OUTPUTS == 2)
        connect<>(firGraph.out2, out[1].in[0]);
#endif
#endif
#if (USE_COEFF_RELOAD == 1)
        connect<>(coeff, firGraph.coeff);
#endif

#ifdef USING_UUT
        // Report out for AIE Synthesizer QoR harvest
        if (&firGraph.getKernels()[0] != NULL) {
            printf("KERNEL_ARCHS: [");
            int arch = firGraph.getKernelArchs();
            printf("%d", arch);
            printf("]\n");
        }
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
