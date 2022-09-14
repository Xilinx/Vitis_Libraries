/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef _DSPLIB_FFT_R2COMB_HPP_
#define _DSPLIB_FFT_R2COMB_HPP_

/*
FFT/IFFT, DIT, single channel, R2 combiner stage.
The R2 combiner stage is used for cases where the FFT operation as a whole is split into FFT
subframes whose outputs need to be combined to perform the overall FFT.
This file exists to capture the definition of the single channel FFT/iFFT R2 combiner kernel class.
The class definition holds defensive checks on parameter range and other legality.
The constructor definition is held in this class because this class must be accessible to graph
level aie compilation.
The main runtime function is captured elsewhere as it contains aie intrinsics which are not
included in aie graph level compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes
*/

#include <adf.h>
#include <vector>

#include "fft_ifft_dit_1ch_traits.hpp"

#ifndef _DSPLIB_FFT_R2COMB_HPP_DEBUG_
//#define _DSPLIB_FFT_R2COMB_HPP_DEBUG_
#endif //_DSPLIB_FFT_R2COMB_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {

//-----------------------------------------------------------------------------------------------------

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_INDEX,
          unsigned int TP_ORIG_PAR_POWER>
class fft_r2comb {
   public:
    // Constructor
    fft_r2comb() {} // Constructor

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_r2comb::fft_r2comb_main); }
    // FFT
    void fft_r2comb_main(input_window<TT_DATA>* __restrict inWindow, output_window<TT_DATA>* __restrict outWindow);
};
}
}
}
}
}
#endif // _DSPLIB_FFT_R2COMB_HPP_
