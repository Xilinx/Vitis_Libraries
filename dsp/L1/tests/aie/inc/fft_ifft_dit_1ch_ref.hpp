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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_REF_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_REF_HPP_

/*
FFT/iFFT DIT single channel reference model
*/

//#define _DSPLIB_FFT_IFFT_DIT_1CH_REF_DEBUG_

#include <adf.h>
#include <limits>

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {

#include "fft_ref_utils.hpp"

constexpr int kFftDynHeadBytes = __ALIGN_BYTE_SIZE__;

//-----------------------------------------------------------------------------------------------------
// FFT/iFFT DIT single channel reference model class
template <typename TT_DATA,    // type of data input and output
          typename TT_TWIDDLE, // type of twiddle factor
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_ORIG_PAR_POWER = 0,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA>
class fft_ifft_dit_1ch_ref {
   private:
    TT_TWIDDLE twiddles[TP_POINT_SIZE];
    static constexpr unsigned int kMaxLogPtSize = 12;
    static constexpr unsigned int kRanks =
        fnGetPointSizePower<TP_POINT_SIZE>(); // while each rank is radix2 this is true
    void r2StageInt(
        T_int_data<TT_DATA>* samplesA, T_int_data<TT_DATA>* samplesB, TT_TWIDDLE* twiddles, int pptSize, bool inv);
    void r2StageFloat(T_int_data<TT_DATA>* samplesA,
                      T_int_data<TT_DATA>* samplesB,
                      TT_TWIDDLE* twiddles,
                      unsigned int rank,
                      int pptSize,
                      bool inv);
    void r4StageIntSpoof(T_int_data<TT_DATA>* samplesIn,
                         TT_TWIDDLE* twiddles1,
                         TT_TWIDDLE* twiddles2,
                         unsigned int n,
                         unsigned int r,
                         unsigned int shift,
                         unsigned int rank,
                         T_int_data<TT_DATA>* samplesOut,
                         int pptSize,
                         bool inv);
    void r4StageIntTrue(T_int_data<TT_DATA>* samplesIn,
                        TT_TWIDDLE* twiddles1,
                        TT_TWIDDLE* twiddles2,
                        unsigned int n,
                        unsigned int r,
                        unsigned int shift,
                        unsigned int rank,
                        T_int_data<TT_DATA>* samplesOut,
                        int pptSize,
                        bool inv);

   public:
    // Constructor
    fft_ifft_dit_1ch_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_ifft_dit_1ch_ref::fft); }
    // FFT
    void fft(input_buffer<TT_DATA>& inWindow, output_buffer<TT_OUT_DATA>& outWindow);
    void nonBitAccfft(input_buffer<TT_DATA>& inWindow, output_buffer<TT_OUT_DATA>& outWindow);
};
}
}
}
}
} // namespace closing braces

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_REF_HPP_
