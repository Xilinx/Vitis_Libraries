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
#include <adf.h>
#include "outer_tensor_graph.hpp"

using namespace adf;
namespace outer_tensor_example {

#define DATATYPE_A_OT cint16
#define DATATYPE_B_OT cint16
#define DIM_A_OT 32
#define DIM_B_OT 32
#define NUM_FRAMES_OT 4
#define SHIFT_OT 2
#define API_OT 0
#define SSR_OT 1

class test_outer_tensor : public adf::graph {
   public:
    port<input> inA;
    port<input> inB;
    port<output> out;
    xf::dsp::aie::outer_tensor::
        outer_tensor_graph<DATATYPE_A_OT, DATATYPE_B_OT, DIM_A_OT, DIM_B_OT, NUM_FRAMES_OT, SHIFT_OT, API_OT, SSR_OT>
            outerTensor;
    test_outer_tensor() {
        connect<>(inA, outerTensor.inA[0]);
        connect<>(inB, outerTensor.inB[0]);
        connect<>(outerTensor.out[0], out);
    };
};
};