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
#ifndef _DSPLIB_fir_tdm_REF_HPP_
#define _DSPLIB_fir_tdm_REF_HPP_

/*
Single rate asymetric FIR filter reference model
*/

#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace tdm {

//-----------------------------------------------------------------------------------------------------
// Single Rate class
// Static coefficients
template <typename TT_DATA,
          typename TT_OUT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_TDM_CHANNELS = 0,
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_SAT = 1>
class fir_tdm_ref {
   private:
    TT_COEFF internalTaps[TP_FIR_LEN * TP_TDM_CHANNELS] = {};

   public:
    // Constructor
    fir_tdm_ref(const TT_COEFF (&taps)[TP_FIR_LEN * TP_TDM_CHANNELS]) {
        for (int i = 0; i < TP_FIR_LEN * TP_TDM_CHANNELS; i++) {
            internalTaps[i] = taps[i];
        }
    }
    // Constructor
    fir_tdm_ref() {
        // Do nothing here.
    }

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_tdm_ref::filter); }
    // FIR
    void filter(input_circular_buffer<TT_DATA,
                                      extents<inherited_extent>,
                                      margin<fnTDMFirMargin<TP_FIR_LEN, TT_DATA, TP_TDM_CHANNELS>()> >& inWindow,
                output_circular_buffer<TT_OUT_DATA>& outWindow);
};
}
}
}
}
}

#endif // _DSPLIB_fir_tdm_REF_HPP_
