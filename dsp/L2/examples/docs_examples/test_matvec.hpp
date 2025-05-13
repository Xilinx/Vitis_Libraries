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
#include "matrix_vector_mul_graph.hpp"

using namespace adf;
namespace mv_example {

#define MV_DATA_A cint16
#define MV_DATA_B cint16
#define MV_DIM_A 32
#define MV_DIM_B 16
#define MV_SHIFT 16
#define MV_ROUND_MODE 0
#define MV_NUM_FRAMES 1
#define MV_CASC_LEN 1
#define MV_SAT_MODE 1
#define MV_SSR 2
#define MV_DIM_A_LEADING 1
namespace dsplib = xf::dsp::aie;

class test_mv : public adf::graph {
   public:
    xf::dsp::aie::port_array<input, MV_SSR * MV_CASC_LEN> inA;
    xf::dsp::aie::port_array<input, MV_SSR * MV_CASC_LEN> inB;
    xf::dsp::aie::port_array<output, MV_SSR> out;
    xf::dsp::aie::blas::matrix_vector_mul::matrix_vector_mul_graph<MV_DATA_A,
                                                                   MV_DATA_B,
                                                                   MV_DIM_A,
                                                                   MV_DIM_B,
                                                                   MV_SHIFT,
                                                                   MV_ROUND_MODE,
                                                                   MV_NUM_FRAMES,
                                                                   MV_CASC_LEN,
                                                                   MV_SAT_MODE,
                                                                   MV_SSR,
                                                                   MV_DIM_A_LEADING>
        matrix_vector_mulGraph;
    test_mv() {
        // make connections
        for (int ssrIdx = 0; ssrIdx < MV_SSR; ssrIdx++) {
            for (int cascIdx = 0; cascIdx < MV_CASC_LEN; cascIdx++) {
                connect<>(inA[(ssrIdx * MV_CASC_LEN) + cascIdx],
                          matrix_vector_mulGraph.inA[(ssrIdx * MV_CASC_LEN) + cascIdx]);
                connect<>(inB[(ssrIdx * MV_CASC_LEN) + cascIdx],
                          matrix_vector_mulGraph.inB[(ssrIdx * MV_CASC_LEN) + cascIdx]);
            }
            connect<>(matrix_vector_mulGraph.out[ssrIdx], out[ssrIdx]);
        }
    };
};
};