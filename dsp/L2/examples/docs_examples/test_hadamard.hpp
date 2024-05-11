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
#include "hadamard_graph.hpp"

using namespace adf;
namespace hadamard_example {

#define T_DATA_A_HAD cint16
#define T_DATA_B_HAD cint16
#define DIM_SIZE_HAD 64
#define NUM_FRAMES_HAD 4
#define SHIFT_HAD 14
#define API_IO_HAD 0
#define UUT_SSR_HAD 1
#define ROUND_MODE_HAD 0
#define SAT_MODE_HAD 0

class test_hadamard : public adf::graph {
   public:
    port<input> inA;
    port<input> inB;
    port<output> out;
    xf::dsp::aie::hadamard::hadamard_graph<T_DATA_A_HAD,
                                           T_DATA_B_HAD,
                                           DIM_SIZE_HAD,
                                           NUM_FRAMES_HAD,
                                           SHIFT_HAD,
                                           API_IO_HAD,
                                           UUT_SSR_HAD,
                                           ROUND_MODE_HAD,
                                           SAT_MODE_HAD>
        hadamardGraph;

    test_hadamard() {
        connect<>(inA, hadamardGraph.inA[0]);
        connect<>(inB, hadamardGraph.inB[0]);
        connect<>(hadamardGraph.out[0], out);
    };
};
};