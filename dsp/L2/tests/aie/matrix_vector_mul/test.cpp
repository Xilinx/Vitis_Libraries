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

/*
This file is the test harness for the matrix_vector_mul graph class.
*/

#include <stdio.h>
#include "test.hpp"

xf::dsp::aie::testcase::test_graph matrix_vector_mul_tb;

int main(void) {
    matrix_vector_mul_tb.init();
#if defined(USING_UUT) && (USE_MATRIX_RELOAD == 1)

    for (int ssr = 0; ssr < UUT_SSR; ssr++) {
        for (int casc = 0; casc < CASC_LEN; casc++) {
            matrix_vector_mul_tb.update(matrix_vector_mul_tb.matrixRtp[casc + ssr * CASC_LEN],
                                        matrix_vector_mul_tb.m_matrixA[casc + ssr * CASC_LEN][0],
                                        NUM_FRAMES * DIM_A * DIM_B / (UUT_SSR * CASC_LEN));
        }
    }
    matrix_vector_mul_tb.run(NITER / 2);
    matrix_vector_mul_tb.wait();
    for (int ssr = 0; ssr < UUT_SSR; ssr++) {
        for (int casc = 0; casc < CASC_LEN; casc++) {
            matrix_vector_mul_tb.update(matrix_vector_mul_tb.matrixRtp[casc + ssr * CASC_LEN],
                                        matrix_vector_mul_tb.m_matrixA[casc + ssr * CASC_LEN][1],
                                        NUM_FRAMES * DIM_A * DIM_B / (UUT_SSR * CASC_LEN));
        }
    }
    matrix_vector_mul_tb.run(NITER / 2);
#else
    matrix_vector_mul_tb.run(NITER);
#endif

    matrix_vector_mul_tb.end();

    return 0;
}
