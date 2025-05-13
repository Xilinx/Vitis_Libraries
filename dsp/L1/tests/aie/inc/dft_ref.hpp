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
#ifndef _DSPLIB_DFT_REF_HPP_
#define _DSPLIB_DFT_REF_HPP_

/*
DFT single channel reference model
*/

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
namespace dft {
#include "fft_ref_utils.hpp"

//-----------------------------------------------------------------------------------------------------
// DFT single channel reference model class
template <typename TT_DATA,    // type of data input and output
          typename TT_TWIDDLE, // type of twiddle factor
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_SSR>
class dft_ref {
   private:
    TT_TWIDDLE coeff[TP_POINT_SIZE][TP_POINT_SIZE];
    cfloat tmp[TP_POINT_SIZE][TP_POINT_SIZE];
    typedef typename std::conditional_t<
        std::is_same<TT_DATA, int16>::value,
        cint16,
        std::conditional_t<std::is_same<TT_DATA, int32>::value,
                           cint32,
                           std::conditional_t<std::is_same<TT_DATA, float>::value, cfloat, TT_DATA> > >
        T_outDataType;

   public:
    // Constructor
    dft_ref() {
        // Form twiddle table for this point size;
        double inv = (TP_FFT_NIFFT == 1) ? -1.0 : 1.0;
        for (unsigned int n = 0; n < TP_POINT_SIZE; n++) {
            for (unsigned int k = 0; k < TP_POINT_SIZE; k++) {
                tmp[n][k].real = (cos(M_PI * 2.0 * (double)n * (double)k / (double)TP_POINT_SIZE));
                tmp[n][k].imag = (inv * sin(M_PI * 2.0 * (double)n * (double)k / (double)TP_POINT_SIZE));
                // to aviod Arithmetic overflow for 32768
                if (std::is_same<TT_TWIDDLE, cfloat>::value) {
                    coeff[n][k].real = tmp[n][k].real;
                    coeff[n][k].imag = tmp[n][k].imag;
                } else {
                    tmp[n][k].real = (tmp[n][k].real * 32768.0);
                    tmp[n][k].imag = (tmp[n][k].imag * 32768.0);
                    if (tmp[n][k].real >= 32767.0) {
                        tmp[n][k].real = 32767.0;
                    }
                    if (tmp[n][k].imag >= 32767.0) {
                        tmp[n][k].imag = 32767.0;
                    }
                    coeff[n][k].real = (int16)tmp[n][k].real;
                    coeff[n][k].imag = (int16)tmp[n][k].imag;
                }
            }
        }
    }
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dft_ref::dftMatrixVectorMult); }
    // FFT
    void dftMatrixVectorMult(input_buffer<TT_DATA>& inWindow, output_buffer<T_outDataType>& outWindow);
};
}
}
}
}
} // namespace closing braces

#endif // _DSPLIB_DFT_REF_HPP_
