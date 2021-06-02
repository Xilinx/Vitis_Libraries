#ifndef _DSPLIB_WIDGET_API_CAST_REF_GRAPH_HPP_
#define _DSPLIB_WIDGET_API_CAST_REF_GRAPH_HPP_

/*
This file holds the definition of the Widget API Cast Reference model graph.
*/

#include <adf.h>
#include <vector>
#include "widget_api_cast_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {
using namespace adf;

template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES>
class widget_api_cast_ref_graph : public graph {
   public:
    port<input> in[TP_NUM_INPUTS];
    port<output> out[TP_NUM_OUTPUT_CLONES];

    kernel m_kernel;

    // Constructor
    widget_api_cast_ref_graph() {
        printf("============================\n");
        printf("== WIDGET API CAST REF Graph\n");
        printf("============================\n");
        printf("IN_API            = %d\n", TP_IN_API);
        printf("OUT_API           = %d\n", TP_OUT_API);
        printf("NUM_INPUTS        = %d\n", TP_NUM_INPUTS);
        printf("WINDOW_VSIZE      = %d\n", TP_WINDOW_VSIZE);
        printf("NUM_OUTPUT_CLONES = %d\n", TP_NUM_OUTPUT_CLONES);

        m_kernel = kernel::create_object<widget_api_cast_ref<TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS,
                                                             TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES> >();
        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.
        // Source files
        source(m_kernel) = "widget_api_cast_ref.cpp";

        // make connections
        if (TP_IN_API == kWindowAPI) {
            for (int i = 0; i < TP_NUM_INPUTS; i++) {
                connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA)> >(in[i], m_kernel.in[i]);
            }
        } else if (TP_IN_API == kStreamAPI) {
            for (int i = 0; i < TP_NUM_INPUTS; i++) {
                connect<stream>(in[i], m_kernel.in[i]);
            }
        }
        if (TP_OUT_API == kWindowAPI) {
            for (int i = 0; i < TP_NUM_OUTPUT_CLONES; i++) {
                connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA)> >(m_kernel.out[i], out[i]);
            }
        } else if (TP_OUT_API == kStreamAPI) {
            for (int i = 0; i < TP_NUM_OUTPUT_CLONES; i++) {
                connect<stream>(m_kernel.out[i], out[i]);
            }
        }
    };
};
}
}
}
}
}
#endif // _DSPLIB_WIDGET_API_CAST_REF_GRAPH_HPP_

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
