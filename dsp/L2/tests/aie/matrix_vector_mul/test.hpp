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
#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

/*
This file holds the declaration of the test harness graph class for the
matrix_vector_mul graph class.
*/
#include <adf.h>
#include <vector>
#include "utils.hpp"

#include "uut_static_config.h"
#include "uut_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH matrix_vector_mul_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {

class test_graph : public graph {
   private:
    static constexpr int matrixSizePerKernel = DIM_A * DIM_B * NUM_FRAMES / (CASC_LEN * UUT_SSR);
    DATA_A matrixA[matrixSizePerKernel * NITER];

   public:
#ifdef USING_UUT
    port_conditional_array<input, (USE_MATRIX_RELOAD == 1), UUT_SSR * CASC_LEN> matrixRtp;
#endif
    std::array<input_plio, CASC_LEN * UUT_SSR> inA;
    std::array<input_plio, CASC_LEN * UUT_SSR*(DUAL_IP + 1)> inB;
    std::array<output_plio, UUT_SSR * NUM_OUTPUTS> out;
    DATA_A m_matrixA[CASC_LEN * UUT_SSR][2][matrixSizePerKernel];

    // Constructor
    test_graph() {
        printf("==========================\n");
        printf("== UUT Graph Class: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("==========================\n");
        printf("Input samples A    = %d \n", INPUT_SAMPLES_A);
        printf("Input window A [B] = %lu \n", INPUT_SAMPLES_A * sizeof(DATA_A));
        printf("Input samples B    = %d \n", INPUT_SAMPLES_B);
        printf("Input window B [B] = %lu \n", INPUT_SAMPLES_B * sizeof(DATA_B));
        printf("Output samples     = %d \n", OUTPUT_SAMPLES);
        printf("Shift              = %d \n", SHIFT);
        printf("ROUND_MODE         = %d \n", ROUND_MODE);
        printf("SAT_MODE           = %d \n", SAT_MODE);
        printf("Data type          = ");
        printf(QUOTE(DATA_A) QUOTE(DATA_B));
        printf("\n");
        printf("DIM_A              = %d \n", DIM_A);
        printf("DIM_B              = %d \n", DIM_B);
        printf("UUT_SSR            = %d \n", UUT_SSR);
        printf("CASC_LEN           = %d \n", CASC_LEN);
        printf("API_IO           = %d \n", API_IO);
        printf("USE_MATRIX_RELOAD  = %d \n", USE_MATRIX_RELOAD);
        printf("DUAL_IP            = %d \n", DUAL_IP);
        printf("NUM_OUTPUTS        = %d \n", NUM_OUTPUTS);
        printf("\n");
        printf("\n");
        printf("==========================\n");

        namespace dsplib = xf::dsp::aie;
        dsplib::blas::matrix_vector_mul::UUT_GRAPH<DATA_A, DATA_B, DIM_A, DIM_B, SHIFT, ROUND_MODE, NUM_FRAMES,
                                                   CASC_LEN, SAT_MODE, UUT_SSR, DIM_A_LEADING, USE_MATRIX_RELOAD,
                                                   API_IO, DUAL_IP, NUM_OUTPUTS>
            matrix_vector_mulGraph;

#ifdef USING_UUT
        for (int ssr = 0; ssr < UUT_SSR; ssr++) {
            for (int casc = 0; casc < CASC_LEN; casc++) {
                //////////////////////////////////////////////// UUT A
                ///////////////////////////////////////////////////////////////////
                std::string filenameInMatrix = QUOTE(INPUT_FILE_A);
                filenameInMatrix.insert(filenameInMatrix.length() - 4,
                                        ("_" + std::to_string(ssr) + "_" + std::to_string(casc)));

#if (SINGLE_BUF == 1)
                single_buffer(matrix_vector_mulGraph.getKernels()[(ssr * CASC_LEN) + casc].in[0]);
                single_buffer(matrix_vector_mulGraph.getKernels()[(ssr * CASC_LEN) + casc].in[1]);
                printf("INFO: Single Buffer Constraint applied to input buffers of kernel %d.\n",
                       ((ssr * CASC_LEN) + casc));
#endif

// iobuffer A
#if (USE_MATRIX_RELOAD == 0)
                inA[casc + ssr * CASC_LEN] = input_plio::create("PLIO_in_A" + std::to_string(casc + ssr * CASC_LEN),
                                                                adf::plio_64_bits, filenameInMatrix);
                connect<>(inA[casc + ssr * CASC_LEN].out[0], matrix_vector_mulGraph.inA[casc + ssr * CASC_LEN]);
// RTP A
#else
                // Load NITER samples per kernel
                test_stim<DATA_A, matrixSizePerKernel * NITER, 0> taps_gen(filenameInMatrix);
                taps_gen.gen(1, matrixA);

                printf("ssr = %d casc = %d matrixSize = %d\n", ssr, casc, matrixSizePerKernel);
                for (int i = 0; i < matrixSizePerKernel; i++) {
                    m_matrixA[casc + ssr * CASC_LEN][0][i] = matrixA[i];
                }
                for (int i = 0; i < matrixSizePerKernel; i++) {
                    m_matrixA[casc + ssr * CASC_LEN][1][i] = matrixA[(matrixSizePerKernel * NITER / 2) + i];
                }
                connect<>(matrixRtp[casc + ssr * CASC_LEN], matrix_vector_mulGraph.matrixA[casc + ssr * CASC_LEN]);
#endif
                //////////////////////////////////////////////// UUT b
                ////////////////////////////////////////////////////////////////////
                // Streams B (single or dual)
                for (int dualIdx = 0; dualIdx < (DUAL_IP + 1); dualIdx++) {
                    int bPortIdx = (DUAL_IP + 1) * (casc + ssr * CASC_LEN) + dualIdx;
                    std::string filenameInVector = QUOTE(INPUT_FILE_B);
                    printf("Connecting B port %d to kernel %d  ", dualIdx, bPortIdx);
                    filenameInVector.insert(
                        filenameInVector.length() - 4,
                        ("_" + std::to_string(ssr)) + "_" + std::to_string(casc) + "_" + std::to_string(dualIdx));
                    inB[bPortIdx] =
                        input_plio::create("PLIO_in_B" + std::to_string(bPortIdx), adf::plio_64_bits, filenameInVector);
                    connect<>(inB[bPortIdx].out[0], matrix_vector_mulGraph.inB[bPortIdx]);
                }
            }
#if (SINGLE_BUF == 1)
            single_buffer(matrix_vector_mulGraph.getKernels()[(ssr * CASC_LEN) + (CASC_LEN - 1)].out[0]);
            printf("INFO: Single Buffer Constraint applied the output buffer of kernel %d.\n",
                   (ssr * CASC_LEN) + (CASC_LEN - 1));
#endif
        }
        //////////////////////////////////////////////// UUT out////////////////////////////////////////////////////////

        // Streams out (single or dual)
        for (int ssrOut = 0; ssrOut < UUT_SSR; ssrOut++) {
            for (int outIdx = 0; outIdx < NUM_OUTPUTS; outIdx++) {
                int outPortIdx = (NUM_OUTPUTS * ssrOut) + outIdx;

                std::string filenameOut = QUOTE(OUTPUT_FILE);
                filenameOut.insert(filenameOut.length() - 4,
                                   ("_" + std::to_string(ssrOut) + "_" + std::to_string(outIdx)));

                out[outPortIdx] =
                    output_plio::create("PLIO_out_" + std::to_string(outPortIdx), adf::plio_64_bits, filenameOut);
                connect<>(matrix_vector_mulGraph.out[outPortIdx], out[outPortIdx].in[0]);
            }
        }
//////////////////////////////////////////////// Ref ////////////////////////////////////////////////////////////////

// using ref
#else
        std::string filenameInMatrix = QUOTE(INPUT_FILE_A);
        std::string filenameInVector = QUOTE(INPUT_FILE_B);
        // Make connections
        inA[0] = input_plio::create("PLIO_in_A" + std::to_string(0), adf::plio_64_bits, filenameInMatrix);
        inB[0] = input_plio::create("PLIO_in_B" + std::to_string(0), adf::plio_64_bits, filenameInVector);
        connect<>(inA[0].out[0], matrix_vector_mulGraph.inA[0]);
        connect<>(inB[0].out[0], matrix_vector_mulGraph.inB[0]);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        // filenameOut.insert(filenameOut.length()-4, ("_0_0"));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(matrix_vector_mulGraph.out[0], out[0].in[0]);
#endif
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_
