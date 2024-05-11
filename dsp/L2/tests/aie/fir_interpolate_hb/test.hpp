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

/*
This file holds the declaraion of the test harness graph class for the
Halfband Interpolator FIR graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_utils.hpp"
#include "fir_common_traits.hpp"

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_interpolate_hb_graph
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
    COEFF_TYPE taps[(FIR_LEN + 1) / 4 + 1];

   public:
    static constexpr unsigned int NUM_SSR_OUTPUTS = P_SSR == 1 ? NUM_OUTPUTS : 2 * (NUM_OUTPUTS);

    static constexpr unsigned int SSR_OUT = P_SSR * P_PARA_INTERP_POLY;
    std::array<input_plio, P_SSR*(DUAL_INPUT_SAMPLES + 1)> in;

    std::array<output_plio, SSR_OUT*(NUM_OUTPUTS)> out;

#if (USE_COEFF_RELOAD == 1)
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeff;
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeffCT;
#endif

    COEFF_TYPE m_taps[2][(FIR_LEN + 1) / 4 + 1];
    std::vector<COEFF_TYPE> m_taps_v;

    template <unsigned int windowVSize = INPUT_SAMPLES>
    using uut_g = dsplib::fir::interpolate_hb::UUT_GRAPH<DATA_TYPE,
                                                         COEFF_TYPE,
                                                         FIR_LEN,
                                                         SHIFT,
                                                         ROUND_MODE,
                                                         windowVSize,
                                                         CASC_LEN,
                                                         DUAL_IP,
                                                         USE_COEFF_RELOAD,
                                                         NUM_OUTPUTS,
                                                         UPSHIFT_CT,
                                                         PORT_API,
                                                         P_SSR,
                                                         P_PARA_INTERP_POLY,
                                                         SAT_MODE>;
    // Constructor
    test_graph() {
        printConfig();

        namespace dsplib = xf::dsp::aie;
        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        test_stim<COEFF_TYPE, (FIR_LEN + 1) / 4 + 1, 0> taps_gen(QUOTE(COEFF_FILE));
        srand(115552);
        int error_tap =
            rand() %
            FIR_LEN; // Randomly selects a single coefficient to be changed in second coefficient array to test reload
#ifdef _DSPLIB_FIR_DEBUG_ADL_
        error_tap = (FIR_LEN + 1) / 4 + 1 - 1; // Always overwrite the last coeff only.
#endif                                         // _DSPLIB_FIR_DEBUG_ADL_
        for (int j = 0; j < 2; j++) {
            taps_gen.prepSeed(COEFF_SEED);
            taps_gen.gen(COEFF_STIM_TYPE, taps);
            for (int i = 0; i < (FIR_LEN + 1) / 4 + 1; i++) {
                m_taps[j][i] = taps[i];
                if (i == error_tap && j == 1) {
                    m_taps[j][i] = addError(m_taps[j][i]);
                }
                // Last one - Center tap coeff
                if (UPSHIFT_CT == 0) {
                    if (i == error_tap && j == 1) {
                        m_taps[j][(FIR_LEN + 1) / 4] = addError(taps[(FIR_LEN + 1) / 4]);
                    }
                } else {
                    m_taps[j][(FIR_LEN + 1) / 4] = formatUpshiftCt(taps[(FIR_LEN + 1) / 4]);
                }
            }
        }

        // Copy taps from C++ array into std::vector
        for (int i = 0; i < (FIR_LEN + 1) / 4 + 1; i++) {
            m_taps_v.push_back(m_taps[0][i]);
        }

#ifdef USING_UUT
        static constexpr int kMinLen =
            uut_g<INPUT_SAMPLES>::getMinCascLen<FIR_LEN, PORT_API, DATA_TYPE, COEFF_TYPE, P_SSR>();
        printf("For this config the Minimum CASC_LEN is %d\n", kMinLen);
#if (PORT_API == 1)
        static constexpr int kOptLen =
            uut_g<INPUT_SAMPLES>::getOptCascLen<FIR_LEN, DATA_TYPE, COEFF_TYPE, PORT_API, NUM_OUTPUTS, P_SSR>();
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
#if (USE_CHAIN == 1 && ((NUM_OUTPUTS == 1 && DUAL_IP == 0) || (NUM_OUTPUTS == 2 && DUAL_INPUT_SAMPLES == 1)))
// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        uut_g<INPUT_SAMPLES * 2> firGraph2;
#else // Multi-kernel, static coefficients
        uut_g<INPUT_SAMPLES * 2> firGraph2(m_taps_v);
#endif
#endif

        // Make connections
        createPLIOFileConnections<P_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<SSR_OUT, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

        for (unsigned int i = 0; i < P_SSR; ++i) {
            unsigned int plioBaseIdx = i * (DUAL_INPUT_SAMPLES + 1);
            unsigned int plioOutputBaseIdx = i * P_PARA_INTERP_POLY * NUM_OUTPUTS;
            connect<>(in[plioBaseIdx].out[0], firGraph.in[i]); // firGraph.in[i] once graph supports array ports

#if (DUAL_IP == 1 && PORT_API == 0) // dual input to avoid contention
            // if not using interleaved streams, just use a duplicate of in1
            connect<>(in[plioBaseIdx].out[0], firGraph.in2[i]); // firGraph.in2[i] later. Script performs clone. If it
                                                                // doesn't, remove DUAL_INPUT_SAMPLES here.
#endif
#if (DUAL_IP == 1 && PORT_API == 1)
            connect<>(in[plioBaseIdx + DUAL_INPUT_SAMPLES].out[0],
                      firGraph.in2[i]); // firGraph.in2[i] once graph supports array ports
#endif

#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1)
            // Chained connections mutually explusive with multiple outputs.
            connect<>(firGraph.out, firGraph2.in);
            connect<>(firGraph2.out, out[plioBaseIdx].in[0]);
#else
            connect<>(firGraph.out[i], out[plioOutputBaseIdx].in[0]);
#if (NUM_OUTPUTS == 2)
            connect<>(firGraph.out2[i],
                      out[plioOutputBaseIdx + 1].in[0]); // firGraph.out[1] or similar when array ports are supported.
#endif
#if (P_PARA_INTERP_POLY == 2)
            connect<>(firGraph.out3[i], out[plioOutputBaseIdx + NUM_OUTPUTS].in[0]);
#if (NUM_OUTPUTS == 2)
            connect<>(firGraph.out4[i],
                      out[plioOutputBaseIdx + 3].in[0]); // firGraph.out[1] or similar when array ports are supported.
#endif
#endif
#endif
        }
#if (USE_COEFF_RELOAD == 1)
        for (int i = 0; i < P_SSR; i++) {
            connect<>(coeff[i], firGraph.coeff[i]);
#if (P_PARA_INTERP_POLY == 2)
            connect<>(coeffCT[i], firGraph.coeffCT[i]);
#endif
        }
#endif

#ifdef USING_UUT
        const int MAX_PING_PONG_SIZE = 16384;
        const int MEMORY_MODULE_SIZE = 32768;
        const int inputBufferSize = PORT_API == 1 ? 0 : (FIR_LEN + INPUT_SAMPLES) * sizeof(DATA_TYPE);
        const int outputBufferSize =
            PORT_API == 1
                ? 0
                : (INPUT_SAMPLES * 2) *
                      sizeof(DATA_TYPE); // Due to interpolation, output buffer may be of greater size than input buffer

        if (inputBufferSize > MAX_PING_PONG_SIZE) {
            single_buffer(firGraph.getKernels()->in[0]);
            if
                constexpr(DUAL_IP == 1) { single_buffer(firGraph.getKernels()->in[1]); }
        }
        if (outputBufferSize > MAX_PING_PONG_SIZE) {
            single_buffer(firGraph.getKernels()[CASC_LEN - 1].out[0]);
#if (NUM_OUTPUTS == 2)
            single_buffer(firGraph.getKernels()[CASC_LEN - 1].out[1]);
#endif
        }

        // use default ping-pong buffer, unless requested buffer exceeds memory module size
        static_assert(!(PORT_API == 0 && inputBufferSize >= MEMORY_MODULE_SIZE),
                      "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
                      "Module size of 32kB");
        static_assert(!(PORT_API == 0 && outputBufferSize >= MEMORY_MODULE_SIZE),
                      "ERROR: Output Window size (based on requrested window size and rate change) exceeds Memory "
                      "Module size of 32kB");
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
