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

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_utils.hpp"
#include "fir_common_traits.hpp"
#include "test_stim.hpp"
#include "device_defs.h"

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_decimate_sym_graph
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
    static constexpr unsigned int kNumTaps = (FIR_LEN + 1) / 2;

    COEFF_TYPE taps[kNumTaps];

   public:
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in;
    std::array<output_plio, P_SSR * NUM_OUTPUTS> out;

#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeff;
#endif

    COEFF_TYPE m_taps[2][kNumTaps];
    std::vector<COEFF_TYPE> m_taps_v;

    template <unsigned int windowVSize = INPUT_SAMPLES>
    using uut_g = dsplib::fir::decimate_sym::UUT_GRAPH<DATA_TYPE,
                                                       COEFF_TYPE,
                                                       FIR_LEN,
                                                       DECIMATE_FACTOR,
                                                       SHIFT,
                                                       ROUND_MODE,
                                                       windowVSize,
                                                       CASC_LEN,
                                                       DUAL_IP,
                                                       USE_COEFF_RELOAD,
                                                       NUM_OUTPUTS,
                                                       PORT_API,
                                                       P_SSR,
                                                       SAT_MODE>;

    // Constructor
    test_graph() {
        printConfig();

        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        test_stim<COEFF_TYPE, kNumTaps, 0> taps_gen(QUOTE(COEFF_FILE));
        srand(115552);
        int error_tap =
            rand() %
            kNumTaps; // Randomly selects a single coefficient to be changed in second coefficient array to test reload
#ifdef _DSPLIB_FIR_DEBUG_ADL_
        error_tap = kNumTaps - 1; // Always overwrite the last coeff only.
#endif                            // _DSPLIB_FIR_DEBUG_ADL_
        for (int j = 0; j < 2; j++) {
            taps_gen.prepSeed(COEFF_SEED);
            taps_gen.gen(COEFF_STIM_TYPE, taps);
            for (int i = 0; i < kNumTaps; i++) {
                m_taps[j][i] = taps[i];
                if (i == error_tap && j == 1) {
                    m_taps[j][i] = addError(m_taps[j][i]);
                }
            }
        }
        // Copy taps from C++ array into std::vector
        for (int i = 0; i < kNumTaps; i++) {
            m_taps_v.push_back(m_taps[0][i]);
        }
#ifdef USING_UUT
        static constexpr int kMinLen =
            uut_g<INPUT_SAMPLES>::getMinCascLen<FIR_LEN, DATA_TYPE, COEFF_TYPE, DECIMATE_FACTOR, PORT_API, P_SSR>();
        printf("For this config the Minimum CASC_LEN is %d\n", kMinLen);
#if (PORT_API == 1)
        static constexpr int kOptLen =
            uut_g<INPUT_SAMPLES>::getOptCascLen<FIR_LEN, DATA_TYPE, COEFF_TYPE, PORT_API, DECIMATE_FACTOR, P_SSR>();
        printf("For this config the Optimal CASC_LEN is %d\n", kOptLen);
#endif
#endif

// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        static_assert(NITER % 2 == 0,
                      "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
        uut_g<INPUT_SAMPLES> firGraph;
#else // Static coefficients
        uut_g<INPUT_SAMPLES> firGraph(m_taps_v);
#endif
#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1 && \
     USE_COEFF_RELOAD == 1) // temporarily disable chained connection for configs with RTPs
// Chained connections mutually explusive with multiple outputs.
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        uut_g<INPUT_SAMPLES / DECIMATE_FACTOR> firGraph2;
#else // Multi-kernel, static coefficients
        uut_g<INPUT_SAMPLES / DECIMATE_FACTOR> firGraph2(m_taps_v);
#endif
#endif

        // Make connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

        for (unsigned int i = 0; i < P_SSR; ++i) {
            unsigned int plioIpBaseIdx = i * (DUAL_INPUT_SAMPLES + 1);
            unsigned int plioOpBaseIdx = i * NUM_OUTPUTS;
            // Size of window in Bytes.
            connect<>(in[plioIpBaseIdx].out[0], firGraph.in[i]);
#if (DUAL_IP == 1)
            if (PORT_API == 0) {                                      // Window
                connect<>(in[plioIpBaseIdx].out[0], firGraph.in2[i]); // clone/broadcast input
            } else {
                connect<>(in[plioIpBaseIdx + 1].out[0],
                          firGraph.in2[i]); //'dual' streams carry independent data which when combined are one channel
            }
#endif
#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1 && \
     USE_COEFF_RELOAD == 1) // temporarily disable chained connection for configs with RTPs
            connect<>(firGraph.out[i], firGraph2.in[i]);
            connect<>(firGraph2.out[i], out[plioBaseIdx].in[0]);
#else
            connect<>(firGraph.out[i], out[plioOpBaseIdx].in[0]);
#if (NUM_OUTPUTS == 2)
            connect<>(firGraph.out2[i], out[plioOpBaseIdx + 1].in[0]);
#endif
#endif
        }
#if (USE_COEFF_RELOAD == 1)
        for (int i = 0; i < P_SSR; i++) {
            connect<>(coeff[i], firGraph.coeff[i]);
        }
#endif

#ifdef USING_UUT
        const int MAX_PING_PONG_SIZE = __DATA_MEM_BYTES__ / 2;
#if (P_SSR == 1)
        const int bufferSize = ((FIR_LEN + INPUT_SAMPLES) * sizeof(DATA_TYPE));
        if ((bufferSize > MAX_PING_PONG_SIZE && PORT_API == 0) || (SINGLE_BUF == 1 && PORT_API == 0)) {
            single_buffer(firGraph.getKernels()[0].in[0]);

            if (DUAL_IP == 1) {
                single_buffer(firGraph.getKernels()[0].in[1]);
            }
            printf("INFO: Single Buffer Constraint applied to input buffers of kernel 0.\n");

            single_buffer(firGraph.getKernels()[CASC_LEN - 1].out[0]);
            if (NUM_OUTPUTS == 2) {
                single_buffer(firGraph.getKernels()[CASC_LEN - 1].out[1]);
            }
            printf("INFO: Single Buffer Constraint applied to output buffers of kernel %d.\n", CASC_LEN - 1);
        }
#endif
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
        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
