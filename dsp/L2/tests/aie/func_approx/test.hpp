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

// This file holds the definition of the test harness for the Function Approximation graph.

#include <adf.h>
#include <vector>
#include <functional>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"
#include "func_approx_fns.hpp"
#include "func_approx_native_generated_graph/func_approx_generated_graph.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH func_approx_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

#define SQRT_FUNC 0
#define INVSQRT_FUNC 1
#define LOG_FUNC 2
#define EXP_FUNC 3
#define INV_FUNC 4

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

class test_graph : public graph {
   private:
    static constexpr int sectionTotal = 1 << COARSE_BITS;
    static constexpr int sectionWidth = 1 << FINE_BITS;
    static constexpr int ignoreTopDomainBit = (DOMAIN_MODE == 1) ? 1 : 0;

   public:
    // LUTs for bfloat16 input data have float slope and offset values within
    typedef typename std::conditional<std::is_same<DATA_TYPE, bfloat16>::value, float, DATA_TYPE>::type LUT_TYPE;

    static constexpr int lutSize =
        (sectionTotal * 2) / (ignoreTopDomainBit + 1); // Each section has two values (slope and offset)
    std::array<input_plio, 1> in;
    std::array<output_plio, 1> out;
    // Static function to initialize LUT based on function choice
    static std::array<LUT_TYPE, lutSize> initializeLUT(int func_choice) {
        std::array<LUT_TYPE, lutSize> lut_array = {};
        namespace dsplib = xf::dsp::aie;
        switch (func_choice) {
            case SQRT_FUNC:
                dsplib::func_approx::getSqrt<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&lut_array[0], COARSE_BITS, FINE_BITS,
                                                                  DOMAIN_MODE, SHIFT);
                break;
            case INVSQRT_FUNC:
                dsplib::func_approx::getInvSqrt<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&lut_array[0], COARSE_BITS, FINE_BITS,
                                                                     DOMAIN_MODE, SHIFT);
                break;
            case LOG_FUNC:
                dsplib::func_approx::getLog<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&lut_array[0], COARSE_BITS, FINE_BITS,
                                                                 DOMAIN_MODE, SHIFT);
                break;
            case EXP_FUNC:
                dsplib::func_approx::getExp<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&lut_array[0], COARSE_BITS, FINE_BITS,
                                                                 DOMAIN_MODE, SHIFT);
                break;
            case INV_FUNC:
                dsplib::func_approx::getInv<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&lut_array[0], COARSE_BITS, FINE_BITS,
                                                                 DOMAIN_MODE, SHIFT);
                break;
            default:
                dsplib::func_approx::getSqrt<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&lut_array[0], COARSE_BITS, FINE_BITS,
                                                                  DOMAIN_MODE, SHIFT);
                break;
        }
        return lut_array;
    }
#ifdef USING_UUT
    // use generated graph
    using uut = func_approx_native_generated_graph;
#else
    using uut = xf::dsp::aie::func_approx::UUT_GRAPH<DATA_TYPE,
                                                     COARSE_BITS,
                                                     FINE_BITS,
                                                     DOMAIN_MODE,
                                                     WINDOW_VSIZE,
                                                     SHIFT,
                                                     ROUND_MODE,
                                                     SAT_MODE,
                                                     USE_LUT_RELOAD>;
#endif
    static constexpr int rtpPortsPerKernel = uut::getTotalRtpPorts();
    port_conditional_array<input, (USE_LUT_RELOAD == 1), rtpPortsPerKernel> rtpLut;

    std::array<LUT_TYPE, lutSize> m_luts_ab = initializeLUT(FUNC_CHOICE);

    uut funcApproxGraph;
// Function approximation graph instance
#if (USE_LUT_RELOAD == 1)
    std::array<LUT_TYPE, lutSize> m_luts_cd = initializeLUT(SQRT_FUNC);
    test_graph() {
#else
    test_graph() : funcApproxGraph(m_luts_ab) {
#endif
#if (USE_LUT_RELOAD != 0) // Reloadable lut
        static_assert(NITER % 2 == 0, "ERROR: Please set NITER to be a multiple of 2 when reloadable lookups are used");
#endif

#if (USE_LUT_RELOAD == 1)
        // pass two luts via rtp
        for (int i = 0; i < rtpPortsPerKernel; i++) {
            connect<parameter>(rtpLut[i], funcApproxGraph.rtpLut[i]);
        }
#endif
        // Make connections
        std::string filenameIn = QUOTE(INPUT_FILE);
        in[0] = input_plio::create("PLIO_in_" + std::to_string(0), adf::plio_64_bits, filenameIn);
        connect<>(in[0].out[0], funcApproxGraph.in[0]);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(funcApproxGraph.out[0], out[0].in[0]);
#ifdef USING_UUT
#if (SINGLE_BUF == 1)
        printf("DEBUG1.\n");
        single_buffer(funcApproxGraph.getKernels()[0].in[0]);
        single_buffer(funcApproxGraph.getKernels()[0].out[0]);
        printf("INFO: Single Buffer Constraint applied to input and output buffers of kernel.\n");
#endif
#endif

        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
