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
fir_decimate_asym graph class.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"
#include "uut_config.h"
#include "uut_static_config.h"
#include "test_utils.hpp"
#include "fir_common_traits.hpp"

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_decimate_asym_graph
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
    COEFF_TYPE taps[FIR_LEN];

   public:
    static constexpr unsigned int IN_SSR = P_SSR * P_PARA_DECI_POLY;
    std::array<input_plio, IN_SSR*(DUAL_INPUT_SAMPLES + 1)> in; // 0? dual_ip - not supported by sr_sym
    std::array<output_plio, P_SSR * NUM_OUTPUTS> out;           // NUM_OUTPUTS forces to 1 for ref

#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeff;
#endif

    COEFF_TYPE m_taps[2][FIR_LEN];
    std::vector<COEFF_TYPE> m_taps_v;

    template <unsigned int windowVSize = INPUT_SAMPLES>
    using uut_g = dsplib::fir::decimate_asym::UUT_GRAPH<DATA_TYPE,
                                                        COEFF_TYPE,
                                                        FIR_LEN,
                                                        DECIMATE_FACTOR,
                                                        SHIFT,
                                                        ROUND_MODE,
                                                        windowVSize,
                                                        CASC_LEN,
                                                        USE_COEFF_RELOAD,
                                                        NUM_OUTPUTS,
                                                        DUAL_IP,
                                                        PORT_API,
                                                        P_SSR,
                                                        P_PARA_DECI_POLY,
                                                        SAT_MODE>;

    // Constructor
    test_graph() {
        printConfig();

        namespace dsplib = xf::dsp::aie;
        // Generate random taps
        // STIM_GEN_INCONES, STIM_GEN_ALLONES, STIM_GEN_IMPULSE, STIM_GEN_RANDOM
        test_stim<COEFF_TYPE, FIR_LEN, 0> taps_gen(QUOTE(COEFF_FILE));
        srand(115552);
        int error_tap =
            rand() %
            FIR_LEN; // Randomly selects a single coefficient to be changed in second coefficient array to test reload
        for (int j = 0; j < 2; j++) {
            taps_gen.prepSeed(COEFF_SEED);
            taps_gen.gen(COEFF_STIM_TYPE, taps);
            for (int i = 0; i < (FIR_LEN); i++) {
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

#ifdef USING_UUT
        static constexpr int kMinLen =
            uut_g<INPUT_SAMPLES>::getMinCascLen<FIR_LEN, PORT_API, DATA_TYPE, COEFF_TYPE, DECIMATE_FACTOR, P_SSR>();
        printf("For this config the Minimum CASC_LEN is %d\n", kMinLen);
#if (PORT_API == 1)
        static constexpr int kOptLen =
            uut_g<INPUT_SAMPLES>::getOptCascLen<FIR_LEN, DATA_TYPE, COEFF_TYPE, PORT_API, DECIMATE_FACTOR, P_SSR>();
        printf("For this config the Optimal CASC_LEN is %d\n", kOptLen);
#endif
#endif

#if (USE_COEFF_RELOAD != 0) // Reloadable coefficients
        static_assert(NITER % 2 == 0,
                      "ERROR: Please set NITER to be a multiple of 2 when reloadable coefficients are used");
#endif

#if (USE_COEFF_RELOAD == 0)
        // Static coefficients
        uut_g<INPUT_SAMPLES> firGraph(m_taps_v);
#elif (USE_COEFF_RELOAD == 1)
        // RTP coefficients
        uut_g<INPUT_SAMPLES> firGraph;
#else
        //  default. Pass coeffs through graph constructor.
        uut_g<INPUT_SAMPLES> firGraph(m_taps_v);
#endif

#if (USE_CHAIN == 1 && ((NUM_OUTPUTS == 1 && DUAL_IP == 0) || (NUM_OUTPUTS == 2 && DUAL_INPUT_SAMPLES == 1)))
// FIR sub-graph
#if (USE_COEFF_RELOAD == 1) // Reloadable coefficients
        uut_g<INPUT_SAMPLES / DECIMATE_FACTOR> firGraph2;
#else // Multi-kernel, static coefficients
        uut_g<INPUT_SAMPLES / DECIMATE_FACTOR> firGraph2(m_taps_v);
#endif
#endif

        // Make connections
        createPLIOFileConnections<IN_SSR, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<P_SSR, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

        for (unsigned int i = 0; i < IN_SSR; ++i) {
            // Size of window in Bytes.
            unsigned int plioBaseIdxIn = i * (DUAL_INPUT_SAMPLES + 1);

            connect<>(in[plioBaseIdxIn].out[0], firGraph.in[i]);
#if (DUAL_IP == 1)
            connect<>(in[plioBaseIdxIn + 1].out[0], firGraph.in2[i]); // will change when fir adopts array ports
#endif
        }

#if (USE_CHAIN == 1 && NUM_OUTPUTS == 1)
        // Chained connections mutually explusive with multiple outputs.
        connect<>(firGraph.out[plioBaseIdxOut], firGraph2.in[plioBaseIdxIn]);
        connect<>(firGraph2.out[plioBaseIdxOut], out[plioBaseIdxOut].in[0]);
#else

        for (unsigned int i = 0; i < P_SSR; ++i) {
            unsigned int plioBaseIdxOut = i * (NUM_OUTPUTS);
            connect<>(firGraph.out[i], out[plioBaseIdxOut].in[0]);
#if (NUM_OUTPUTS == 2)
            connect<>(firGraph.out2[i], out[plioBaseIdxOut + 1].in[0]);
#endif
        }
#endif

#if (USE_COEFF_RELOAD == 1)
        for (int i = 0; i < P_SSR; i++) {
            connect<>(coeff[i], firGraph.coeff[i]);
        }
#endif

#ifdef USING_UUT
#if (USE_CUSTOM_CONSTRAINT == 1)
        // place location constraints
        int LOC_XBASE = 20;
        int LOC_YBASE = 0;
        for (int i = 1; i < CASC_LEN; i++) {
            location<kernel>(*firGraph.getKernels(i)) = tile(LOC_XBASE + i, LOC_YBASE);
        }

        // overwrite the internally calculated fifo depth with some other value.
        for (int outPath = 0; outPath < P_SSR; outPath++) {
            for (int inPhase = 0; inPhase < P_SSR; inPhase++) {
                for (int i = 1; i < CASC_LEN; i++) {
                    connect<stream, stream>* net = firGraph.getInNet(outPath, inPhase, i);
                    fifo_depth(*net) = 384 + 16 * i;
#if (DUAL_IP == 1)
                    connect<stream, stream>* net2 = firGraph.getIn2Net(outPath, inPhase, i);
                    fifo_depth(*net2) = 384 + 16 * i;
#endif
                }
            }
        }
#endif

        const int MAX_PING_PONG_SIZE = 16384;
        const int MEMORY_MODULE_SIZE = 32768;
        const int bufferSize = (PORT_API == 1 ? 0 : (FIR_LEN + INPUT_SAMPLES) * sizeof(DATA_TYPE));
        if (bufferSize > MAX_PING_PONG_SIZE) {
            single_buffer(firGraph.getKernels()->in[0]);
            if (DUAL_IP == 1) {
                single_buffer(firGraph.getKernels()->in[1]);
            }
        }
        static_assert(bufferSize < MEMORY_MODULE_SIZE,
                      "ERROR: Input Window size (based on requrested window size and FIR length margin) exceeds Memory "
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
