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
#ifndef _DSPLIB_FIR_DECIMATE_ASYM_REF_HPP_
#define _DSPLIB_FIR_DECIMATE_ASYM_REF_HPP_

/*
Asymmetric Decimation FIR filter reference model
*/

#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_asym {

//-----------------------------------------------------------------------------------------------------
// Decimate Asym Reference Model Class - static coefficients, single output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_SAT>
class fir_decimate_asym_ref {
   private:
    alignas(32) TT_COEFF m_internalTaps[TP_FIR_LEN];

   public:
    // Constructor
    fir_decimate_asym_ref(const TT_COEFF (&taps)[TP_FIR_LEN]) {
        for (int i = 0; i < TP_FIR_LEN; i++) {
            m_internalTaps[i] = taps[i];
        }
    }
    // Constructor
    fir_decimate_asym_ref() {}

    // Register Kernel Class
    static void registerKernelClass() {
        if
            constexpr(TP_USE_COEFF_RELOAD == 1) { REGISTER_FUNCTION(fir_decimate_asym_ref::filterRtp); }
        else {
            REGISTER_FUNCTION(fir_decimate_asym_ref::filter);
        }
    }
    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_circular_buffer<TT_DATA>& outWindow);
    void filterRtp(
        input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
            inWindow,
        output_circular_buffer<TT_DATA>& outWindow,
        const TT_COEFF (&inTaps)[TP_FIR_LEN]);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_DECIMATE_ASYM_REF_HPP_
