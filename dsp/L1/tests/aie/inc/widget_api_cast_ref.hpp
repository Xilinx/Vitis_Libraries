#ifndef _DSPLIB_widget_api_cast_REF_HPP_
#define _DSPLIB_widget_api_cast_REF_HPP_

/*
Widget API Cast reference model
*/

#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"

#define _DSPLIB_WIDGET_API_CAST_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {

static constexpr unsigned int kWindowAPI = 0;
static constexpr unsigned int kStreamAPI = 1;

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, // type of data input and output
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES>
class widget_api_cast_ref {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_window<TT_DATA>* inWindow0, output_window<TT_DATA>* outWindow0);
};

// window to window, 1 to 2
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_window<TT_DATA>* inWindow0,
                      output_window<TT_DATA>* outWindow0,
                      output_window<TT_DATA>* outWindow1);
};

// window to window, 1 to 3
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_window<TT_DATA>* inWindow0,
                      output_window<TT_DATA>* outWindow0,
                      output_window<TT_DATA>* outWindow1,
                      output_window<TT_DATA>* outWindow2);
};

// stream to window, 1 to 1
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_stream<TT_DATA>* inStream0, output_window<TT_DATA>* outWindow0);
};

// stream to window, 1 to 2
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_stream<TT_DATA>* inStream0,
                      output_window<TT_DATA>* outWindow0,
                      output_window<TT_DATA>* outWindow1);
};

// stream to window, 1 to 3
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_stream<TT_DATA>* inStream0,
                      output_window<TT_DATA>* outWindow0,
                      output_window<TT_DATA>* outWindow1,
                      output_window<TT_DATA>* outWindow2);
};

// stream to window, 1 to 4
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_stream<TT_DATA>* inStream0,
                      output_window<TT_DATA>* outWindow0,
                      output_window<TT_DATA>* outWindow1,
                      output_window<TT_DATA>* outWindow2,
                      output_window<TT_DATA>* outWindow3);
};

// Dual stream in
// stream to window, 2 to 1
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_stream<TT_DATA>* inStream0,
                      input_stream<TT_DATA>* inStream1,
                      output_window<TT_DATA>* outWindow0);
};

// stream to window, 1 to 2
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_stream<TT_DATA>* inStream0,
                      input_stream<TT_DATA>* inStream1,
                      output_window<TT_DATA>* outWindow0,
                      output_window<TT_DATA>* outWindow1);
};

// stream to window, 2 to 3
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_stream<TT_DATA>* inStream0,
                      input_stream<TT_DATA>* inStream1,
                      output_window<TT_DATA>* outWindow0,
                      output_window<TT_DATA>* outWindow1,
                      output_window<TT_DATA>* outWindow2);
};

// stream to window, 2 to 4
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_stream<TT_DATA>* inStream0,
                      input_stream<TT_DATA>* inStream1,
                      output_window<TT_DATA>* outWindow0,
                      output_window<TT_DATA>* outWindow1,
                      output_window<TT_DATA>* outWindow2,
                      output_window<TT_DATA>* outWindow3);
};

// Window to stream
// window to stream, 1 to 1
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_window<TT_DATA>* inWindow0, output_stream<TT_DATA>* outStream0);
};

// window to stream, 1 to 2
template <typename TT_DATA, // type of data input and output
          unsigned int TP_WINDOW_VSIZE>
class widget_api_cast_ref<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2> {
   private:
   public:
    // Constructor
    widget_api_cast_ref() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast_ref::transferData); }
    // FIR
    void transferData(input_window<TT_DATA>* inWindow0,
                      output_stream<TT_DATA>* outStream0,
                      output_stream<TT_DATA>* outStream1);
};
}
}
}
}
}

#endif // _DSPLIB_widget_api_cast_REF_HPP_

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
