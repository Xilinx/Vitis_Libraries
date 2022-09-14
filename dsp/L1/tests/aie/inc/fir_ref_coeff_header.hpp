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
#ifndef _DSPLIB_FIR_REF_COEFF_HEADER_HPP_
#define _DSPLIB_FIR_REF_COEFF_HEADER_HPP_

#include <adf.h>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD>
void firHeaderReload(input_window<TT_DATA>* inWindow, TT_COEFF (&internalTaps)[TP_FIR_LEN]) {
    if (TP_USE_COEFF_RELOAD == 2) {
        input_window<TT_DATA> temp_w;
        input_window<TT_DATA>* __restrict cpWindow;
        cpWindow = &temp_w;
        window_copy(cpWindow, inWindow);
        const unsigned int kFirMargin = fnFirMargin<TP_FIR_LEN, TT_DATA>(); // FIR Margin, aligned to 32 Bytes.

        window_incr(inWindow, kFirMargin); // skip margin
        int coeffSize;
        if
            constexpr(isFloat<TT_DATA>()) {
                TT_DATA header_tmp = window_read(inWindow);
                if
                    constexpr(isComplex<TT_DATA>()) { coeffSize = static_cast<int>(header_tmp.real); }
                else {
                    coeffSize = static_cast<int>(header_tmp);
                }
            }
        else {
            coeffSize = window_read((input_window<int32>*)inWindow); // read header as int32
        }
        int headerSize = 256 / 8 / sizeof(TT_COEFF); // read 256-bits
        int firCeil = (headerSize - (TP_FIR_LEN % headerSize)) % headerSize;
        int coeffExpectedSize = TP_FIR_LEN + firCeil;

        if (coeffSize == 0 || coeffSize == coeffExpectedSize) {
            // Coefficient Array Size embedded in header may only take limited values.
            // All good here
        } else {
            printf("Error: Stream Header Coefficient Array Size malformed. Got: %d, expecting either 0 or %d \n",
                   coeffSize, coeffExpectedSize);
        }
        // skip Header Config
        window_incr((input_window<TT_COEFF>*)inWindow, headerSize);
        if (coeffSize == 0) {
            window_incr((input_window<TT_COEFF>*)inWindow, TP_FIR_LEN); // incr FIR Length samples
            window_incr((input_window<TT_COEFF>*)inWindow, firCeil);    // incr remainder of 256-bits
        } else {
            for (int i = 0; i < TP_FIR_LEN; i++) {
                internalTaps[i] = window_readincr((input_window<TT_COEFF>*)inWindow);
            }
            window_incr((input_window<TT_COEFF>*)inWindow, firCeil); // incr remainder of 256-bits
        }
        TT_DATA firstSample = window_read(inWindow); // Window pointer should point to the very first sample

        TT_DATA sample[kFirMargin];

        // cut out the Header from the input window,i.e.
        // shift margin from original place by
        window_decr(inWindow,
                    kFirMargin); // revert back by margin samples, so the pointer is at position: Data[0] - kFirMargin
        for (int i = 0; i < kFirMargin; i++) {
            sample[i] = window_readincr(cpWindow);
        }
        for (int i = 0; i < kFirMargin; i++) {
            window_writeincr((output_window<TT_DATA>*)inWindow, sample[i]);
        }
        window_decr(inWindow,
                    kFirMargin); // revert back by margin samples, so the pointer is at position: Data[0] - kFirMargin
        window_decr(cpWindow, kFirMargin); // revert back by margin samples
    }
};
}
}
}
}

#endif // ifdef _DSPLIB_FIR_REF_COEFF_HEADER_HPP_
