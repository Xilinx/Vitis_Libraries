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
#ifndef _DSPLIB_TEST_FA_HPP_
#define _DSPLIB_TEST_FA_HPP_

// This file holds the definition of the test harness for the Function Approximation graph.

#include <adf.h>
#include <vector>
#include <array>

#include "func_approx_fns.hpp"
#include "func_approx_graph.hpp"

using namespace adf;
namespace func_approx_example {
#define DATA_TYPE_FA float
#define COARSE_BITS_FA 8
#define FINE_BITS_FA 7
#define DOMAIN_MODE_FA 2
#define WINDOW_VSIZE_FA 512
#define SHIFT_FA 0
#define ROUND_MODE_FA 1
#define SAT_MODE_FA 0

class test_func_approx : public graph {
   private:
    static constexpr int sectionTotal = 1 << COARSE_BITS_FA;
    static constexpr int sectionWidth = 1 << FINE_BITS_FA;
    static constexpr int ignoreTopDomainBit =
        (DOMAIN_MODE_FA == 1)
            ? 1
            : 0; // DOMAIN_MODE_FA = 1 assumes that the most significant coarse bit has been set to zero
    static constexpr int lutSize =
        (sectionTotal * 2) / (ignoreTopDomainBit + 1); // Each section has two values (slope and offset)

    // typename TT_LUT = DATA_TYPE_FA, except when DATA_TYPE_FA = bfloat16, where TT_LUT = float
    typedef typename std::conditional<std::is_same<DATA_TYPE_FA, bfloat16>::value, float, DATA_TYPE_FA>::type LUT_TYPE;

    std::array<LUT_TYPE, lutSize> lut_values;

   public:
    port<input> in;
    port<output> out;
    // Constructor
    test_func_approx() {
        // populate lookup table with slope-offset values for sqrt approximation. This will be a constructor argument to
        // the func_approx graph.
        xf::dsp::aie::func_approx::getSqrt<DATA_TYPE_FA, LUT_TYPE>((LUT_TYPE*)&lut_values[0], COARSE_BITS_FA,
                                                                   FINE_BITS_FA, DOMAIN_MODE_FA, SHIFT_FA);
        xf::dsp::aie::func_approx::func_approx_graph<DATA_TYPE_FA, COARSE_BITS_FA, FINE_BITS_FA, DOMAIN_MODE_FA,
                                                     WINDOW_VSIZE_FA, SHIFT_FA, ROUND_MODE_FA, SAT_MODE_FA>
            funcApproxGraph(lut_values);

        // Make connections
        connect<>(in, funcApproxGraph.in[0]);
        connect<>(funcApproxGraph.out[0], out);
    };
};
};

#endif // _DSPLIB_TEST_FA_HPP_