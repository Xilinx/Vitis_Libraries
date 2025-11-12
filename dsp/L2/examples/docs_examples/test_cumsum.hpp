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
// This file holds the definition of the example test harness for the cumsum graph.

#include <adf.h>
#include <vector>
#include <array>

#include "cumsum_graph.hpp"

using namespace adf;
namespace cumsum_example {
#define DATA_TYPE_CS int16
#define DATA_OUT_TYPE_CS int16
#define DIM_A_CS 128
#define DIM_B_CS 1
#define NUM_FRAMES_CS 1
#define MODE_CS 0
#define SHIFT_CS 0

class test_cumsum : public graph {
   private:
   public:
    port<input> in;
    port<output> out;

    xf::dsp::aie::cumsum::
        cumsum_graph<DATA_TYPE_CS, DATA_OUT_TYPE_CS, DIM_A_CS, DIM_B_CS, NUM_FRAMES_CS, MODE_CS, SHIFT_CS>
            cumsumGraph;

    // Constructor
    test_cumsum() {
        // Make connections
        connect<>(in, cumsumGraph.in[0]);
        connect<>(cumsumGraph.out[0], out);
    };
};
};
