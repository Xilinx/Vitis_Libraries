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
#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

// This file holds the definition of the test harness for the bitonic_sort graph.

#include <adf.h>
#include <vector>
#include <array>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#include "device_defs.h"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH bitonic_sort_graph
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
namespace dsplib = xf::dsp::aie;

class test_graph : public graph {
   public:
    std::array<input_plio, UUT_SSR> in;
    std::array<output_plio, 1> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== Bitonic Sort test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type         = ");
        printf(QUOTE(DATA_TYPE));
        printf("\n");
        printf("DIM_SIZE          = %d \n", DIM_SIZE);
        printf("NUM_FRAMES        = %d \n", NUM_FRAMES);
        printf("ASCENDING         = %d \n", ASCENDING);
        printf("CASC_LEN          = %d \n", CASC_LEN);
        printf("UUT_SSR          = %d \n", UUT_SSR);
        printf("========================\n");

#ifdef USING_UUT
        // Bitonic Sort sub-graph
        dsplib::bitonic_sort::UUT_GRAPH<DATA_TYPE, DIM_SIZE, NUM_FRAMES, ASCENDING, CASC_LEN, UUT_SSR>
            bitonic_sortGraph;
        for (int ssr = 0; ssr < UUT_SSR; ssr++) {
            // Make connections
            std::string filenameIn = QUOTE(INPUT_FILE);
            filenameIn.insert(filenameIn.length() - 4, ("_" + std::to_string(ssr) + "_" + std::to_string(0)));
            in[ssr] = input_plio::create("PLIO_in_" + std::to_string(ssr), adf::plio_64_bits, filenameIn);
            connect<>(in[ssr].out[0], bitonic_sortGraph.in[ssr]);
        }
        std::string filenameOut = QUOTE(OUTPUT_FILE);
        // filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(0)));
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(bitonic_sortGraph.out[0], out[0].in[0]);
#if (SINGLE_BUF == 1)
        for (int casc = 0; casc < CASC_LEN; casc++) {
#if (UUT_SSR == 1)
            single_buffer(bitonic_sortGraph.m_kernels[casc].in[0]);
            single_buffer(bitonic_sortGraph.m_kernels[casc].out[0]);
            printf("INFO: Single Buffer Constraint applied to input and output buffers of the kernel %d.\n", (casc));
#endif
#if (UUT_SSR == 2)
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.m_kernels[casc].in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.m_kernels[casc].out[0]);
            printf("INFO: Single Buffer Constraint applied to input and output buffers of the subframe0/kernel %d.\n",
                   (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.m_kernels[casc].in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.m_kernels[casc].out[0]);
            printf("INFO: Single Buffer Constraint applied to input and output buffers of the subframe1/kernel %d.\n",
                   (casc));

#endif
#if (UUT_SSR == 4)
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.m_kernels[casc].in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.m_kernels[casc].out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the subframe0/subframe0/kernel "
                "%d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.m_kernels[casc].in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.m_kernels[casc].out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the subframe0/subframe1/kernel "
                "%d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.m_kernels[casc].in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.m_kernels[casc].out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the subframe1/subframe0/kernel "
                "%d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.m_kernels[casc].in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.m_kernels[casc].out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the subframe1/subframe1/kernel "
                "%d.\n",
                (casc));

#endif
#if (UUT_SSR == 8)
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe0/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe0/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe1/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe1/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe0/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe0/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe1/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe1/subframe1/kernel %d.\n",
                (casc));
#endif
#if (UUT_SSR == 16)
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe0/subframe0/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe0/subframe0/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe0/subframe1/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe0/subframe1/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe1/subframe0/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe1/subframe0/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe1/subframe1/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe0.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe0/subframe1/subframe1/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe0/subframe0/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe0
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe0/subframe0/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe0/subframe1/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe0.bitonic_merge_subframe1
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe0/subframe1/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe1/subframe0/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe0
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe1/subframe0/subframe1/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .bitonic_merge_subframe0.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe1/subframe1/subframe0/kernel %d.\n",
                (casc));

            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .in[0]);
            single_buffer(bitonic_sortGraph.bitonic_merge_subframe1.bitonic_merge_subframe1.bitonic_merge_subframe1
                              .bitonic_merge_subframe1.m_kernels[casc]
                              .out[0]);
            printf(
                "INFO: Single Buffer Constraint applied to input and output buffers of the "
                "subframe1/subframe1/subframe1/subframe1/kernel %d.\n",
                (casc));

#endif
        }
#endif

#else
        // Bitonic Sort sub-graph
        dsplib::bitonic_sort::UUT_GRAPH<DATA_TYPE, DIM_SIZE, NUM_FRAMES, ASCENDING, 1, 1> bitonic_sortGraph;

        std::string filenameIn = QUOTE(INPUT_FILE);
        in[0] = input_plio::create("PLIO_in_" + std::to_string(0), adf::plio_64_bits, filenameIn);
        connect<>(in[0].out[0], bitonic_sortGraph.in);

        std::string filenameOut = QUOTE(OUTPUT_FILE);
        out[0] = output_plio::create("PLIO_out_" + std::to_string(0), adf::plio_64_bits, filenameOut);
        connect<>(bitonic_sortGraph.out, out[0].in[0]);
#endif
    };
};
} // namespace testcase
} // namespace aie
} // namespace dsp
}; // namespace xf

#endif // _DSPLIB_TEST_HPP_