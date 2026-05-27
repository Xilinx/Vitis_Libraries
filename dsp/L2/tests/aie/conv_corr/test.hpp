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

// This file holds the definition of the test harness for the conv_corr graph.

#include <adf.h>
#include <vector>
#include <array>
#include <cstdlib>
#include "utils.hpp"

#include "uut_config.h"
#include "uut_static_config.h"
#include "test_stim.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#ifndef UUT_GRAPH
#define UUT_GRAPH conv_corr_graph
#endif

#ifndef SEED_RTP
#define SEED_RTP 1
#endif

#include QUOTE(UUT_GRAPH.hpp)

using namespace adf;
using xf::dsp::aie::conv_corr::getMinLen;
using xf::dsp::aie::conv_corr::getMaxLen;

// Returns a non-power-of-2 F length that is a valid multiple of vecSize,
// spread across [3*vecSize, TP_F_LEN] using a half-range stride between successive calls.
template <typename TT_DATA_F, unsigned int TP_F_LEN>
unsigned int getRtpFLen() {
    constexpr unsigned int minFLen = getMinLen<TT_DATA_F>();
    constexpr unsigned int maxFLen = MIN(TP_F_LEN, getMaxLen<TT_DATA_F>());
    constexpr unsigned int vecSize = minFLen >> 1;            // elements per single vector load
    constexpr unsigned int lenStep = minFLen;                 // 2*vecSize: step over even multiples of vecSize
    constexpr unsigned int minNonPow2Len = minFLen + vecSize; // 3*vecSize: first odd multiple of vecSize >= minFLen
    constexpr unsigned int numLens = (maxFLen >= minNonPow2Len) ? (maxFLen - minNonPow2Len) / lenStep + 1u : 0u;
    if
        constexpr(numLens == 0u) {
            return getMinLen<TT_DATA_F>(); // TP_F_LEN < 3*vecSize: no non-power-of-2 length exists
        }
    else {
        constexpr unsigned int stride = numLens / 2u + 1u; // half-range jump alternates halves
        static unsigned int startIdx = rand() % numLens;   // random start, set once
        static unsigned int callIdx = 0;
        return minNonPow2Len + lenStep * ((startIdx + callIdx++ * stride) % numLens);
    }
}

// Returns a non-power-of-2 G length that is a valid multiple of vecSize,
// spread across [3*vecSize, TP_G_LEN] using a half-range stride between successive calls.
template <typename TT_DATA_G, unsigned int TP_G_LEN>
unsigned int getRtpGLen() {
    constexpr unsigned int minGLen = getMinLen<TT_DATA_G>();
    constexpr unsigned int maxGLen = MIN(TP_G_LEN, getMaxLen<TT_DATA_G>());
    constexpr unsigned int vecSize = minGLen >> 1;            // elements per single vector load
    constexpr unsigned int lenStep = minGLen;                 // 2*vecSize: step over even multiples of vecSize
    constexpr unsigned int minNonPow2Len = minGLen + vecSize; // 3*vecSize: first odd multiple of vecSize >= minGLen
    constexpr unsigned int numLens = (maxGLen >= minNonPow2Len) ? (maxGLen - minNonPow2Len) / lenStep + 1u : 0u;
    if
        constexpr(numLens == 0u) {
            return getMinLen<TT_DATA_G>(); // TP_G_LEN < 3*vecSize: no non-power-of-2 length exists
        }
    else {
        constexpr unsigned int stride = numLens / 2u + 1u; // half-range jump alternates halves
        static unsigned int startIdx = rand() % numLens;   // random start, set once
        static unsigned int callIdx = 0;
        return minNonPow2Len + lenStep * ((startIdx + callIdx++ * stride) % numLens);
    }
}

namespace xf {
namespace dsp {
namespace aie {
namespace testcase {
namespace dsplib = xf::dsp::aie;

class test_graph : public graph {
   public:
    std::array<input_plio, PHASES> inWindowF;
    std::array<input_plio, 1> inWindowG;
    std::array<output_plio, PHASES> outWindow;

    // Conv_Corr sub-graph instance (class member so update_rtp() and connections can access it)
    dsplib::conv_corr::UUT_GRAPH<DATA_F,
                                 DATA_G,
                                 DATA_OUT,
                                 FUNCT_TYPE,
                                 COMPUTE_MODE,
                                 REF_F_LEN,
                                 G_LEN,
                                 SHIFT,
                                 API_IO,
                                 RND,
                                 SAT,
                                 NUM_FRAMES,
                                 CASC_LEN,
                                 PHASES,
                                 USE_RTP_VECTOR_LENGTHS>
        conv_corrGraph;

#if (USE_RTP_VECTOR_LENGTHS == 1)
    port_conditional_array<input, (USE_RTP_VECTOR_LENGTHS == 1), 1> rtpVecLen;
#endif

