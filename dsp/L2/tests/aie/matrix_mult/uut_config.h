// This file holds constants defining default values for the configuration
// of the parameterized graph class.
// This file further was required to capture an extern declaration
// of the specific class defined by these particular values of the generic
// class, as this was required before aiecompiler supported generic
// class definitions.
//------------------------------------------------------------------------------
// UUT CONFIGURATION
#ifndef T_DATA_A
#define T_DATA_A cint16
#endif
#ifndef T_DATA_B
#define T_DATA_B cint16
#endif
#ifndef P_DIM_A
#define P_DIM_A 16
#endif
#ifndef P_DIM_AB
#define P_DIM_AB 16
#endif
#ifndef P_DIM_B
#define P_DIM_B 16
#endif
#ifndef P_SHIFT
#define P_SHIFT 16
#endif
#ifndef P_ROUND_MODE
#define P_ROUND_MODE 0
#endif
#ifndef P_DIM_A_LEADING
#define P_DIM_A_LEADING 0
#endif
#ifndef P_DIM_B_LEADING
#define P_DIM_B_LEADING 1
#endif
#ifndef P_DIM_OUT_LEADING
#define P_DIM_OUT_LEADING 0
#endif
#ifndef P_ADD_TILING_A
#define P_ADD_TILING_A 1
#endif
#ifndef P_ADD_TILING_B
#define P_ADD_TILING_B 1
#endif
#ifndef P_ADD_DETILING_OUT
#define P_ADD_DETILING_OUT 1
#endif
#ifndef P_INPUT_WINDOW_VSIZE_A
#define P_INPUT_WINDOW_VSIZE_A P_DIM_A* P_DIM_AB
#endif
#ifndef P_INPUT_WINDOW_VSIZE_B
#define P_INPUT_WINDOW_VSIZE_B P_DIM_B* P_DIM_AB
#endif
#ifndef INPUT_FILE_A
#define INPUT_FILE_A "data/inputA.txt"
#endif
#ifndef INPUT_FILE_B
#define INPUT_FILE_B "data/inputB.txt"
#endif
#ifndef OUTPUT_FILE
#define OUTPUT_FILE "data/output.txt"
#endif
#ifndef REF_OUTPUT_FILE
#define REF_OUTPUT_FILE "data/ref_output.txt"
#endif

#ifndef NITER
#define NITER 1
#endif

#ifndef P_CASC_LEN
#define P_CASC_LEN 1
#endif

#define P_INPUT_SAMPLES_A P_INPUT_WINDOW_VSIZE_A* NITER
#define P_INPUT_SAMPLES_B P_INPUT_WINDOW_VSIZE_B* NITER

#define P_OUTPUT_SAMPLES P_INPUT_WINDOW_VSIZE_A / P_DIM_AB* P_INPUT_WINDOW_VSIZE_B / P_DIM_AB* NITER

// END OF UUT CONFIGURATION
//------------------------------------------------------------------------------
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
