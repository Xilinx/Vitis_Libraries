#ifndef _DSPLIB_WIDGET_API_CAST_HPP_
#define _DSPLIB_WIDGET_API_CAST_HPP_

/*
Widget API Cast Kernel.
This file exists to capture the definition of the widget api cast kernel class.
The class definition holds defensive checks on parameter range and other
legality.
The constructor definition is held in this class because this class must be
accessible to graph level aie compilation.
The main runtime filter function is captured elsewhere as it contains aie
intrinsics which are not included in aie graph level
compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

*/

#include <adf.h>
#include "widget_api_cast_traits.hpp"
#include <vector>

//#define _DSPLIB_WIDGET_API_CAST_HPP_DEBUG_
#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES>
class kernelClass {
   private:
    // Parameter value defensive and legality checks
    static_assert(TP_IN_API <= 2, "ERROR: Unsupported TP_IN_API value set. ");
    static_assert(TP_WINDOW_VSIZE * sizeof(TT_DATA) % 16 == 0, "ERROR: TP_WINDOW_VSIZE must be a multiple of 128bits");

   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, TP_IN_API> inInterface, T_outputIF<TT_DATA, TP_OUT_API> outInterface);
};

template <typename TT_DATA, unsigned int TP_NUM_INPUTS, unsigned int TP_WINDOW_VSIZE, unsigned int TP_NUM_OUTPUT_CLONES>
class kernelClass<TT_DATA, kStreamAPI, kWindowAPI, TP_NUM_INPUTS, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, kStreamAPI> inInterface, T_outputIF<TT_DATA, kWindowAPI> outInterface);
};

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_NUM_OUTPUT_CLONES>
class kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES> {
   private:
   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(T_inputIF<TT_DATA, kWindowAPI> inInterface, T_outputIF<TT_DATA, kStreamAPI> outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. Used for single window to single window copy
template <typename TT_DATA,
          unsigned int TP_IN_API,
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES>
class widget_api_cast
    : public kernelClass<TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES> {
   public:
    // Constructor
    widget_api_cast()
        : kernelClass<TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_WINDOW_VSIZE, TP_NUM_OUTPUT_CLONES>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_window<TT_DATA>* __restrict inWindow, output_window<TT_DATA>* __restrict outWindow0);
};

// Specialization for single window in, dual window out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_window<TT_DATA>* __restrict inWindow0,
                      output_window<TT_DATA>* __restrict outWindow0,
                      output_window<TT_DATA>* __restrict outWindow1);
};

// Specialization for single window in, triple window out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3>
    : public kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kWindowAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_window<TT_DATA>* __restrict inWindow0,
                      output_window<TT_DATA>* __restrict outWindow0,
                      output_window<TT_DATA>* __restrict outWindow1,
                      output_window<TT_DATA>* __restrict outWindow2);
};

// stream to  window, 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 1>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0, output_window<TT_DATA>* __restrict outWindow0);
};

// stream to  window, 2 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 2>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      output_window<TT_DATA>* __restrict outWindow0,
                      output_window<TT_DATA>* __restrict outWindow1);
};

// stream to  window, 3 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 3>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      output_window<TT_DATA>* __restrict outWindow0,
                      output_window<TT_DATA>* __restrict outWindow1,
                      output_window<TT_DATA>* __restrict outWindow2);
};

// stream to  window, 4 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 1, TP_WINDOW_VSIZE, 4>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      output_window<TT_DATA>* __restrict outWindow0,
                      output_window<TT_DATA>* __restrict outWindow1,
                      output_window<TT_DATA>* __restrict outWindow2,
                      output_window<TT_DATA>* __restrict outWindow3);
};

// dual stream input
// stream to  window, 2 in 1 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 1>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_window<TT_DATA>* __restrict outWindow0);
};

// stream to  window, 2 in 2 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 2>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_window<TT_DATA>* __restrict outWindow0,
                      output_window<TT_DATA>* __restrict outWindow1);
};

// stream to  window, 2 in 3 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 3>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_window<TT_DATA>* __restrict outWindow0,
                      output_window<TT_DATA>* __restrict outWindow1,
                      output_window<TT_DATA>* __restrict outWindow2);
};

// stream to  window, 2 in 4 out
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4>
    : public kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kStreamAPI, kWindowAPI, 2, TP_WINDOW_VSIZE, 4>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_stream<TT_DATA>* __restrict inStream0,
                      input_stream<TT_DATA>* __restrict inStream1,
                      output_window<TT_DATA>* __restrict outWindow0,
                      output_window<TT_DATA>* __restrict outWindow1,
                      output_window<TT_DATA>* __restrict outWindow2,
                      output_window<TT_DATA>* __restrict outWindow3);
};

// Window to stream 1 to 1
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1>
    : public kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 1>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_window<TT_DATA>* __restrict inWindow0, output_stream<TT_DATA>* __restrict outStream0);
};

// Window to stream 1 to 2
template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_api_cast<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2>
    : public kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2> {
   public:
    // Constructor
    widget_api_cast() : kernelClass<TT_DATA, kWindowAPI, kStreamAPI, 1, TP_WINDOW_VSIZE, 2>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_api_cast::transferData); }

    // Main function
    void transferData(input_window<TT_DATA>* __restrict inWindow0,
                      output_stream<TT_DATA>* __restrict outStream0,
                      output_stream<TT_DATA>* __restrict outStream1);
};
}
}
}
}
}

#endif // _DSPLIB_WIDGET_API_CAST_HPP_

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
