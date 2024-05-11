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
#ifndef _DSPLIB_FIR_REF_COEFF_HEADER_HPP_
#define _DSPLIB_FIR_REF_COEFF_HEADER_HPP_

#include <adf.h>
#include "aie_api/aie_adf.hpp"

using namespace adf;

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
void firHeaderReload(
    auto& inWindowPtr,
    auto& cpWindowPtr,
    // auto is a shortcut. The following comments show these arguments types better.
    //::aie::detail::random_circular_iterator<TT_DATA, inheritedSize, fnFirMargin<TP_FIR_LEN,TT_DATA>()>& inWindowPtr,
    //::aie::detail::random_circular_iterator<TT_DATA, inheritedSize, fnFirMargin<TP_FIR_LEN,TT_DATA>()>& cpWindowPtr,
    TT_COEFF (&internalTaps)[TP_FIR_LEN]) {
    using t_datav = ::aie::vector<TT_DATA, 1>;
    using t_coeffv = ::aie::vector<TT_DATA, 1>;
    using t_int32v = ::aie::vector<int32, 1>;

    if (TP_USE_COEFF_RELOAD == 2) {
        const unsigned int kFirMargin = fnFirMargin<TP_FIR_LEN, TT_DATA>(); // FIR Margin, aligned to 32 Bytes.

        inWindowPtr += kFirMargin;
        int32 coeffSize;
        int32* coeffSizePtr;
        if
            constexpr(isFloat<TT_DATA>()) {
                TT_DATA header_tmp = *inWindowPtr;
                if
                    constexpr(isComplex<TT_DATA>()) { coeffSize = static_cast<int>(header_tmp.real); }
                else {
                    coeffSize = static_cast<int>(header_tmp);
                }
            }
        else {
            coeffSizePtr = (int32*)&*inWindowPtr;
            coeffSize = *coeffSizePtr;
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
        TT_COEFF* inWindowCoeffPtr = (TT_COEFF*)&*inWindowPtr;
        TT_COEFF* inWindowCoeffPtrbase = inWindowCoeffPtr;
        inWindowCoeffPtr += headerSize;
        if (coeffSize == 0) {
            inWindowCoeffPtr += TP_FIR_LEN;
            inWindowCoeffPtr += firCeil;
        } else {
            for (int i = 0; i < TP_FIR_LEN; i++) {
                internalTaps[i] = *inWindowCoeffPtr++;
            }
            inWindowCoeffPtr += firCeil;
        }
        inWindowPtr += (inWindowCoeffPtr - inWindowCoeffPtrbase) * sizeof(TT_COEFF) / sizeof(TT_DATA);
        TT_DATA firstSample = *inWindowPtr;

        TT_DATA sample[kFirMargin];

        // cut out the Header from the input window,i.e.
        // shift margin from original place by
        inWindowPtr -= kFirMargin;
        for (int i = 0; i < kFirMargin; i++) {
            sample[i] = *cpWindowPtr++;
        }
        for (int i = 0; i < kFirMargin; i++) {
            *inWindowPtr++ = sample[i];
        }
        *inWindowPtr -= kFirMargin;
        *cpWindowPtr -= kFirMargin;
    }
};
}
}
}
}

#endif // ifdef _DSPLIB_FIR_REF_COEFF_HEADER_HPP_
