#ifndef _DSPLIB_WIDGET_REAL2COMPLEX_GRAPH_HPP_
#define _DSPLIB_WIDGET_REAL2COMPLEX_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Real to Complex Widget library element.
*/
/**
 * @file widget_real2complex_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <tuple>

#include "widget_real2complex.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace real2complex {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// widget_real2complex_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief widget_real2complex is utility to convert real data to complex or vice versa
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function.
 *         This is a typename and must be one of the following:
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_OUT_DATA describes the type of individual data samples output from the function.
 *         This is a typename and must be one of the following:
 *         int16, cint16, int32, cint32, float, cfloat.
 *         TT_OUT_DATA must also be the real or complex counterpart of TT_DATA, e.g.
 *         TT_DATA = int16 and TT_OUT_DATA = cint16 is valid,
 *         TT_DATA = cint16 and TT_OUT_DATA = int16 is valid, but
 *         TT_DATA = int16 and TT_OUT_DATA = cint32 is not valid.
 * @tparam TP_WINDOW_VSIZE describes the number of samples in the window API
 *         used if either input or output is a window.
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 **/
template <typename TT_DATA, typename TT_OUT_DATA, unsigned int TP_WINDOW_VSIZE>
/**
 * This is the class for the Widget API Cast graph
 **/
class widget_real2complex_graph : public graph {
   public:
    static_assert((std::is_same<TT_DATA, cint16>::value && std::is_same<TT_OUT_DATA, int16>::value) ||
                      (std::is_same<TT_DATA, cint32>::value && std::is_same<TT_OUT_DATA, int32>::value) ||
                      (std::is_same<TT_DATA, cfloat>::value && std::is_same<TT_OUT_DATA, float>::value) ||
                      (std::is_same<TT_DATA, int16>::value && std::is_same<TT_OUT_DATA, cint16>::value) ||
                      (std::is_same<TT_DATA, int32>::value && std::is_same<TT_OUT_DATA, cint32>::value) ||
                      (std::is_same<TT_DATA, float>::value && std::is_same<TT_OUT_DATA, cfloat>::value),
                  "ERROR: TT_DATA and TT_OUT_DATA are not a real/complex pair");

    /**
     * The input data to the function. Window API is expected.
     * Data is read from here, converted and written to output.
     **/
    port<input> in;
    /**
     * An API of TT_DATA type.
     **/
    port<output> out;
    /**
      * @cond NOCOMMENTS
      */
    kernel m_kernel;

    // Access function for AIE synthesizer
    /**
      * @endcond
      */

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/

    kernel* getKernels() { return &m_kernel; };

    /**
     * @brief This is the constructor function for the Widget API Cast graph.
     **/
    widget_real2complex_graph() {
        m_kernel = kernel::create_object<widget_real2complex<TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE> >();
        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.
        // Source files
        source(m_kernel) = "widget_real2complex.cpp";

        // make connections
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_DATA)> >(in, m_kernel.in[0]);
        connect<window<TP_WINDOW_VSIZE * sizeof(TT_OUT_DATA)> >(m_kernel.out[0], out);
    }; // constructor
};
}
}
}
}
} // namespace braces

#endif //_DSPLIB_WIDGET_REAL2COMPLEX_GRAPH_HPP_

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
