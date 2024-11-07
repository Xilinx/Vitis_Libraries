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
#include "aie_api/aie_adf.hpp"
#include "fir_decimate_asym_ref.hpp"
#include "fir_ref_utils.hpp"
#include "fir_ref_coeff_header.hpp"
// #define _DSPLIB_FIR_DECIMATE_ASYM_REF_DEBUG_

/*
Decimator Asymmetric FIR filter reference model
*/

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_asym {

//-----------------------------------------------------------------------------------------------------
// REF FIR function - static coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void filter_ref(
    input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
    output_circular_buffer<TT_DATA>& outWindow,
    const TT_COEFF (&taps)[TP_FIR_LEN]) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;

    auto inItr = ::aie::begin_random_circular(inWindow);
    auto outItr = ::aie::begin_random_circular(outWindow);
    int q = 0;

    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    inItr += kFirMarginOffset;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / TP_DECIMATE_FACTOR; i++) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
                                        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d_in[j] = *inItr++; // read input data

            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], taps[TP_FIR_LEN - 1 - j]);
        }
        // Revert data pointer for next sample
        inItr -= TP_FIR_LEN - TP_DECIMATE_FACTOR;

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum, TP_SAT);
        accumSrs = castAcc(accum);
        *outItr++ = accumSrs;
    }
};

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_SAT>
void fir_decimate_asym_ref<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_DECIMATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           TP_USE_COEFF_RELOAD,
                           TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                                 extents<inherited_extent>,
                                                                 margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                           output_circular_buffer<TT_DATA>& outWindow) {
    //    firHeaderReload<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_USE_COEFF_RELOAD>(inWindow,
    //    m_internalTaps); //header of coeffs feature is no longer supported
    filter_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_SAT>(
        inWindow, outWindow, m_internalTaps);
};

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD, // 1 = use coeff reload, 0 = don't use coeff reload
          unsigned int TP_SAT>
void fir_decimate_asym_ref<TT_DATA,
                           TT_COEFF,
                           TP_FIR_LEN,
                           TP_DECIMATE_FACTOR,
                           TP_SHIFT,
                           TP_RND,
                           TP_INPUT_WINDOW_VSIZE,
                           TP_USE_COEFF_RELOAD,
                           TP_SAT>::
    filterRtp(input_circular_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >&
                  inWindow,
              output_circular_buffer<TT_DATA>& outWindow,
              const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    // Coefficient reload
    for (int i = 0; i < TP_FIR_LEN; i++) {
        m_internalTaps[i] = inTaps[i];
    }
    filter_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_SAT>(
        inWindow, outWindow, m_internalTaps);
};
}
}
}
}
}
