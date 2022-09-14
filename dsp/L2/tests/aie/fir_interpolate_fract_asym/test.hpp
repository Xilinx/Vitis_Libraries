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
#include "test_utils.hpp"

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
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in;
    std::array<output_plio, P_SSR * NUM_OUTPUTS> out;
#if (USE_COEFF_RELOAD == 1) // Single kernel, reloadable coefficients
    port<input> coeff;
#endif

    COEFF_TYPE m_taps[2][FIR_LEN];
    std::vector<COEFF_TYPE> m_taps_v;

    // Constructor
    test_graph() {
        printConfig();

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
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

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
