/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _DSPLIB_FFT_WINDOW_HPP_
#define _DSPLIB_FFT_WINDOW_HPP_

/*
FFT Window Kernel.
This file exists to capture the definition of the FFT window kernel class.
The class definition holds defensive checks on parameter range and other
legality.
The constructor definition is held in this class because this class must be
accessible to graph level aie compilation.
The main runtime function is captured elsewhere (cpp) as it contains aie
intrinsics (albeit aie api) which are not included in aie graph level
compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

*/

//#include "fft_window_traits.hpp"
#include <vector>
#include <array>
#include <adf.h>
#include "device_defs.h"

using namespace adf;

//#define _DSPLIB_FFT_WINDOW_HPP_DEBUG_

#include "fft_window_traits.hpp" //for fnPointSizePwr

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace windowfn {
//-----------------------------------------------------------------------------------------------------
// FFT window kernel class
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class fft_window {
   private:
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int kLogSamplesInVect = std::is_same<TT_DATA, cint16>::value ? 3 : 2;
    static constexpr unsigned int kVecInFrame = TP_POINT_SIZE / kSamplesInVect;
    static constexpr unsigned int kLogSSR = fnLogSSR<TP_SSR>();
    static constexpr unsigned int kCoeffsTableSize = (TP_DYN_PT_SIZE == 0) ? TP_POINT_SIZE : TP_POINT_SIZE * 2;
    static constexpr unsigned int kPtSizePwr = fnPointSizePower<TP_POINT_SIZE>();
    // size of weights array is obvious for static case. For dynamic case, we need the static table, plus tables which
    // are 1/2 that size,
    // 1/4 that size, 1/8.... which converges a size twice that of the static case.
    alignas(__ALIGN_BYTE_SIZE__)
        TT_COEFF weights[kCoeffsTableSize]; // A shorthand way of saying N for static, 2N for dynamic
    // TT_COEFF* weights;
    int tableStarts[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

   public:
    // Constructor
    fft_window(const TT_COEFF (&kernel_weights)[TP_POINT_SIZE * (1 + TP_DYN_PT_SIZE)]);

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_DYN_PT_SIZE == 0) { REGISTER_FUNCTION(fft_window::fft_window_main); }
        else {
            REGISTER_FUNCTION(fft_window::fft_window_main_dyn);
        }
    }

    // Main function
    void fft_window_main(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
    void fft_window_main_dyn(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};

// fft window kernel class - stream specialization.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class fft_window<TT_DATA,
                 TT_COEFF,
                 TP_POINT_SIZE,
                 TP_WINDOW_VSIZE,
                 TP_SHIFT,
                 1,
                 TP_SSR,
                 TP_DYN_PT_SIZE,
                 TP_RND,
                 TP_SAT> {
   private:
    static constexpr unsigned int kSamplesInVect = 256 / 8 / sizeof(TT_DATA);
    static constexpr unsigned int kLogSamplesInVect = std::is_same<TT_DATA, cint16>::value ? 3 : 2;
    static constexpr unsigned int kVecInFrame = TP_POINT_SIZE / kSamplesInVect;
    static constexpr unsigned int kLogSSR = fnLogSSR<TP_SSR>();
    static constexpr unsigned int kCoeffsTableSize = (TP_DYN_PT_SIZE == 0) ? TP_POINT_SIZE : TP_POINT_SIZE * 2;
    static constexpr unsigned int kPtSizePwr = fnPointSizePower<TP_POINT_SIZE>();
    // size of weights array is obvious for static case. For dynamic case, we need the static table, plus tables which
    // are 1/2 that size,
    // 1/4 that size, 1/8.... which converges a size twice that of the static case.
    alignas(__ALIGN_BYTE_SIZE__) TT_COEFF weights[kCoeffsTableSize];
    int tableStarts[kPtSizePwr];

   public:
    // Constructor
    fft_window(const TT_COEFF (&kernel_weights)[TP_POINT_SIZE * (1 + TP_DYN_PT_SIZE)]);

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_DYN_PT_SIZE == 0) { REGISTER_FUNCTION(fft_window::fft_window_main); }
        else {
            REGISTER_FUNCTION(fft_window::fft_window_main_dyn);
        }
    }

// Main function
// These could be claused out according to __STREAMS_PER_TILE__, but the overload is unambiguous.
#if __STREAMS_PER_TILE__ == 2
    void fft_window_main(input_stream<TT_DATA>* __restrict inStream0,
                         input_stream<TT_DATA>* __restrict inStream1,
                         output_stream<TT_DATA>* __restrict outStream0,
                         output_stream<TT_DATA>* __restrict outStream1);
    void fft_window_main_dyn(input_stream<TT_DATA>* __restrict inStream0,
                             input_stream<TT_DATA>* __restrict inStream1,
                             output_stream<TT_DATA>* __restrict outStream0,
                             output_stream<TT_DATA>* __restrict outStream1);
#else
    void fft_window_main(input_stream<TT_DATA>* __restrict inStream0, output_stream<TT_DATA>* __restrict outStream0);
    void fft_window_main_dyn(input_stream<TT_DATA>* __restrict inStream0,
                             output_stream<TT_DATA>* __restrict outStream0);
#endif // __STREAMS_PER_TILE__ == 2
};
}
}
}
}
}

#endif // _DSPLIB_FFT_WINDOW_HPP_