    // Constructor
    test_graph() {
        printf("=============================================================\n");
        printf("   conv_corr test.hpp parameters:    ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("=============================================================\n");
        printf("Data type of F Sig.  = ");
        printf(QUOTE(DATA_F));
        printf("\n");
        printf("Data type of G Sig.  = ");
        printf(QUOTE(DATA_G));
        printf("\n");
        printf("Data type of Conv/Corr Output.  = ");
        printf(QUOTE(DATA_OUT));
        printf("\n");
        printf("FUNCT_TYPE     = %d \n", FUNCT_TYPE);
        printf("COMPUTE_MODE   = %d \n", COMPUTE_MODE);
        printf("F_LEN          = %d \n", REF_F_LEN);
        printf("G_LEN          = %d \n", G_LEN);
        printf("SHIFT          = %d \n", SHIFT);
        printf("API            = %d \n", API_IO);
        printf("RND            = %d \n", RND);
        printf("SAT            = %d \n", SAT);
        printf("NUM_FRAMES     = %d \n", NUM_FRAMES);
        printf("CASC_LEN       = %d \n", CASC_LEN);
        printf("PHASES         = %d \n", PHASES);
        printf("USE_RTP_VECTOR_LENGTHS = %d \n", USE_RTP_VECTOR_LENGTHS);

// Make connections:
#ifdef USING_UUT
        for (int i = 0; i < PHASES; i++) {
            std::string inpFile_F_Sig = QUOTE(INPUT_FILE_F);
            inpFile_F_Sig.insert(inpFile_F_Sig.length() - 4, ("_" + std::to_string(i)));
            inWindowF[i] =
                input_plio::create("PLIO_inpFile_F_Sig" + std::to_string(i), adf::plio_64_bits, inpFile_F_Sig);
            connect<>(inWindowF[i].out[0], conv_corrGraph.inF[i]);

            std::string OutFile = QUOTE(OUTPUT_FILE);
            OutFile.insert(OutFile.length() - 4, ("_" + std::to_string(i) + "_0"));
            outWindow[i] = output_plio::create("PLIO_OutFile" + std::to_string(i), adf::plio_64_bits, OutFile);
            connect<>(conv_corrGraph.out[i], outWindow[i].in[0]);
        }
#if (SINGLE_BUF == 1)
#if (API_IO == 0)
        single_buffer(conv_corrGraph.getKernels()[0].in[0]);
        single_buffer(conv_corrGraph.getKernels()[0].in[1]);
        single_buffer(conv_corrGraph.getKernels()[0].out[0]);
        printf("INFO: Single Buffer Constraint applied to input and output buffers of kernels.\n");
#else
        for (int i = 0; i < PHASES; i++) {
            single_buffer(conv_corrGraph.m_conv_corr[i][0].in[2]);
            printf("INFO: Single Buffer Constraint applied to input-G buffer of the kernel %d.\n", i);
        }
#endif // API
#endif // SINGLE_BUF

#else
        std::string inpFile_F_Sig = QUOTE(INPUT_FILE_F);
        inWindowF[0] = input_plio::create("PLIO_inpFile_F_Sig", adf::plio_64_bits, inpFile_F_Sig);
        connect<>(inWindowF[0].out[0], conv_corrGraph.inF[0]);

        std::string OutFile = QUOTE(OUTPUT_FILE);
        OutFile.insert(OutFile.length() - 4, ("_0_0"));
        outWindow[0] = output_plio::create("PLIO_OutFile", adf::plio_64_bits, OutFile);
        connect<>(conv_corrGraph.out[0], outWindow[0].in[0]);
#endif

        std::string inpFile_G_Sig = QUOTE(INPUT_FILE_G);
        inWindowG[0] = input_plio::create("PLIO_inpFile_G_Sig", adf::plio_64_bits, inpFile_G_Sig);
        connect<>(inWindowG[0].out[0], conv_corrGraph.inG);

#if (USE_RTP_VECTOR_LENGTHS == 1)
        connect<parameter>(rtpVecLen[0], conv_corrGraph.rtpVecLen[0]);
#endif

        printf("==================================\n");
        printf("======= End of sub-graph ======\n");
        printf("==================================\n");
    };
};

} // namespace testcase
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_TEST_HPP_
