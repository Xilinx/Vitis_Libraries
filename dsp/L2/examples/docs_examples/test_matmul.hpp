/*
 * Copyright 2022 Xilinx, Inc.
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

class test_mm : public adf::graph {
   public:
    port<input> inA;
    port<input> inB;
    port<output> out;
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
                                                       P_CASC_LEN>
        matrixMult;
    test_mm() {
        connect<>(inA, matrixMult.inA[0]);
        connect<>(inB, matrixMult.inB[0]);
        connect<>(matrixMult.out, out);
        kernel* kernels = matrixMult.getKernels();
        for (int i = 0; i < P_CASC_LEN; i++) {
            runtime<ratio>(kernels[i]) = 0.7;
            runtime<ratio>(matrixMult.tilerA[i]) = 0.5;
            runtime<ratio>(matrixMult.tilerA[i]) = 0.5;
        }
        runtime<ratio>(matrixMult.untiler) = 0.5;
    };
};
};