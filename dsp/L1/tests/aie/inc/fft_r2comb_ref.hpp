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
#ifndef _DSPLIB_FFT_R2_COMB_REF_HPP_
#define _DSPLIB_FFT_R2_COMB_REF_HPP_

/*
FFT/iFFT DIT single channel reference model
*/

//#define _DSPLIB_FFT_R2_COMB_REF_DEBUG_

#include <adf.h>
#include <limits>

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {
#include "fft_ref_utils.hpp"
#include "fft_r2comb_twiddle_lut_all.hpp"

//-----------------------------------------------------------------------------------------------------
// FFT/iFFT R2 combiner stage reference model class
template <typename TT_DATA,    // type of data input and output
          typename TT_TWIDDLE, // type of twiddle factor
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_PARALLEL_POWER = 1,
          unsigned int TP_ORIG_PAR_POWER = TP_PARALLEL_POWER,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0>
class fft_r2comb_ref {
   private:
    static constexpr unsigned int kSupportedPtSizes = 12; // 16 to 64k.
    int twiddle_tables[kSupportedPtSizes];
    TT_TWIDDLE twiddles[(TP_POINT_SIZE / (2 - TP_DYN_PT_SIZE)) >> TP_PARALLEL_POWER]; // inelegant way of saying table
                                                                                      // for dynamic point size is 2x
                                                                                      // the static one because it is
                                                                                      // 1+1/2+1/4+...
   public:
    unsigned int kIndex;
    // Constructor
    // a twiddle page is on 1/4, 1/8th or some other 2^-Nth of the full twiddle table for the point size in question.
    // There will be 4, 8 or some other N clones of this kernel all executing on different pages.
    fft_r2comb_ref(unsigned int inIdx) {
        kIndex = inIdx;
        const TT_TWIDDLE* twiddle_master = fnGetR2TwiddleMasterBase<TT_TWIDDLE, TP_TWIDDLE_MODE>();

        int idx;
        int stride;
        int table_index = 0;
        int table_base_index = 0;

        int ptSize = TP_POINT_SIZE;
        if
            constexpr(TP_DYN_PT_SIZE == 1) {
                for (int table_size = (TP_POINT_SIZE / 2) >> TP_PARALLEL_POWER; table_size >= 8;
                     table_size = (table_size >> 1)) {
                    idx = 2 * kR2MasterTableSize / ptSize * kIndex;
                    stride = ((2 * kR2MasterTableSize / ptSize) << TP_PARALLEL_POWER);
                    twiddle_tables[table_base_index++] = table_index;
                    for (int i = 0; i<(ptSize / 2)>> TP_PARALLEL_POWER; i++) {
                        twiddles[table_index] = twiddle_master[idx];
                        idx += stride;
                        table_index++;
                    }
                    ptSize = (ptSize >> 1);
                }
            }
        else {
            idx = 2 * kR2MasterTableSize / ptSize * kIndex;
            stride = ((2 * kR2MasterTableSize / ptSize) << TP_PARALLEL_POWER);
            twiddle_tables[table_base_index++] = table_index;
            for (int i = 0; i<(ptSize / 2)>> TP_PARALLEL_POWER; i++) {
                twiddles[table_index] = twiddle_master[idx];
                idx += stride;
                table_index++;
            }
        }
    }
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fft_r2comb_ref::fft_r2comb_ref_main); }
    // FFT
    void r2StageInt(
        TT_DATA* samplesA, TT_DATA* samplesB, TT_TWIDDLE* twiddles, int pptSize /*used for dyn pt size*/, bool inv);
    void r2StageFloat(
        TT_DATA* samplesA, TT_DATA* samplesB, TT_TWIDDLE* twiddles, int pptSize /*used for dyn pt size*/, bool inv);
    void fft_r2comb_ref_main(input_buffer<TT_DATA>& inWindow, output_buffer<TT_DATA>& outWindow);
};
}
}
}
}
} // namespace closing braces

#endif // _DSPLIB_FFT_R2_COMB_REF_HPP_
