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
#include "aie_api/aie_adf.hpp"
#include "fir_resampler_ref.hpp"
#include "fir_ref_utils.hpp"
#include "fir_ref_coeff_header.hpp"

/*
Fractional interpolator asymetric FIR filter reference model
*/

// #define _DSPLIB_FIR_RESAMPLER_REF_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace resampler {

// base definition, static coeffs, single output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_SAT>
void filter_ref(
    input_circular_buffer<
        TT_DATA,
        extents<inherited_extent>,
        margin<fnFirMargin<(TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR, TT_DATA>()> >& inWindow,
    output_circular_buffer<TT_DATA>& outWindow,
    const TT_COEFF (&taps)[TP_FIR_LEN]) {
    const unsigned int shift = TP_SHIFT;
    const unsigned int kPolyLen = (TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[kPolyLen + TP_DECIMATE_FACTOR - 1];
    TT_DATA accumSrs;

    auto inItr = ::aie::begin_random_circular(inWindow);
    auto outItr = ::aie::begin_random_circular(outWindow);
    int q = 0;

    unsigned int dataIndex, coefIndex;
    const unsigned int kFirMarginOffset = fnFirMargin<kPolyLen, TT_DATA>() - kPolyLen + 1; // FIR Margin Offset.
    inItr += kFirMarginOffset; // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / TP_DECIMATE_FACTOR; i++) {
        // We produce interpolate factor number of ouputs over decimate factor of inputs
        // so we need extra samples
        for (unsigned int j = 0; j < kPolyLen + TP_DECIMATE_FACTOR; j++) {
            d_in[j] = *inItr++; // read input data
        }
        // TP_INTERPOLATE_FACTOR outputs over TP_DECIMATE_FACTOR input samples
        for (unsigned int k = 0; k < TP_INTERPOLATE_FACTOR; k++) {
            accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
            // p for polyphase taps
            for (unsigned int p = 0; p < kPolyLen; p++) {
                // + kPolyLen-1 so we don't get negative indices for margin samples.
                // Data goes in reverse direction like theory
                dataIndex = ((k * TP_DECIMATE_FACTOR) / TP_INTERPOLATE_FACTOR) - p + kPolyLen - 1;
                // Coefficients goes in Forward direction by interpolation rate for each polyphase.
                coefIndex = p * TP_INTERPOLATE_FACTOR + ((k * TP_DECIMATE_FACTOR) % TP_INTERPOLATE_FACTOR);
                // guard for fir_len padding
                if (coefIndex < TP_FIR_LEN) {
                    multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[dataIndex], taps[coefIndex]);
                }
            }

            roundAcc(TP_RND, shift, accum);
            saturateAcc(accum, TP_SAT);
            accumSrs = castAcc(accum);

            *outItr++ = accumSrs;
        }
        // Revert data pointer for next set of samples
        inItr -= (kPolyLen + TP_DECIMATE_FACTOR) - TP_DECIMATE_FACTOR;
    }
};

// base definition, static coeffs, single output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
void fir_resampler_ref<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_INTERPOLATE_FACTOR,
                       TP_DECIMATE_FACTOR,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       TP_USE_COEFF_RELOAD,
                       TP_NUM_OUTPUTS,
                       TP_SAT>::
    filter(input_circular_buffer<
               TT_DATA,
               extents<inherited_extent>,
               margin<fnFirMargin<(TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR, TT_DATA>()> >&
               inWindow,
           output_circular_buffer<TT_DATA>& outWindow) {
    //    firHeaderReload<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INPUT_WINDOW_VSIZE, TP_USE_COEFF_RELOAD>(inWindow,
    //    m_internalTaps); //coeff reload from header is no longer supported
    filter_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
               TP_INPUT_WINDOW_VSIZE, TP_SAT>(inWindow, outWindow, m_internalTaps);
};

// specialization for reload coeffs, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_SAT>
void fir_resampler_ref<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_INTERPOLATE_FACTOR,
                       TP_DECIMATE_FACTOR,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       TP_USE_COEFF_RELOAD,
                       TP_NUM_OUTPUTS,
                       TP_SAT>::
    filterRtp(input_circular_buffer<
                  TT_DATA,
                  extents<inherited_extent>,
                  margin<fnFirMargin<(TP_FIR_LEN + TP_INTERPOLATE_FACTOR - 1) / TP_INTERPOLATE_FACTOR, TT_DATA>()> >&
                  inWindow,
              output_circular_buffer<TT_DATA>& outWindow,
              const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    // Coefficient reload
    for (int i = 0; i < TP_FIR_LEN; i++) {
        m_internalTaps[i] = inTaps[i];
    }
    filter_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_INTERPOLATE_FACTOR, TP_DECIMATE_FACTOR, TP_SHIFT, TP_RND,
               TP_INPUT_WINDOW_VSIZE, TP_SAT>(inWindow, outWindow, m_internalTaps);
};
}
}
}
}
}
