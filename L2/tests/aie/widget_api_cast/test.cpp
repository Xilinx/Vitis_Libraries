/*
This file holds the body of the test harness for the widger api cast
reference model graph
*/

#include <stdio.h>
#include "test.hpp"

#if (NUM_INPUTS == 1)
#if (NUM_OUTPUT_CLONES == 1)
simulation::platform<1, 1> platform(QUOTE(INPUT_FILE), QUOTE(OUTPUT_FILE));
#elif (NUM_OUTPUT_CLONES == 2)
simulation::platform<1, 2> platform(QUOTE(INPUT_FILE), QUOTE(OUTPUT_FILE), QUOTE(OUTPUT_FILE2));
#elif (NUM_OUTPUT_CLONES == 3)
simulation::platform<1, 3> platform(QUOTE(INPUT_FILE), QUOTE(OUTPUT_FILE), QUOTE(OUTPUT_FILE2), QUOTE(OUTPUT_FILE3));
#elif (NUM_OUTPUT_CLONES == 4)
simulation::platform<1, 4> platform(
    QUOTE(INPUT_FILE), QUOTE(OUTPUT_FILE), QUOTE(OUTPUT_FILE2), QUOTE(OUTPUT_FILE3), QUOTE(OUTPUT_FILE4));
#endif
#elif (NUM_INPUTS == 2)
#if (NUM_OUTPUT_CLONES == 1)
simulation::platform<2, 1> platform(QUOTE(INPUT_FILE), QUOTE(INPUT_FILE2), QUOTE(OUTPUT_FILE));
#elif (NUM_OUTPUT_CLONES == 2)
simulation::platform<2, 2> platform(QUOTE(INPUT_FILE), QUOTE(INPUT_FILE2), QUOTE(OUTPUT_FILE), QUOTE(OUTPUT_FILE2));
#elif (NUM_OUTPUT_CLONES == 3)
simulation::platform<2, 3> platform(
    QUOTE(INPUT_FILE), QUOTE(INPUT_FILE2), QUOTE(OUTPUT_FILE), QUOTE(OUTPUT_FILE2), QUOTE(OUTPUT_FILE3));
#elif (NUM_OUTPUT_CLONES == 4)
simulation::platform<2, 4> platform(QUOTE(INPUT_FILE),
                                    QUOTE(INPUT_FILE2),
                                    QUOTE(OUTPUT_FILE),
                                    QUOTE(OUTPUT_FILE2),
                                    QUOTE(OUTPUT_FILE3),
                                    QUOTE(OUTPUT_FILE4));
#endif
#endif

xf::dsp::aie::testcase::test_graph widgetTestHarness;

// Connect platform to uut
connect<> net_in0(platform.src[0], widgetTestHarness.in[0]);
#if (NUM_INPUTS > 1)
connect<> net_in1(platform.src[1], widgetTestHarness.in[1]);
#endif

connect<> net_out0(widgetTestHarness.out[0], platform.sink[0]);
#if (NUM_OUTPUT_CLONES > 1)
connect<> net_out2(widgetTestHarness.out[1], platform.sink[1]);
#endif
#if (NUM_OUTPUT_CLONES > 2)
connect<> net_out3(widgetTestHarness.out[2], platform.sink[2]);
#endif
#if (NUM_OUTPUT_CLONES > 3)
connect<> net_out4(widgetTestHarness.out[3], platform.sink[3]);
#endif

int main(void) {
    printf("\n");
    printf("========================\n");
    printf("UUT: ");
    printf(QUOTE(UUT_GRAPH));
    printf("\n");
    printf("========================\n");
    printf("Number of samples   = %d \n", WINDOW_VSIZE);
    switch (IN_API) {
        case 0:
            printf("Input API   = window\n");
            break;
        case 1:
            printf("Input API   = stream\n");
            break;
        default:
            printf("Input API unrecognised = %d\n", IN_API);
            break;
    };
    switch (OUT_API) {
        case 0:
            printf("Output API   = window\n");
            break;
        case 1:
            printf("Output API   = stream\n");
            break;
        default:
            printf("Output API unrecognised = %d\n", OUT_API);
            break;
    };
    printf("Data type       = ");
    printf(QUOTE(DATA_TYPE));
    printf("\n");
    printf("NUM_OUTPUT_CLONES     = %d \n", NUM_OUTPUT_CLONES);
    printf("\n");

    widgetTestHarness.init();
    widgetTestHarness.run(NITER);
    widgetTestHarness.end();

    return 0;
}

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

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
