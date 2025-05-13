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
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "matrix_vector_mul_ref.hpp"
#include "fir_ref_utils.hpp"

// #define _DSPLIB_MATRIX_VECTOR_MUL_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_vector_mul {

// Example test for matrix_vector_mul matrix vector multiply
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_DIM_A_LEADING,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
void matrix_vector_mul_ref<TT_DATA_A,
                           TT_DATA_B,
                           TP_DIM_A,
                           TP_DIM_B,
                           TP_DIM_A_LEADING,
                           TP_SHIFT,
                           TP_RND,
                           TP_SAT,
                           TP_NUM_FRAMES,
                           TP_CASC_LEN>::matrix_vector_mul(input_buffer<TT_DATA_A>& inWindowA,
                                                           input_buffer<TT_DATA_B>& inWindowB,
                                                           output_buffer<outType_t<TT_DATA_A, TT_DATA_B> >& outWindow) {
    const unsigned int shift = TP_SHIFT;
    using TT_OUT = outType_t<TT_DATA_A, TT_DATA_B>;
    TT_DATA_A dA_in[TP_DIM_B];
    TT_DATA_B dB_in[TP_DIM_B];
    TT_OUT outData;
    T_accRef<TT_OUT> accum;

    TT_DATA_A* inPtrA = (TT_DATA_A*)inWindowA.data();
    TT_DATA_A* inRowA;
    TT_DATA_B* inPtrB = (TT_DATA_B*)inWindowB.data();
    TT_OUT* outPtr = (TT_OUT*)outWindow.data();

    printf("Ref model params:\n");
    printf("TP_SHIFT = %d\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("windowSizeA = %d\n", windowSizeA);
    printf("windowSizeB = %d\n", windowSizeB);
    printf("windowSizeOut = %d\n", windowSizeOut);
    printf("NUM_FRAMES = %d\n\n", TP_NUM_FRAMES);

    const int matrixRowInc = (TP_DIM_A_LEADING == COL_MAJOR) ? 1 : TP_DIM_B;
    const int matrixElemInc = (TP_DIM_A_LEADING == COL_MAJOR) ? TP_DIM_A : 1;
    for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
        // Load vector data
        for (int elemB = 0; elemB < TP_DIM_B; elemB++) {
            dB_in[elemB] = *inPtrB++;
        }
        // Multiply each row of matrix with the vector
        for (int row = 0; row < TP_DIM_A; row++) {
            inRowA = inPtrA + (row * matrixRowInc);
            // Load a row of the the matrix
            for (int elemA = 0; elemA < TP_DIM_B; elemA++) {
                dA_in[elemA] = *inRowA;
                inRowA += matrixElemInc;
            }
            accum = null_accRef<TT_OUT>();

            for (int elemAB = 0; elemAB < TP_DIM_B; elemAB++) {
                multiplyAcc(accum, dA_in[elemAB], dB_in[elemAB]);
            }
            roundAcc(TP_RND, shift, accum);
            saturateAcc(accum, TP_SAT);
            outData = castAcc(accum);
            *outPtr++ = outData;
        }
        inPtrA += (TP_DIM_B) * (TP_DIM_A);
    }
};
}
}
}
}
}
