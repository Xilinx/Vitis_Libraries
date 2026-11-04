/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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
#ifndef _TEST_HPP_
#define _TEST_HPP_

#include <adf.h>
#include <string>
#include <array>
#include <cstdio>
#include "uut_config.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH ls_cholesky_solver_graph
#endif

#if defined(USING_UUT) && USING_UUT == 1
#include "ls_cholesky_solver_native_generated_graph/ls_cholesky_solver_generated_graph.h"
#else
#include QUOTE(UUT_GRAPH.hpp)
#endif

#ifndef USING_UUT
#define USING_UUT 1
#endif

using namespace adf;

namespace xf::solver::aie::test {

class test_graph : public adf::graph {
   public:
#if USING_UUT == 1
    static constexpr unsigned int kNumPortKernels = GRID_DIM * (GRID_DIM + 1) / 2;

    // Input/output PLIO ports
    std::array<input_plio, kNumPortKernels> plio_in_A;
    std::array<input_plio, kNumPortKernels> plio_in_y;
    std::array<output_plio, GRID_DIM> plio_out_x;

    test_graph() {
        printf("========================\n");
        printf("== ls_cholesky_solver test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type      = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("DIM_SIZE       = %d\n", DIM_SIZE);
        printf("GRID_DIM       = %d\n", GRID_DIM);
        printf("CASC_LEN       = %d\n", CASC_LEN);
        printf("NUM_FRAMES     = %d\n", NUM_FRAMES);
        printf("NITER          = %d\n", NITER);
        printf("kNumPortKernels= %d\n", kNumPortKernels);

        // Instantiate the ls_cholesky_solver graph
        ls_cholesky_solver_native_generated_graph dut_graph;

        // Wire PLIO ports
        int portIdx = 0;
        for (int gy = 0; gy < GRID_DIM; gy++) {
            for (int gx = 0; gx <= gy; gx++) {
                // A matrix tile input (named _gx_gy, column-major tiling)
                std::string filenameA = QUOTE(INPUT_FILE_A);
                filenameA.insert(filenameA.length() - 4, ("_" + std::to_string(gx) + "_" + std::to_string(gy)));
                plio_in_A[portIdx] =
                    input_plio::create("PLIO_in_A_" + std::to_string(portIdx), adf::plio_64_bits, filenameA);
                connect<>(plio_in_A[portIdx].out[0], dut_graph.A_in[portIdx]);

                // y vector slice input (named _gx_0; same slice replicated to all
                // kernels in the same column gx of the substitution grid)
                std::string filenameY = QUOTE(INPUT_FILE_Y);
                filenameY.insert(filenameY.length() - 4, ("_" + std::to_string(gx) + "_0"));
                plio_in_y[portIdx] =
                    input_plio::create("PLIO_in_Y_" + std::to_string(portIdx), adf::plio_64_bits, filenameY);
                connect<>(plio_in_y[portIdx].out[0], dut_graph.y_in[portIdx]);

                portIdx++;
            }
        }

        // x vector output — one slice per diagonal tile
        for (int gy = 0; gy < GRID_DIM; gy++) {
            std::string filenameX = QUOTE(OUTPUT_FILE);
            filenameX.insert(filenameX.length() - 4, ("_" + std::to_string(gy) + "_0"));
            plio_out_x[gy] = output_plio::create("PLIO_out_X_" + std::to_string(gy), adf::plio_64_bits, filenameX);
            connect<>(dut_graph.x_out[gy], plio_out_x[gy].in[0]);
        }
    }

#else // USING_UUT == 0 : reference model

    // Single-port interface for the reference graph.
    // The ref graph takes the full (non-tiled) A matrix and y vector so that
    // the reference kernel can operate on the complete problem without needing
    // to reassemble tiles.
    input_plio plio_in_A;
    input_plio plio_in_y;
    output_plio plio_out_x;

    test_graph() {
        printf("========================\n");
        printf("== ls_cholesky_solver REF test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type      = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("DIM_SIZE       = %d\n", DIM_SIZE);
        printf("NUM_FRAMES     = %d\n", NUM_FRAMES);
        printf("NITER          = %d\n", NITER);

        // Instantiate the reference graph (single kernel, full matrix/vector)
        xf::solver::aie::ls_cholesky_solver::UUT_GRAPH<DATA_TYPE, DIM_SIZE, NUM_FRAMES> dut_graph;

        // Full (non-tiled) A matrix input
        plio_in_A = input_plio::create("PLIO_in_A_0", adf::plio_64_bits, QUOTE(INPUT_FILE_A));
        connect<>(plio_in_A.out[0], dut_graph.A_in);

        // Full y vector input
        plio_in_y = input_plio::create("PLIO_in_Y_0", adf::plio_64_bits, QUOTE(INPUT_FILE_Y));
        connect<>(plio_in_y.out[0], dut_graph.y_in);

        // Full x solution output — written directly to OUTPUT_FILE (no per-port suffix)
        plio_out_x = output_plio::create("PLIO_out_X_0", adf::plio_64_bits, QUOTE(OUTPUT_FILE));
        connect<>(dut_graph.x_out, plio_out_x.in[0]);
    }

#endif // USING_UUT
};

} // namespace xf::solver::aie::test

#endif // _TEST_HPP_
