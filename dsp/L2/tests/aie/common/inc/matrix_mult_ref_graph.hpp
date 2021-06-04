#ifndef MATRIX_MULT_REF_HPP
#define MATRIX_MULT_REF_HPP

// This file holds the definition header of thematrix mult reference model graph class

#include <adf.h>
#include <vector>

#include "matrix_mult_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {
using namespace adf;

template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_AB,
          unsigned int TP_DIM_B,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_DIM_A_LEADING = ROW_MAJOR,
          unsigned int TP_DIM_B_LEADING = COL_MAJOR,
          unsigned int TP_DIM_OUT_LEADING = ROW_MAJOR,
          unsigned int TP_ADD_TILING_A = 1,     // not used - just to match UUT.
          unsigned int TP_ADD_TILING_B = 1,     // not used - just to match UUT.
          unsigned int TP_ADD_DETILING_OUT = 1, // not used - just to match UUT.
          unsigned int TP_INPUT_WINDOW_VSIZE_A = TP_DIM_A* TP_DIM_AB,
          unsigned int TP_INPUT_WINDOW_VSIZE_B = TP_DIM_B* TP_DIM_AB,
          unsigned int TP_CASC_LEN = 1 // not used - just to match UUT.
          >
class matrix_mult_ref_graph : public graph {
   public:
    // port<input> in[2];
    port<input> inA;
    port<input> inB;
    port<output> out;

    // FIR Kernel
    kernel m_firKernel;

    // Constructor
    matrix_mult_ref_graph() {
        printf("===========================\n");
        printf("==    MATRIX MULT REF   == \n");
        printf("===========================\n");

        // Create FIR class
        m_firKernel = kernel::create_object<
            matrix_mult_ref<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_AB, TP_DIM_B, TP_SHIFT, TP_RND, TP_DIM_A_LEADING,
                            TP_DIM_B_LEADING, TP_DIM_OUT_LEADING, TP_INPUT_WINDOW_VSIZE_A, TP_INPUT_WINDOW_VSIZE_B> >();
        printf("Created object");
        // Make connections
        // Size of window in Bytes.
        connect<window<TP_INPUT_WINDOW_VSIZE_A * sizeof(TT_DATA_A)> >(inA, m_firKernel.in[0]);
        connect<window<TP_INPUT_WINDOW_VSIZE_B * sizeof(TT_DATA_B)> >(inB, m_firKernel.in[1]);
        connect<window<(TP_INPUT_WINDOW_VSIZE_A / TP_DIM_AB) * (TP_INPUT_WINDOW_VSIZE_B / TP_DIM_AB) *
                       sizeof(outType_t<TT_DATA_A, TT_DATA_B>)> >(m_firKernel.out[0], out);
        printf("connected window");
        // Specify mapping constraints
        runtime<ratio>(m_firKernel) = 0.4;
        printf("entering source");
        // Source files
        source(m_firKernel) = "matrix_mult_ref.cpp";
        printf("finished constructing");
    };
};
}
}
}
}
}
#endif // MATRIX_MULT_REF_HPP

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
