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
/*
Single Rate Symmetrical FIR Reference model
This file holds the body of the reference model for the above kernel class.
The reference model is agnostic of intrinsics, so is simpler and easier to validate.
It is then used as the verification golden reference for the kernel class.
*/
#include "aie_api/aie_adf.hpp"
#include "fir_sr_sym_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_sym {

// REF FIR function - no coefficient reload
//-----------------------------------------------------------------------------------------------------
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
void fir_sr_sym_ref<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    TP_USE_COEFF_RELOAD,
                    TP_NUM_OUTPUTS,
                    TP_API,
                    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                          extents<inherited_extent>,
                                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                    output_circular_buffer<TT_DATA>& outWindow) {
    auto inWindowPtr = ::aie::begin_random_circular(inWindow);
    auto outWindowPtr = ::aie::begin_random_circular(outWindow);

    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;

    printf("Ref model params:\n");
    printf("TP_FIR_LEN = %lu\n", TP_FIR_LEN);
    printf("TP_SHIFT = %lu\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_WINDOW_SIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    printf("TP_SAT = %d\n", TP_SAT);
    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    inWindowPtr += kFirMarginOffset;

    for (int i = 0; i < TP_FIR_LEN; i++) {
        printf("Ref Coeffs[%d]: %d \n", i, m_internalTaps[i]); // only real coeffs!
    }

    // Each iteration of this loop calculates a single output sample from this FIR.
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d_in[j] = *inWindowPtr++;
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], m_internalTaps[TP_FIR_LEN - 1 - j]);
        }
        // Revert data pointer for next sample
        inWindowPtr -= TP_FIR_LEN - 1;

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum, TP_SAT);
        accumSrs = castAcc(accum);
        *outWindowPtr++ = accumSrs;
        // printf("Wrote to and incremented output window\n");
    }
};

// REF FIR function - using coefficient reload, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_API,
          unsigned int TP_SAT>
void fir_sr_sym_ref<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    USE_COEFF_RELOAD_TRUE,
                    TP_NUM_OUTPUTS,
                    TP_API,
                    TP_SAT>::filter(input_circular_buffer<TT_DATA,
                                                          extents<inherited_extent>,
                                                          margin<fnFirMargin<TP_FIR_LEN, TT_DATA>()> >& inWindow,
                                    output_circular_buffer<TT_DATA>& outWindow,
                                    const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 2]) {
    auto inWindowPtr = ::aie::begin_random_circular(inWindow);
    auto outWindowPtr = ::aie::begin_random_circular(outWindow);

    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;
    firReload(inTaps); // loads inTaps into m_internalTaps

    printf("Ref model params:\n");
    printf("TP_FIR_LEN = %lu\n", TP_FIR_LEN);
    printf("TP_SHIFT = %lu\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_WINDOW_SIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    printf("TP_SAT = %d\n", TP_SAT);
    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    inWindowPtr += kFirMarginOffset;

    for (int i = 0; i < TP_FIR_LEN; i++) {
        printf("Ref Coeffs[%d]: %d \n", i, m_internalTaps[i]); // only real coeffs!
    }

    // Each iteration of this loop calculates a single output sample from this FIR.
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d_in[j] = *inWindowPtr++;
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], m_internalTaps[TP_FIR_LEN - 1 - j]);
        }
        // Revert data pointer for next sample
        inWindowPtr -= TP_FIR_LEN - 1;

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum, TP_SAT);
        accumSrs = castAcc(accum);
        *outWindowPtr++ = accumSrs;
    }
};
}
}
}
}
}
