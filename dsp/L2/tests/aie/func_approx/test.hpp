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
    static constexpr int lutSize =
        (sectionTotal * 2) / (ignoreTopDomainBit + 1); // Each section has two values (slope and offset)

   public:
    std::array<input_plio, 1> in;
    std::array<output_plio, 1> out;

    // LUTs for bfloat16 input data have float slope and offset values within
    typedef typename std::conditional<std::is_same<DATA_TYPE, bfloat16>::value, float, DATA_TYPE>::type LUT_TYPE;
    std::array<LUT_TYPE, lutSize> m_luts_ab;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== Function Approximation test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type         = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("WINDOW_VSIZE      = %d \n", WINDOW_VSIZE);
        printf("COARSE_BITS       = %d \n", COARSE_BITS);
        printf("FINE_BITS         = %d \n", FINE_BITS);
        printf("DOMAIN_MODE       = %d \n", DOMAIN_MODE);

        namespace dsplib = xf::dsp::aie;
        switch (FUNC_CHOICE) {
            case SQRT_FUNC:
                printf("Function for approximation is set to sqrt()\n");
                dsplib::func_approx::getSqrt<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&m_luts_ab[0], COARSE_BITS, FINE_BITS,
                                                                  DOMAIN_MODE, SHIFT);
                break;
            case INVSQRT_FUNC:
                printf("Function for approximation is set to invsqrt()\n");
                dsplib::func_approx::getInvSqrt<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&m_luts_ab[0], COARSE_BITS, FINE_BITS,
                                                                     DOMAIN_MODE, SHIFT);
                break;
            case LOG_FUNC:
                printf("Function for approximation is set to log()\n");
                dsplib::func_approx::getLog<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&m_luts_ab[0], COARSE_BITS, FINE_BITS,
                                                                 DOMAIN_MODE, SHIFT);
                break;
            case EXP_FUNC:
                printf("Function for approximation is set to exp()\n");
                dsplib::func_approx::getExp<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&m_luts_ab[0], COARSE_BITS, FINE_BITS,
                                                                 DOMAIN_MODE, SHIFT);
                break;
            case INV_FUNC:
                printf("Function for approximation is set to inv()\n");
                dsplib::func_approx::getInv<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&m_luts_ab[0], COARSE_BITS, FINE_BITS,
                                                                 DOMAIN_MODE, SHIFT);
                break;
            default:
                printf("ERROR: unknown FUNC_CHOICE. Defaulting to sqrt().\n");
                dsplib::func_approx::getSqrt<DATA_TYPE, LUT_TYPE>((LUT_TYPE*)&m_luts_ab[0], COARSE_BITS, FINE_BITS,
                                                                  DOMAIN_MODE, SHIFT);
                break;
                break;
        }

// Func Approx sub-graph
#ifdef USING_UUT
        dsplib::func_approx::UUT_GRAPH<DATA_TYPE, COARSE_BITS, FINE_BITS, DOMAIN_MODE, WINDOW_VSIZE, SHIFT, ROUND_MODE,
                                       SAT_MODE>
            funcApproxGraph(m_luts_ab);
#else
        dsplib::func_approx::UUT_GRAPH<DATA_TYPE, COARSE_BITS, FINE_BITS, DOMAIN_MODE, WINDOW_VSIZE, SHIFT, ROUND_MODE,
                                       SAT_MODE, FUNC_CHOICE>
            funcApproxGraph;
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
