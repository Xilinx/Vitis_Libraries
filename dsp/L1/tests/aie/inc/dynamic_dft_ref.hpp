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
#ifndef _DSPLIB_DYNAMIC_DFT_REF_HPP_
#define _DSPLIB_DYNAMIC_DFT_REF_HPP_

#ifndef _DSPLIB_DFT_REF_DEBUG_
//#define _DSPLIB_DFT_REF_DEBUG_
#endif //_DSPLIB_DFT_REF_DEBUG_
#include "device_defs.h"
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
// DFT single channel reference model class
template <typename TT_DATA,    // type of data input and output
          typename TT_TWIDDLE, // type of twiddle factor
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_SSR>
class dynamic_dft_ref {
   private:
   public:
    // Constructor
    dynamic_dft_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dynamic_dft_ref::dynDftMatrixVectorMult); }
    // FFT
    void dynDftMatrixVectorMult(input_buffer<TT_DATA>& inWindow,
                                input_buffer<TT_DATA>& headerInWindow,
                                output_buffer<TT_DATA>& outWindow,
                                output_buffer<TT_DATA>& headerOutWindow);
};
}
}
}
}
} // namespace closing braces

#endif // _DSPLIB_DYNAMIC_DFT_REF_HPP_
