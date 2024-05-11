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

#include <adf.h>

#include "fir_sr_sym_graph.hpp"

#define FIR129_NITER 8
#define FIR129_LENGTH 129
#define FIR129_SHIFT 15
#define FIR129_ROUND_MODE 0
#define FIR129_INPUT_SAMPLES 256

using namespace adf;

namespace testcase {

class test_kernel : public graph {
   private:
    // FIR coefficients
    std::vector<int16> m_taps = std::vector<int16>{
        -1,   -3,   3,    -1,   -3,   6,    -1,   -7,   9,     -1,   -12,  14,    1,     -20,  19,   5,    -31,
        26,   12,   -45,  32,   23,   -63,  37,   40,   -86,   40,   64,   -113,  39,    96,   -145, 33,   139,
        -180, 17,   195,  -218, -9,   266,  -258, -53,  357,   -299, -118, 472,   -339,  -215, 620,  -376, -360,
        822,  -409, -585, 1118, -437, -973, 1625, -458, -1801, 2810, -470, -5012, 10783, 25067};
    // FIR Graph class
    xf::dsp::aie::fir::sr_sym::
        fir_sr_sym_graph<cint16, int16, FIR129_LENGTH, FIR129_SHIFT, FIR129_ROUND_MODE, FIR129_INPUT_SAMPLES>
            firGraph;

   public:
    input_plio in;
    output_plio out;

    // in = input_plio::create(plio_64_bits,"data/input.txt");
    // out = output_plio::create(plio_64_bits,"data/output.txt");

    // Constructor - with FIR graph class initialization
    test_kernel() : firGraph(m_taps) {
        in = input_plio::create(plio_32_bits, "data/input.txt");
        out = output_plio::create(plio_32_bits, "data/output.txt");

        // Make connections
        // Size of window in Bytes.
        // Margin gets automatically added within the FIR graph class.
        // Margin equals to FIR length rounded up to nearest multiple of 32 Bytes.
        connect<>(in.out[0], firGraph.in[0]);
        connect<>(firGraph.out[0], out.in[0]);
    };
};
};
