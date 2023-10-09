/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#include "matrix_vector_mul_graph.hpp"

using namespace adf;
namespace mv_example {

#define MV_DATA_A cint16
#define MV_DATA_B cint16
#define MV_DIM_A 16
#define MV_DIM_B 32
#define MV_SHIFT 16
#define MV_ROUND_MODE 0
#define MV_NUM_FRAMES 1
#define MV_CASC_LEN 1
#define MV_SAT_MODE 1
namespace dsplib = xf::dsp::aie;

class test_mv : public adf::graph {
   public:
    xf::dsp::aie::port_array<input, MV_CASC_LEN> inA;
    xf::dsp::aie::port_array<input, MV_CASC_LEN> inB;
    xf::dsp::aie::port_array<output, 1> out;
    xf::dsp::aie::blas::matrix_vector_mul::matrix_vector_mul_graph<MV_DATA_A,
                                                                   MV_DATA_B,
                                                                   MV_DIM_A,
                                                                   MV_DIM_B,
                                                                   MV_SHIFT,
                                                                   MV_ROUND_MODE,
                                                                   MV_NUM_FRAMES,
                                                                   MV_CASC_LEN,
                                                                   MV_SAT_MODE>
        matrix_vector_mulGraph;
    test_mv() {
        // make connections
        for (int i = 0; i < MV_CASC_LEN; i++) {
            connect<>(inA[i], matrix_vector_mulGraph.inA[i]);
            connect<>(inB[i], matrix_vector_mulGraph.inB[i]);
        }
        connect<>(matrix_vector_mulGraph.out[0], out[0]);
    };
};
};