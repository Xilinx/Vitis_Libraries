/*
Widget real2complex kernal code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#define __AIEARCH__ 1
#define __AIENGINE__ 1
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
//#include "widget_real2complex_traits.hpp"
#include "widget_real2complex.hpp"

#include "widget_real2complex_utils.hpp"

#include "kernel_api_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace real2complex {

// Base specialization, used for
// real to complex (all 3 variants)
template <typename TT_DATA, typename TT_OUT_DATA, unsigned int TP_WINDOW_VSIZE>
inline void kernelClass<TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE>::kernelClassMain(const TT_DATA* inBuff,
                                                                                TT_OUT_DATA* outBuff) {
    using inReal128VectorType = ::aie::vector<TT_DATA, 128 / 8 / sizeof(TT_DATA)>;
    using inReal256VectorType = ::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>;
    using outCplx256VectorType = ::aie::vector<TT_OUT_DATA, 256 / 8 / sizeof(TT_OUT_DATA)>;

    constexpr unsigned int inStep = 16 / sizeof(TT_DATA);
    constexpr unsigned int outStep = 32 / sizeof(TT_OUT_DATA);
    constexpr unsigned int kLsize = TP_WINDOW_VSIZE / inStep;

    const inReal128VectorType inZeroes = ::aie::zeros<TT_DATA, 128 / 8 / sizeof(TT_DATA)>();
    inReal128VectorType inReal;
    ::std::pair<inReal128VectorType, inReal128VectorType> inRealIntlv;
    outCplx256VectorType outCplx;
    inReal256VectorType realLarge;

    inReal128VectorType* __restrict inPtr = (inReal128VectorType*)inBuff;
    outCplx256VectorType* __restrict outPtr = (outCplx256VectorType*)outBuff;

    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            inReal = *inPtr++; // load
            inRealIntlv =
                ::aie::interleave_zip(inReal, inZeroes, 1); // convert to complex by interleaving zeros for imag parts
            realLarge = ::aie::concat<inReal128VectorType, inReal128VectorType>(inRealIntlv.first, inRealIntlv.second);
            outCplx = ::aie::vector_cast<TT_OUT_DATA, inReal256VectorType>(realLarge); // cast
            *outPtr++ = outCplx;
        }
};

template <unsigned int TP_WINDOW_VSIZE>
inline void kernelClass<cint16, int16, TP_WINDOW_VSIZE>::kernelClassMain(const cint16* inBuff, int16* outBuff) {
    typedef cint16 TT_DATA;
    typedef int16 TT_OUT_DATA;
    using inCplx256VectorType = ::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>;
    using outReal256VectorType = ::aie::vector<TT_OUT_DATA, 256 / 8 / sizeof(TT_OUT_DATA)>;
    using outReal128VectorType = ::aie::vector<TT_OUT_DATA, 128 / 8 / sizeof(TT_OUT_DATA)>;

    constexpr unsigned int inStep = 32 / sizeof(TT_DATA);      // numsamples in 256b read
    constexpr unsigned int outStep = 16 / sizeof(TT_OUT_DATA); // numsamples in 128b write
    constexpr unsigned int kLsize = TP_WINDOW_VSIZE / inStep;
    inCplx256VectorType inCplx;
    outReal256VectorType realLarge;
    outReal128VectorType outReal;
    inCplx256VectorType* __restrict inPtr = (inCplx256VectorType*)inBuff;
    outReal128VectorType* __restrict outPtr = (outReal128VectorType*)outBuff;

    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            inCplx = *inPtr++;                                                        // load
            realLarge = ::aie::vector_cast<TT_OUT_DATA, inCplx256VectorType>(inCplx); // convert to real
            outReal = ::aie::filter_even<outReal256VectorType>(realLarge);            // cast
            *outPtr++ = outReal;
        }
};

template <unsigned int TP_WINDOW_VSIZE>
inline void kernelClass<cint32, int32, TP_WINDOW_VSIZE>::kernelClassMain(const cint32* inBuff, int32* outBuff) {
    typedef cint32 TT_DATA;
    typedef int32 TT_OUT_DATA;
    using inCplx256VectorType = ::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>;
    using outReal256VectorType = ::aie::vector<TT_OUT_DATA, 256 / 8 / sizeof(TT_OUT_DATA)>;
    using outReal128VectorType = ::aie::vector<TT_OUT_DATA, 128 / 8 / sizeof(TT_OUT_DATA)>;

    constexpr unsigned int inStep = 32 / sizeof(TT_DATA);      // numsamples in 256b read
    constexpr unsigned int outStep = 16 / sizeof(TT_OUT_DATA); // numsamples in 128b write
    constexpr unsigned int kLsize = TP_WINDOW_VSIZE / inStep;
    inCplx256VectorType inCplx;
    outReal256VectorType realLarge;
    outReal128VectorType outReal;
    inCplx256VectorType* __restrict inPtr = (inCplx256VectorType*)inBuff;
    outReal128VectorType* __restrict outPtr = (outReal128VectorType*)outBuff;

    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            inCplx = *inPtr++;                                                        // load
            realLarge = ::aie::vector_cast<TT_OUT_DATA, inCplx256VectorType>(inCplx); // convert to real
            outReal = ::aie::filter_even<outReal256VectorType>(realLarge);            // cast
            *outPtr++ = outReal;
        }
};

template <unsigned int TP_WINDOW_VSIZE>
inline void kernelClass<cfloat, float, TP_WINDOW_VSIZE>::kernelClassMain(const cfloat* inBuff, float* outBuff) {
    typedef cfloat TT_DATA;
    typedef float TT_OUT_DATA;
    using inCplx256VectorType = ::aie::vector<TT_DATA, 256 / 8 / sizeof(TT_DATA)>;
    using outReal256VectorType = ::aie::vector<TT_OUT_DATA, 256 / 8 / sizeof(TT_OUT_DATA)>;
    using outReal128VectorType = ::aie::vector<TT_OUT_DATA, 128 / 8 / sizeof(TT_OUT_DATA)>;

    constexpr unsigned int inStep = 32 / sizeof(TT_DATA);      // numsamples in 256b read
    constexpr unsigned int outStep = 16 / sizeof(TT_OUT_DATA); // numsamples in 128b write
    constexpr unsigned int kLsize = TP_WINDOW_VSIZE / inStep;
    inCplx256VectorType inCplx;
    outReal256VectorType realLarge;
    outReal128VectorType outReal;
    inCplx256VectorType* __restrict inPtr = (inCplx256VectorType*)inBuff;
    outReal128VectorType* __restrict outPtr = (outReal128VectorType*)outBuff;

    for (int i = 0; i < kLsize; i++) chess_prepare_for_pipelining chess_loop_range(kLsize, ) {
            inCplx = *inPtr++;                                                        // load
            realLarge = ::aie::vector_cast<TT_OUT_DATA, inCplx256VectorType>(inCplx); // convert to real
            outReal = ::aie::filter_even<outReal256VectorType>(realLarge);            // cast
            *outPtr++ = outReal;
        }
};

//-------------------------------------------------------------------------------------------------------
// This is the base specialization of the main class for when there is
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_WINDOW_VSIZE>
__attribute__((noinline))   //This function is the hook for QoR profiling, so must be identifiable after compilation.
void widget_real2complex<TT_DATA, TT_OUT_DATA, TP_WINDOW_VSIZE>::convertData
                (input_window<TT_DATA>* inWindow0,
                 output_window<TT_OUT_DATA>* outWindow0
                )
    {
    TT_DATA* inPtr = (TT_DATA*)inWindow0->ptr;
    TT_OUT_DATA* outPtr = (TT_OUT_DATA*)outWindow0->ptr;
    this->kernelClassMain(inPtr, outPtr);
};
}
}
}
}
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
