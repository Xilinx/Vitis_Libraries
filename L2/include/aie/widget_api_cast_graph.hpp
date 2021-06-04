#ifndef _DSPLIB_WIDGET_API_CAST_GRAPH_HPP_
#define _DSPLIB_WIDGET_API_CAST_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Single Rate Asymmetrical FIR library element.
*/
/**
 * @file widget_api_cast_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <tuple>

#include "widget_api_cast.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// widget_api_cast_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief widget_api_cast is a Asymmetric Single Rate FIR filter
 *
 * These are the templates to configure the Asymmetric Single Rate FIR class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the function. This is a typename and must be one
 *         of the following:
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TP_WINDOW_VSIZE describes the number of samples in the window API
 *         used if either input or output is a window.
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_NUM_OUTPUT_CLONES sets the number of output ports to write the input
 *         data to. Note that while input data from multiple ports is independent,
 *         data out is not.
 **/
template <typename TT_DATA,
          unsigned int TP_IN_API, // 0 = Window, 1 = Stream
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES = 1>
/**
 * This is the class for the Widget API Cast graph
 **/
class widget_api_cast_graph : public graph {
   public:
    static_assert(TP_IN_API == kStreamAPI || TP_IN_API == kWindowAPI,
                  "Error: TP_IN_API is not a supported value of 0 (window) or 1 (stream)");
    static_assert(TP_OUT_API == kStreamAPI || TP_OUT_API == kWindowAPI,
                  "Error: TP_OUT_API is not a supported value of 0 (window) or 1 (stream)");
    static_assert(!(TP_IN_API == kWindowAPI && TP_NUM_INPUTS > 1),
                  "Error: Only a single input is supported for an input API of Window");
    static_assert(TP_NUM_INPUTS >= 1 && TP_NUM_INPUTS <= 2, "Error: TP_NUM_INPUTS is out of the supported range");
    static_assert(TP_NUM_OUTPUT_CLONES >= 1 && TP_NUM_OUTPUT_CLONES <= 4,
                  "Error: TP_NUM_OUTPUT_CLONES is out of the supported range");
    static_assert(TP_IN_API == 1 || TP_NUM_OUTPUT_CLONES < 4,
                  "Error: TP_NUM_OUTPUT_CLONES may only be 4 when stream input is configured.");
    static_assert(TP_IN_API != 1 || TP_OUT_API != 1, "Error: stream to stream connection is not supported");
    static_assert(TP_IN_API != 0 || TP_OUT_API != 1 || TP_NUM_INPUTS == 1,
                  "Error: Window to stream supports only a single connection in.");
    /**
     * The input data to the function. This input may be stream or window.
     * Data is read from here and written directly to output. When there are
     * multiple input streams, a read from each will occur of the maximum size
     * supported (32 bits) with these 2 being
     * concatenated, before being written to the output(s). Multiple input
     * windows is not supported
     **/
    port<input> in[TP_NUM_INPUTS];
    /**
     * An API of TT_DATA type.
     **/
    port<output> out[TP_NUM_OUTPUT_CLONES];
    /**
      * @cond NOCOMMENTS
      */
    kernel m_kernel;

    // Access function for AIE synthesizer
    kernel* getKernels() { return &m_kernel; };

    /**
      * @endcond
      */

    /**
     * @brief This is the constructor function for the Widget API Cast graph.
     **/
    widget_api_cast_graph() {
        m_kernel = kernel::create_object<
            widget_api_cast<TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES> >();

        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.

        // Source files
        source(m_kernel) = "widget_api_cast.cpp";

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
    }; // constructor
};
}
}
}
}
} // namespace braces

#endif //_DSPLIB_WIDGET_API_CAST_GRAPH_HPP_

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
