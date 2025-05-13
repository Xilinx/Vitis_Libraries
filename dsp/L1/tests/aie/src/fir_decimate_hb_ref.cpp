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
Halfband decimation FIR Reference model
This file holds the body of the reference model for the above kernel class.
The reference model is agnostic of intrinsics, so is simpler and easier to validate.
It is then used as the verification golden reference for the kernel class.
*/

#include "aie_api/aie_adf.hpp"
#include "fir_decimate_hb_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_hb {

// Constructor for static coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
fir_decimate_hb_ref<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    TP_USE_COEFF_RELOAD,
                    TP_NUM_OUTPUTS,
                    TP_API,
                    TP_SAT>::fir_decimate_hb_ref(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    int inputIndex = 0;
    for (int i = 0; i < TP_FIR_LEN; ++i) {
        if (i == m_kCentreTapInternalPos) {
            m_internalTaps[i] = taps[m_kCentreTapInputPos];
        } else if (i < TP_FIR_LEN / 2) {
            if ((i % 2) == 0) {
                m_internalTaps[i] = taps[inputIndex++];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
            }
        } else {
            m_internalTaps[i] = m_internalTaps[TP_FIR_LEN - 1 - i]; // symmetric coefficients
        }
    }
};

// Constructor for reloadable coefficients
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
fir_decimate_hb_ref<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    USE_COEFF_RELOAD_TRUE,
                    TP_NUM_OUTPUTS,
                    TP_API,
                    TP_SAT>::fir_decimate_hb_ref(){};

// REF FIR function for static coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
void fir_decimate_hb_ref<TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         TP_USE_COEFF_RELOAD,
                         TP_NUM_OUTPUTS,
                         TP_API,
                         TP_SAT>::
    filter(input_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
           output_buffer<TT_DATA>& outWindow) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in;
    unsigned int dataReads = 0;
    TT_DATA accumSrs;

    auto inItr = ::aie::begin_random_circular(inWindow);
    auto outItr = ::aie::begin_random_circular(outWindow);

    // alternate architecture - to attempt bit-accuracy with UUT by executing float operations in the same order.
    TT_DATA d[TP_FIR_LEN];

    // The margin in the window allows the state of the FIR to be re-established at the start of each new window
    // to match the state of the FIR at the end of the previous window. This margin is the length of the FIR, but
    // padded to be a multiple of 32bytes. This additional padding is stepped over by advancing the window pointer.
    const unsigned int kFirLen = TP_FIR_LEN;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    inItr += kFirMarginOffset;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += 2) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;

        // The following form, of the reference model matches the UUT order of calculations which is necessary
        // for bit-accuracy when using float types.
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d[j] = *inItr++;
            dataReads++;
        }
        for (unsigned int j = 0; j < (TP_FIR_LEN + 1) / 4; j++) {
            // Perform MUL operations mimicking vector processors order.
            d_in = d[j * 2];
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in, m_internalTaps[j * 2]);
            d_in = d[(TP_FIR_LEN - 1) - j * 2];
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in, m_internalTaps[j * 2]);
            if (j == m_kDataSampleCentre) {
                multiplyAcc<TT_DATA, TT_COEFF>(accum, d[j * 2 + 1], m_internalTaps[j * 2 + 1]);
            }
        }

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum, TP_SAT);
        accumSrs = castAcc(accum);
        *outItr++ = accumSrs;

        // Revert data pointer for next sample
        inItr -= dataReads - 2;
    }
};

// REF FIR function for reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
void fir_decimate_hb_ref<TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         USE_COEFF_RELOAD_TRUE,
                         TP_NUM_OUTPUTS,
                         TP_API,
                         TP_SAT>::
    filter(input_buffer<TT_DATA, extents<inherited_extent>, margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
           output_buffer<TT_DATA>& outWindow,
           const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    // Reload coefficients
    int inputIndex = 0;
    for (int i = 0; i < TP_FIR_LEN; ++i) {
        if (i == m_kCentreTapInternalPos) {
            m_internalTaps[i] = inTaps[m_kCentreTapInputPos];
        } else if (i < TP_FIR_LEN / 2) {
            if ((i % 2) == 0) {
                m_internalTaps[i] = inTaps[inputIndex++];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
            }
        } else {
            m_internalTaps[i] = m_internalTaps[TP_FIR_LEN - 1 - i]; // symmetric coefficients
        }
    }

    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in;
    unsigned int dataReads = 0;
    TT_DATA accumSrs;
    // alternate architecture - to attempt bit-accuracy with UUT by executing float operations in the same order.
    TT_DATA d[TP_FIR_LEN];

    auto inItr = ::aie::begin_random_circular(inWindow);
    auto outItr = ::aie::begin_random_circular(outWindow);

    // The margin in the window allows the state of the FIR to be re-established at the start of each new window
    // to match the state of the FIR at the end of the previous window. This margin is the length of the FIR, but
    // padded to be a multiple of 32bytes. This additional padding is stepped over by advancing the window pointer.
    const unsigned int kFirLen = TP_FIR_LEN;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    inItr += kFirMarginOffset; // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += 2) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;

        // The following form, of the reference model matches the UUT order of calculations which is necessary
        // for bit-accuracy when using float types.
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d[j] = *inItr++;
            dataReads++;
        }
        for (unsigned int j = 0; j < (TP_FIR_LEN + 1) / 4; j++) {
            // Perform MUL operations mimicking vector processors order.
            d_in = d[j * 2];
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in, m_internalTaps[j * 2]);
            d_in = d[(TP_FIR_LEN - 1) - j * 2];
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in, m_internalTaps[j * 2]);
            if (j == m_kDataSampleCentre) {
                multiplyAcc<TT_DATA, TT_COEFF>(accum, d[j * 2 + 1], m_internalTaps[j * 2 + 1]);
            }
        }

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum, TP_SAT);
        accumSrs = castAcc(accum);
        *outItr++ = accumSrs;

        // Revert data pointer for next sample
        inItr -= dataReads - 2;
    }
};
}
}
}
}
}
