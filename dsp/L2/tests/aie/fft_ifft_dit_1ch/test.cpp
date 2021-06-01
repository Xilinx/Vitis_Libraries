
/*
This file is the test harness for the fft_ifft_dit_1ch graph class.
*/

#include <stdio.h>
#include "test.hpp"

simulation::platform<1, 1> platform(QUOTE(INPUT_FILE), QUOTE(OUTPUT_FILE));

xf::dsp::aie::testcase::test_graph fft;

connect<> net0(platform.src[0], fft.in);
connect<> net1(fft.out, platform.sink[0]);

int main(void) {
    printf("\n");
    printf("========================\n");
    printf("UUT: ");
    printf(QUOTE(UUT_GRAPH));
    printf("\n");
    printf("========================\n");
    printf("Input samples      = %d \n", INPUT_SAMPLES);
    printf("Output samples     = %d \n", OUTPUT_SAMPLES);
    printf("Point Size         = %d \n", POINT_SIZE);
    printf("FFT/nIFFT          = %d \n", FFT_NIFFT);
    printf("Shift              = %d \n", SHIFT);
    printf("Kernels            = %d \n", CASC_LEN);
    printf("Dynamic point size = %d \n", DYN_PT_SIZE);
    printf("Window Size        = %d \n", WINDOW_VSIZE);
    printf("Data type          = ");
    printf(QUOTE(DATA_TYPE));
    printf("\n");
    printf("Twiddle type       = ");
    printf(QUOTE(TWIDDLE_TYPE));
    printf("\n");

    fft.init();
    fft.run(NITER);
    fft.end();

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
