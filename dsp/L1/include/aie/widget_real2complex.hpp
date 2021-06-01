#ifndef _DSPLIB_WIDGET_REAL2COMPLEX_HPP_
#define _DSPLIB_WIDGET_REAL2COMPLEX_HPP_

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
#include "widget_real2complex_traits.hpp"
#include <vector>

//#define _DSPLIB_WIDGET_REAL2COMPLEX_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace real2complex {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, typename TT_OUT_DATA, unsigned int TP_WINDOW_VSIZE>
class kernelClass {
   private:
    // Parameter value defensive and legality checks

   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(const TT_DATA* inBuff, TT_OUT_DATA* outBuff);
};

template <unsigned int TP_WINDOW_VSIZE>
class kernelClass<cint16, int16, TP_WINDOW_VSIZE> {
   private:
    // Parameter value defensive and legality checks

   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(const cint16* inBuff, int16* outBuff);
};

template <unsigned int TP_WINDOW_VSIZE>
class kernelClass<cint32, int32, TP_WINDOW_VSIZE> {
   private:
    // Parameter value defensive and legality checks

   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(const cint32* inBuff, int32* outBuff);
};

template <unsigned int TP_WINDOW_VSIZE>
class kernelClass<cfloat, float, TP_WINDOW_VSIZE> {
   private:
    // Parameter value defensive and legality checks

   public:
    // Constructor
    kernelClass() {}

    void kernelClassMain(const cfloat* inBuff, float* outBuff);
};

//-----------------------------------------------------------------------------------------------------
// Single kernel base specialization. Used for single window to single window copy
template <typename TT_DATA, typename TT_OUT_DATA, unsigned int TP_WINDOW_VSIZE>
class widget_real2complex : public kernelClass<TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE> {
   public:
    // Constructor
    widget_real2complex() : kernelClass<TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(widget_real2complex::convertData); }

    // Main function
    void convertData(input_window<TT_DATA>* inWindow, output_window<TT_OUT_DATA>* outWindow0);
};
}
}
}
}
}

#endif // _DSPLIB_WIDGET_REAL2COMPLEX_HPP_

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
