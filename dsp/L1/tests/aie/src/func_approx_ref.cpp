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
/*
Function Approximation reference model
*/

#include "func_approx_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {
// Function Approximation Ref Base specialization
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
void func_approx_ref<TT_DATA,
                     TP_COARSE_BITS,
                     TP_FINE_BITS,
                     TP_DOMAIN_MODE,
                     TP_WINDOW_VSIZE,
                     TP_SHIFT,
                     TP_RND,
                     TP_SAT,
                     TP_USE_LUT_RELOAD>::func_approx_main_ref(input_buffer<TT_DATA>& inWindow,
                                                              output_buffer<TT_DATA>& outWindow) {
    TT_DATA* inPtr = inWindow.data();
    TT_DATA* outPtr = outWindow.data();
    TT_LUT* lutPtr = (TP_USE_LUT_RELOAD == 1) ? m_lutRtpPtr : m_lut_ab;
    TT_DATA d_in;
    TT_DATA d_out;
    T_accRef<TT_DATA> accum;
    TT_DATA elemIn;
    int lutAddr;
    TT_DATA fineData;
    TT_DATA slopeVal, offsetVal;
    for (int j = 0; j < TP_WINDOW_VSIZE; j++) {
        d_in = *inPtr++;
        if
            constexpr(!isFloatingPoint) {
                lutAddr = ((coarseMask & d_in) >> TP_FINE_BITS);
                fineData = d_in & fineMask;
                slopeVal = lutPtr[2 * lutAddr];
                offsetVal = lutPtr[(2 * lutAddr) + 1];
                accum = (T_accRef<TT_DATA>)((offsetVal << (TP_FINE_BITS + TP_SHIFT)) + fineData * slopeVal);
                roundAcc(TP_RND, TP_SHIFT + TP_FINE_BITS, accum);
                saturateAcc(accum, TP_SAT);
                d_out = castAcc(accum);
                // printf("d_in = %d lutAddr = %d FineData = %d slopeVal = %d offsetVal = %d out = %d\n", d_in, lutAddr,
                //    fineData, slopeVal, offsetVal, d_out);
            }
        else {
            int32 intDin = (int32)(d_in * (1 << (TP_COARSE_BITS + TP_FINE_BITS - TP_DOMAIN_MODE)));
            lutAddr = (intDin >> TP_FINE_BITS);
            slopeVal = lutPtr[2 * lutAddr];
            offsetVal = lutPtr[(2 * lutAddr) + 1];
            d_out = d_in * slopeVal + offsetVal;
            // printf("din = %f intDin = %d lutAddres = %d slopeVal = %f offsetVal = %f d_out = %f\n", (float)d_in,
            // intDin,
            //    lutAddr, (float)slopeVal, (float)offsetVal, (float)d_out);
        }
        *outPtr++ = d_out;
    }
};
// Function Approximation Ref Base specialization
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
void func_approx_ref<TT_DATA,
                     TP_COARSE_BITS,
                     TP_FINE_BITS,
                     TP_DOMAIN_MODE,
                     TP_WINDOW_VSIZE,
                     TP_SHIFT,
                     TP_RND,
                     TP_SAT,
                     TP_USE_LUT_RELOAD>::approx_main(input_buffer<TT_DATA>& inWindow,
                                                     output_buffer<TT_DATA>& outWindow) {
    this->func_approx_main_ref(inWindow, outWindow);
}

// Function Approximation Ref Base specialization
template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_USE_LUT_RELOAD>
void func_approx_ref<TT_DATA,
                     TP_COARSE_BITS,
                     TP_FINE_BITS,
                     TP_DOMAIN_MODE,
                     TP_WINDOW_VSIZE,
                     TP_SHIFT,
                     TP_RND,
                     TP_SAT,
                     TP_USE_LUT_RELOAD>::approx_rtp(input_buffer<TT_DATA>& inWindow,
                                                    output_buffer<TT_DATA>& outWindow,
                                                    const TT_LUT (&lut0)[kLutValues],
                                                    const TT_LUT (&lut1)[kLutValues]) {
    this->m_lutRtpPtr = (TT_LUT*)lut0;
    this->m_lutRtpPtr1 = (TT_LUT*)lut1;
    this->func_approx_main_ref(inWindow, outWindow);
}
}
}
}
}
