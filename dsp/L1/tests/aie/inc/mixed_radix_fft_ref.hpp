/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_MIXED_RADIX_FFT_REF_HPP_
#define _DSPLIB_MIXED_RADIX_FFT_REF_HPP_

/*
MIXED_RADIX_FFT single channel reference model








This is a placeholder file. Currently the reference model of the Mixed radix FFT is simply the DFT.















*/

#ifndef _DSPLIB_MIXED_RADIX_FFT_REF_DEBUG_
//#define _DSPLIB_MIXED_RADIX_FFT_REF_DEBUG_
#endif //_DSPLIB_MIXED_RADIX_FFT_REF_DEBUG_

#include <adf.h>
#include <limits>

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {
#include "fft_ref_utils.hpp"

//-----------------------------------------------------------------------------------------------------
// MIXED_RADIX_FFT single channel reference model class
template <typename TT_DATA,    // type of data input and output
          typename TT_TWIDDLE, // type of twiddle factor
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT
          // unsigned int    TP_NUM_FRAMES
          >
class mixed_radix_fft_ref {
   private:
   public:
    // Constructor
    mixed_radix_fft_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(mixed_radix_fft_ref::mixed_radix_fftMatrixVectorMult); }
    // FFT
    void mixed_radix_fftMatrixVectorMult(input_buffer<TT_DATA>& inWindow, output_buffer<TT_DATA>& outWindow);
};
}
}
}
}
} // namespace closing braces

#endif // _DSPLIB_MIXED_RADIX_FFT_REF_HPP_
