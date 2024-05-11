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
#ifndef _DSPLIB_FIR_DECIMATE_SYM_REF_HPP_
#define _DSPLIB_FIR_DECIMATE_SYM_REF_HPP_

/*
Symmetric Decimation FIR filter reference model
*/

#include <adf.h>
#include <limits>
#include "fir_ref_utils.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_sym {

//-----------------------------------------------------------------------------------------------------
// Reference model kernel class, static coeffs, single output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD = 0, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_NUM_OUTPUTS = 1,
          unsigned int TP_API = 0,
          unsigned int TP_SAT = 1>
class fir_decimate_sym_ref {
   public:
    // Constructor
    fir_decimate_sym_ref(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 2]) {
        for (int i = 0; i < (TP_FIR_LEN + 1) / 2; i++) {
            m_internalTaps[i] = taps[i];
            m_internalTaps[TP_FIR_LEN - 1 - i] = taps[i];
        }
    }
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym_ref::filter); }
    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_circular_buffer<TT_DATA>& outWindow);

   private:
    TT_COEFF chess_storage(% chess_alignof(v8cint16)) m_internalTaps[TP_FIR_LEN];
};

//-----------------------------------------------------------------------------------------------------
// Specialization of Reference model kernel class for reload coefficients, single output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned TP_SAT>
class fir_decimate_sym_ref<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_DECIMATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           USE_COEFF_RELOAD_TRUE,
                           TP_NUM_OUTPUTS,
                           TP_API,
                           TP_SAT> {
   public:
    // Constructor
    fir_decimate_sym_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(fir_decimate_sym_ref::filter); }
    // FIR
    void filter(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                    inWindow,
                output_circular_buffer<TT_DATA>& outWindow,
                const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2]);

   private:
    TT_COEFF chess_storage(% chess_alignof(v8cint16)) m_internalTaps[TP_FIR_LEN];
};
}
}
}
}
}

#endif // _DSPLIB_FIR_DECIMATE_SYM_REF_HPP_
