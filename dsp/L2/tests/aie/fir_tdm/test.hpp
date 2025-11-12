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
   Asymmetrical FIR reference model graph.
*/

#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_utils.hpp"
#include "fir_common_traits.hpp"
#include "device_defs.h"
#include "fir_tdm_native_generated_graph/fir_tdm_generated_graph.h"
#include "pkt_switch_graph.hpp"

#ifndef UUT_GRAPH
#define UUT_GRAPH fir_tdm_graph
#endif

#ifndef USE_PKT_SWITCHING
#define USE_PKT_SWITCHING 0
#endif

#ifdef USING_UUT
#if (USE_PKT_SWITCHING == 0)

#define NPORT_I P_SSR
#define NPORT_O P_SSR
#endif

#else
// Use SSR when not doing packet switching
#undef USE_PKT_SWITCHING
#undef NPORT_I
#undef NPORT_O

#define USE_PKT_SWITCHING 0
#define NPORT_I P_SSR
#define NPORT_O P_SSR

#endif

#define NPLIO_I NPORT_I
#define NPLIO_O NPORT_O

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
namespace dsplib = xf::dsp::aie;

class test_graph : public graph {
   public:
    static constexpr int tapsNo = (FIR_LEN * TDM_CHANNELS);
    COEFF_TYPE taps[tapsNo];

   public:
    // When just a duplicate (windows dual ip), use only one plio per ssr port on input.

    std::array<input_plio, NPLIO_I> in;
    std::array<output_plio, NPLIO_O> out;

#if (USE_COEFF_RELOAD == 1)
    // port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR> coeff;
    port_conditional_array<input, USE_COEFF_RELOAD == 1, P_SSR * CASC_LEN> coeff;
#endif

#ifdef USING_UUT
    // Use Generated Graph
    using uut = fir_tdm_native_generated_graph;
#else
    // Use Graph class directly for reference model
    using uut = dsplib::fir::tdm::UUT_GRAPH<DATA_TYPE,
                                            COEFF_TYPE,
                                            FIR_LEN,
                                            SHIFT,
                                            ROUND_MODE,
                                            INPUT_WINDOW_VSIZE,
                                            TDM_CHANNELS,
                                            NUM_OUTPUTS,
                                            DUAL_IP,
                                            P_SSR,
                                            SAT_MODE,
                                            CASC_LEN,
                                            DATA_OUT_TYPE,
                                            USE_COEFF_RELOAD>;
#endif

    std::vector<COEFF_TYPE> m_taps_v =
        generateTaps<COEFF_TYPE, COEFF_STIM_TYPE, FIR_LEN * TDM_CHANNELS, COEFF_SEED>(QUOTE(COEFF_FILE));

#if (USE_PKT_SWITCHING == 1)
    pkt_switch_graph<P_SSR, NPORT_I, NPORT_O, uut> firGraph;
#else
    uut firGraph;

#endif

#if (USE_COEFF_RELOAD == 1)
    // Constructor
    test_graph() {
#else
    test_graph() : firGraph(m_taps_v) {
#endif
        printConfig();

        // Make plio connections
        createPLIOFileConnections<NPLIO_I, DUAL_INPUT_SAMPLES>(in, QUOTE(INPUT_FILE), "in");
        createPLIOFileConnections<NPLIO_O, (NUM_OUTPUTS - 1)>(out, QUOTE(OUTPUT_FILE), "out");

#if (USE_PKT_SWITCHING == 1)
        // Connect packet switching network.
        // Internal connections done by Graph will
        for (unsigned int i = 0; i < NPORT_I; ++i) {
            connect<>(in[i].out[0], firGraph.pkt_in[i]);
        }

        for (unsigned int i = 0; i < NPORT_O; ++i) {
            connect<>(firGraph.pkt_out[i], out[i].in[0]);
        }
#else

        for (unsigned int i = 0; i < P_SSR; ++i) {
            unsigned int ssrIdx = i;
            connect<>(in[ssrIdx].out[0], firGraph.in[ssrIdx]);

            connect<>(firGraph.out[ssrIdx], out[ssrIdx].in[0]);
        }
#endif

#ifdef USING_UUT

#if (USE_CUSTOM_CONSTRAINT == 1)
        // place location constraints
        int LOC_XBASE = 20;
        int LOC_YBASE = 0;
        for (int outPath = 0; outPath < P_SSR; outPath++) {
            for (int inPhase = 0; inPhase < P_SSR; inPhase++) {
                for (int i = 1; i < CASC_LEN; i++) {
                    location<kernel>(*firGraph.filter.getKernels(outPath, inPhase, i)) =
                        tile(LOC_XBASE + inPhase * CASC_LEN + i, LOC_YBASE + 2 * outPath);
                }
            }
        }

        // overwrite the internally calculated fifo depth with some other value.

        for (int outPath = 0; outPath < P_SSR; outPath++) {
            for (int inPhase = 0; inPhase < P_SSR; inPhase++) {
                for (int i = 0; i < CASC_LEN; i++) {
                    connect<stream, stream>* net = firGraph.filter.getInNet(outPath, inPhase, i);
                    fifo_depth(*net) = 256 + 16 * outPath + 16 * inPhase + 16 * i;
                }
            }
        }
#endif

#if (USE_PKT_SWITCHING == 1)
// Single buffer constraint is disabled in the testbench because fir_tdm_generated_graph does not provide direct access
// methods. Constraints can be applied using hierarchical members if needed.
#else

        const int MAX_PING_PONG_SIZE = __DATA_MEM_BYTES__ / 2;
        const int bufferSize =
            (PORT_API == 1 ? 0
                           : (/*(FIR_LEN * TDM_CHANNELS / P_SSR)*/ 0 + INPUT_WINDOW_VSIZE / P_SSR) * sizeof(DATA_TYPE));
        if ((bufferSize > MAX_PING_PONG_SIZE) || (SINGLE_BUF == 1 && PORT_API == 0)) {
            for (int ssr = 0; ssr < P_SSR; ssr++) {
                for (int casc_len = 0; casc_len < CASC_LEN; casc_len++) {
                    single_buffer(firGraph.filter.getKernels(CASC_LEN * ssr + casc_len)->in[0]);
                    printf("INFO: Single Buffer Constraint applied to input buffers of kernel %d.\n",
                           CASC_LEN * ssr + casc_len);
                }
                single_buffer(firGraph.filter.getKernels()[CASC_LEN * ssr + CASC_LEN - 1].out[0]);
                printf("INFO: Single Buffer Constraint applied to output buffers of kernel %d.\n",
                       CASC_LEN * ssr + CASC_LEN - 1);
            }
        }
#endif

#endif

#if (USE_COEFF_RELOAD == 1)
        // for (int i = 0; i < P_SSR; i++) {
        for (int i = 0; i < P_SSR * CASC_LEN; i++) {
            connect<>(coeff[i], firGraph.coeff[i]);
        }
#endif
        printf("========================\n");
    };
};
} // namespace testcase
} // namespace aie
} // namespace dsp
}; // namespace xf

#endif // _DSPLIB_TEST_HPP_
