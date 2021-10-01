/*
 * Copyright 2021 Xilinx, Inc.
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
#ifndef _DSPLIB_FFT_R2_COMB_REF_HPP_
#define _DSPLIB_FFT_R2_COMB_REF_HPP_

/*
FFT/iFFT DIT single channel reference model
*/

#ifndef _DSPLIB_FFT_R2COMB_REF_DEBUG_
//#define _DSPLIB_FFT_R2_COMB_REF_DEBUG_
#endif //_DSPLIB_FFT_R2COMB_REF_DEBUG_

#include <adf.h>
#include <limits>

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {
#include "fft_ref_utils.hpp"
#include "fft_r2comb_twiddle_lut.hpp"

//-----------------------------------------------------------------------------------------------------
// FFT/iFFT R2 combiner stage reference model class
template <typename TT_DATA,    // type of data input and output
          typename TT_TWIDDLE, // type of twiddle factor
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_PARALLEL_POWER = 1>
class fft_r2comb_ref {
   private:
    TT_TWIDDLE twiddles[(TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER];

   public:
    unsigned int kIndex;
    // Constructor
    // a twiddle page is on 1/4, 1/8th or some other 2^-Nth of the full twiddle table for the point size in question.
    // There will be 4, 8 or some other N clones of this kernel all executing on different pages.
    fft_r2comb_ref(unsigned int inIdx) {
        kIndex = inIdx;
        const TT_TWIDDLE* twiddle_master = fnGetR2TwiddleMasterBase<TT_TWIDDLE>();
        int idx = ((kR2MasterTableSize >> TP_PARALLEL_POWER) * kIndex);
        int stride;
        stride = (2 * kR2MasterTableSize / TP_POINT_SIZE);
        for (int i = 0; i<(TP_POINT_SIZE / 2)>> TP_PARALLEL_POWER; i++) {
            twiddles[i] = twiddle_master[idx];
            idx += stride;
        }
    }
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_r2comb_ref::fft_r2comb_ref_main); }
    // FFT
    void r2StageInt(
        TT_DATA* samplesA, TT_DATA* samplesB, TT_TWIDDLE* twiddles, int pptSize /*used for dyn pt size*/, bool inv);
    void r2StageFloat(
        TT_DATA* samplesA, TT_DATA* samplesB, TT_TWIDDLE* twiddles, int pptSize /*used for dyn pt size*/, bool inv);
    void fft_r2comb_ref_main(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow);
};
}
}
}
}
} // namespace closing braces

#endif // _DSPLIB_FFT_R2_COMB_REF_HPP_
