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
#include "matrix_mult_graph.hpp"

using namespace adf;
namespace mm_example {

#define T_DATA_A cint16
#define T_DATA_B cint16
#define P_DIM_A 32
#define P_DIM_AB 16
#define P_DIM_B 32
#define P_SHIFT 16
#define P_ROUND_MODE 0
#define P_DIM_A_LEADING 0
#define P_DIM_B_LEADING 1
#define P_DIM_OUT_LEADING 0
#define P_ADD_TILING_A 0
#define P_ADD_TILING_B 0
#define P_ADD_DETILING_OUT 0
#define P_INPUT_WINDOW_VSIZE_A 512
#define P_INPUT_WINDOW_VSIZE_B 512
#define P_CASC_LEN 1
#define P_SSR 1

class test_mm : public adf::graph {
   public:
    xf::dsp::aie::port_array<input, P_SSR * P_CASC_LEN> inA;
    xf::dsp::aie::port_array<input, P_SSR * P_CASC_LEN> inB;
    xf::dsp::aie::port_array<output, P_SSR> out;
    xf::dsp::aie::blas::matrix_mult::matrix_mult_graph<T_DATA_A,
                                                       T_DATA_B,
                                                       P_DIM_A,
                                                       P_DIM_AB,
                                                       P_DIM_B,
                                                       P_SHIFT,
                                                       P_ROUND_MODE,
                                                       P_DIM_A_LEADING,
                                                       P_DIM_B_LEADING,
                                                       P_DIM_OUT_LEADING,
                                                       P_ADD_TILING_A,
                                                       P_ADD_TILING_B,
                                                       P_ADD_DETILING_OUT,
                                                       P_INPUT_WINDOW_VSIZE_A,
                                                       P_INPUT_WINDOW_VSIZE_B,
                                                       P_CASC_LEN,
                                                       P_SSR>
        matrixMult;
    test_mm() {
        kernel* kernels = matrixMult.getKernels();
        for (int ssrIdx = 0; ssrIdx < P_SSR; ssrIdx++) {
            for (int cascIdx = 0; cascIdx < P_CASC_LEN; cascIdx++) {
                // Set runtime ratio for each kernel
                runtime<ratio>(kernels[ssrIdx * P_CASC_LEN + cascIdx]) = 0.7;

                // Connect input A and B data
                connect<>(inA[ssrIdx * P_CASC_LEN + cascIdx], matrixMult.inA[ssrIdx * P_CASC_LEN + cascIdx]);
                connect<>(inB[ssrIdx * P_CASC_LEN + cascIdx], matrixMult.inB[ssrIdx * P_CASC_LEN + cascIdx]);
            }
            // Connect output data
            connect<>(matrixMult.out[ssrIdx], out[ssrIdx]);
        }
    };
};
};