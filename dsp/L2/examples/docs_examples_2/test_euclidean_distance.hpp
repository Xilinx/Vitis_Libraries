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
#ifndef _DSPLIB_TEST_ED_HPP_
#define _DSPLIB_TEST_ED_HPP_

#include <adf.h>
#include <vector>
#include <array>

#include "euclidean_distance_graph.hpp"

using namespace adf;
#define ED_DATA float
#define ED_DATA_OUT float
#define ED_LEN 32
#define ED_DIM 3
#define ED_API_IO 0
#define ED_WINDOW_VSIZE_FA 512
#define ED_RND 0
#define ED_SAT 0
#define ED_IS_OUTPUT_SQUARED 1

namespace euclidean_distance_example {

class test_euclidean_distance : public graph {
   private:
   public:
    port<input> inP;
    port<input> inQ;
    port<output> out;
    // Constructor
    test_euclidean_distance() {
        xf::dsp::aie::euclidean_distance::euclidean_distance_graph<ED_DATA, ED_DATA_OUT, ED_LEN, ED_DIM, ED_API_IO,
                                                                   ED_RND, ED_SAT, ED_IS_OUTPUT_SQUARED>
            euclideanDistanceGraph;

        // Make connections
        connect<>(inP, euclideanDistanceGraph.inWindowP);
        connect<>(inQ, euclideanDistanceGraph.inWindowQ);
        connect<>(euclideanDistanceGraph.outWindow, out);
    };
};
};

#endif // _DSPLIB_TEST_ED_HPP_