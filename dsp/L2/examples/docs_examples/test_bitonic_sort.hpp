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
#include "bitonic_sort_graph.hpp"

using namespace adf;
namespace bitonic_sort_example {

#define DATATYPE_BS int32
#define DIM_BS 32
#define NUM_FRAMES_BS 4
#define ASCEND_BS 1
#define CASCADE_BS 2

class test_bitonic_sort : public adf::graph {
   public:
    port<input> in;
    port<output> out;
    xf::dsp::aie::bitonic_sort::bitonic_sort_graph<DATATYPE_BS, DIM_BS, NUM_FRAMES_BS, ASCEND_BS, CASCADE_BS>
        bitonicSort;
    test_bitonic_sort() {
        connect<>(in, bitonicSort.in[0]);
        connect<>(bitonicSort.out[0], out);
    };
};
};