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
Single rate asymetric FIR filter reference model
*/

#include "aie_api/aie_adf.hpp"
#include "fir_sr_asym_ref.hpp"
#include "fir_ref_utils.hpp"
#include "fir_ref_coeff_header.hpp"

// #define _DSPLIB_FIR_SR_ASYM_REF_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_asym {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void filter_ref(
    auto& inWindowPtr,
    auto& outWindowPtr,
    // auto is a shortcut. The following comments describe the arguments a little better.
    //::aie::detail::random_circular_iterator<TT_DATA, inheritedSize, fnFirMargin<TP_FIR_LEN,TT_DATA>()>& inWindowPtr,
    //::aie::detail::random_circular_iterator<TT_DATA, inheritedSize, fnFirMargin<TP_FIR_LEN,TT_DATA>()>& outWindowPtr,
    const TT_COEFF (&taps)[TP_FIR_LEN]) {
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;

    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.

    inWindowPtr += kFirMarginOffset;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d_in[j] = *inWindowPtr++;
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], taps[TP_FIR_LEN - 1 - j]);
        }
        // Revert data pointer for next sample
        inWindowPtr -= TP_FIR_LEN - 1;

        roundAcc(TP_RND, TP_SHIFT, accum);
        saturateAcc(accum, TP_SAT);
        accumSrs = castAcc(accum);
        *outWindowPtr++ = accumSrs;
    }
};

// REF FIR function - default/base 'specialization'
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          unsigned int TP_SAT

          >
void fir_sr_asym_ref<TT_DATA,
                     TT_COEFF,
                     TP_FIR_LEN,
                     TP_SHIFT,
                     TP_RND,
                     TP_INPUT_WINDOW_VSIZE,
                     TP_USE_COEFF_RELOAD,
                     TP_NUM_OUTPUTS,
                     TP_DUAL_IP,
                     TP_API,
                     TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                           extents<inherited_extent>,
                                                           margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                     output_circular_buffer<TT_DATA>& outWindow) {
    auto inWindowPtr = ::aie::begin_random_circular(inWindow);
    auto cpWindowPtr = ::aie::begin_random_circular(inWindow);
    auto outWindowPtr = ::aie::begin_random_circular(outWindow);
    firHeaderReload<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_USE_COEFF_RELOAD>(inWindowPtr, cpWindowPtr,
                                                                                               internalTaps);
    filter_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_SAT>(
        inWindowPtr, outWindowPtr, internalTaps);
};

// REF FIR function
// RTP coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_DUAL_IP,
          unsigned int TP_API,
          unsigned int TP_SAT

          >
void fir_sr_asym_ref<TT_DATA,
                     TT_COEFF,
                     TP_FIR_LEN,
                     TP_SHIFT,
                     TP_RND,
                     TP_INPUT_WINDOW_VSIZE,
                     TP_USE_COEFF_RELOAD,
                     TP_NUM_OUTPUTS,
                     TP_DUAL_IP,
                     TP_API,
                     TP_SAT>::filterRtp(input_circular_buffer<TT_DATA,
                                                              extents<inherited_extent>,
                                                              margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                        output_circular_buffer<TT_DATA>& outWindow,
                                        const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    // Coefficient Reload
    for (int i = 0; i < TP_FIR_LEN; i++) {
        internalTaps[i] = inTaps[i];
    }
    auto inWindowPtr = ::aie::begin_random_circular(inWindow);
    auto outWindowPtr = ::aie::begin_random_circular(outWindow);
    filter_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, TP_SAT>(
        inWindowPtr, outWindowPtr, internalTaps);
};
}
}
}
}
}
