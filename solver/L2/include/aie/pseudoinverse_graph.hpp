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

#ifndef _XF_SOLVER_PSEUDOINVERSE_GRAPH_HPP_
#define _XF_SOLVER_PSEUDOINVERSE_GRAPH_HPP_

#include "matrix_mult_graph.hpp"

namespace xf {
namespace solver {

using namespace adf;

#define SPLIT 16
#define NUM_KERNEL 8
#define CASC_LN 4
#define N_SAMPLES 1
#define P_DIM_A 64
#define P_DIM_B 64
#define P_DIM_AB 64
#define DIM_A (P_DIM_A / SPLIT)
#define DIM_AB P_DIM_AB
#define DIM_B (P_DIM_B / SPLIT)

#define WINDOW_SIZE_A (DIM_A * DIM_AB * N_SAMPLES)
#define WINDOW_SIZE_B (DIM_B * DIM_AB * N_SAMPLES)

class PseudoInverseComplexFloat : public adf::graph {
   public:
    input_plio matA_inp[CASC_LN];
    input_plio matB_inp[(NUM_KERNEL * CASC_LN)];
    output_plio matC_out[NUM_KERNEL];

    PseudoInverseComplexFloat() {
        // GeMM Graph Declarations...
        xf::dsp::aie::blas::matrix_mult::matrix_mult_graph<cfloat, cfloat, DIM_A, DIM_AB, DIM_B, 0, 0, ROW_MAJOR,
                                                           ROW_MAJOR, ROW_MAJOR, 0, 0, 0, WINDOW_SIZE_A, WINDOW_SIZE_B,
                                                           CASC_LN>
            mmult[NUM_KERNEL];

        std::string aiesim_data_file = "aiesim_data/gemm_" + std::to_string(P_DIM_A) + 'x' + std::to_string(P_DIM_AB) +
                                       'x' + std::to_string(P_DIM_B) + "_ioFiles/";
        // Mat A PLIO node names...
        for (int j = 0; j < CASC_LN; ++j) {
            std::string matA_plioOut_str = "DataInA" + std::to_string(0) + "_CASC" + std::to_string(j);
            const char* matA_plioOut = matA_plioOut_str.c_str();

            std::string matA_Out_file_str =
                aiesim_data_file + "a" + std::to_string(0) + "_casc" + std::to_string(j) + ".txt";
            const char* matA_Out_file = matA_Out_file_str.c_str();

            matA_inp[j] = input_plio::create(matA_plioOut, plio_128_bits, matA_Out_file);
        }

        for (int i = 0; i < NUM_KERNEL; ++i) {
            // CASC_LN No. of kernels will be created...
            adf::kernel* mmult_kernels = mmult[i].getKernels();

            for (int j = 0; j < CASC_LN; ++j) {
                // Mat B PLIO node names...
                std::string matB_plioOut_str = "DataInB" + std::to_string(i) + "_CASC" + std::to_string(j);
                const char* matB_plioOut = matB_plioOut_str.c_str();

                std::string matB_Out_file_str =
                    aiesim_data_file + "b" + std::to_string(i) + "_casc" + std::to_string(j) + ".txt";
                const char* matB_Out_file = matB_Out_file_str.c_str();

                matB_inp[(i * CASC_LN) + j] = input_plio::create(matB_plioOut, plio_128_bits, matB_Out_file);
            }

            // Mat C PLIO node names...
            std::string matC_plioOut_str = "DataOutC" + std::to_string(i);
            const char* matC_plioOut = matC_plioOut_str.c_str();

            std::string matC_Out_file_str = "data/c" + std::to_string(i) + ".txt";
            const char* matC_Out_file = matC_Out_file_str.c_str();

            // Creating PLIO nodes...
            matC_out[i] = output_plio::create(matC_plioOut, plio_128_bits, matC_Out_file);

            // Connecting PLIO Nodes...
            for (int k = 0; k < CASC_LN; ++k) {
                // Setting runtime ratio...
                adf::runtime<ratio>(mmult_kernels[k]) = 1.0;

                // Connecting port IO nodes...
                adf::connect<>(matA_inp[k].out[0], mmult[i].inA[k]);
                adf::connect<>(matB_inp[(i * CASC_LN) + k].out[0], mmult[i].inB[k]);
            }

            // Connecting port IO nodes...
            adf::connect<>(mmult[i].out[0], matC_out[i].in[0]);
        }
    }
};

} // namespace solver
} // namespace xf

#endif
