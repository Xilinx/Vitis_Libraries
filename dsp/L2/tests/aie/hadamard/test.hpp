#ifndef _DSPLIB_TEST_HPP_
#define _DSPLIB_TEST_HPP_

// This file holds the definition of the test harness for the hadamard graph.

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
#define UUT_GRAPH hadamard_graph
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
    std::array<input_plio, UUT_SSR> inA;
    std::array<input_plio, UUT_SSR> inB;
    std::array<output_plio, UUT_SSR> out;

    // Constructor
    test_graph() {
        printf("========================\n");
        printf("== Hadamard test.hpp parameters: ");
        printf(QUOTE(UUT_GRAPH));
        printf("\n");
        printf("========================\n");
        printf("Data type  A       = ");
        printf(QUOTE(T_DATA_A));
        printf("\n");
        printf("Data type  B       = ");
        printf(QUOTE(T_DATA_B));
        printf("\n");
        printf("DIM_SIZE          = %d \n", DIM_SIZE);
        printf("NUM_FRAMES        = %d \n", NUM_FRAMES);
        printf("SHIFT             = %d \n", SHIFT);
        printf("API_IO            = %d \n", API_IO);
        printf("SSR               = %d \n", UUT_SSR);
        printf("ROUND_MODE        = %d \n", ROUND_MODE);
        printf("SAT_MODE          = %d \n", SAT_MODE);

        // Hadamard sub-graph
        dsplib::hadamard::UUT_GRAPH<T_DATA_A, T_DATA_B, DIM_SIZE, NUM_FRAMES, SHIFT, API_IO, UUT_SSR, ROUND_MODE,
                                    SAT_MODE>
            hadamardGraph;

        // Make connections
        for (int i = 0; i < UUT_SSR; i++) {
            std::string filenameInA = QUOTE(INPUT_FILE_A);
            std::string filenameInB = QUOTE(INPUT_FILE_B);
            filenameInA.insert(filenameInA.length() - 4, ("_" + std::to_string(i) + "_0"));
            filenameInB.insert(filenameInB.length() - 4, ("_" + std::to_string(i) + "_0"));
            inA[i] = input_plio::create("PLIO_inA_" + std::to_string(i), adf::plio_32_bits, filenameInA);
            inB[i] = input_plio::create("PLIO_inB_" + std::to_string(i), adf::plio_32_bits, filenameInB);
            connect<>(inA[i].out[0], hadamardGraph.inA[i]);
            connect<>(inB[i].out[0], hadamardGraph.inB[i]);

            std::string filenameOut = QUOTE(OUTPUT_FILE);
            filenameOut.insert(filenameOut.length() - 4, ("_" + std::to_string(i) + "_0"));
            out[i] = output_plio::create("PLIO_out_" + std::to_string(i), adf::plio_32_bits, filenameOut);
            connect<>(hadamardGraph.out[i], out[i].in[0]);
        }

        printf("========================\n");
    };
};
}
}
}
};

#endif // _DSPLIB_TEST_HPP_

/*  (c) Copyright 2022 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
