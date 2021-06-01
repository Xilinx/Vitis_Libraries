#include "fir_interpolate_asym_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_asym {
// body of the Asymmetric Interpolation FIR reference model kernel class, static coeffs, single output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS>
void fir_interpolate_asym_ref<TT_DATA,
                              TT_COEFF,
                              TP_FIR_LEN,
                              TP_INTERPOLATE_FACTOR,
                              TP_SHIFT,
                              TP_RND,
                              TP_INPUT_WINDOW_VSIZE,
                              TP_USE_COEFF_RELOAD,
                              TP_NUM_OUTPUTS>::filter(input_window<TT_DATA>* inWindow,
                                                      output_window<TT_DATA>* outWindow) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN / TP_INTERPOLATE_FACTOR];
    TT_DATA accum_srs;

    printf("Ref model params:\n");
    printf("TP_INTERPOLATE_FACTOR = %d\n", TP_INTERPOLATE_FACTOR);
    printf("TP_SHIFT = %lu\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_INPUT_WINDOW_SIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    const unsigned int kFirLen = TP_FIR_LEN / TP_INTERPOLATE_FACTOR;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        for (unsigned int j = 0; j < TP_FIR_LEN / TP_INTERPOLATE_FACTOR; ++j) {
            d_in[j] = window_readincr(inWindow); // read input data
        }
        for (int k = TP_INTERPOLATE_FACTOR - 1; k >= 0; --k) {
            accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
            for (unsigned int j = 0; j < TP_FIR_LEN / TP_INTERPOLATE_FACTOR; ++j) {
                multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], m_internalTapsRef[j * TP_INTERPOLATE_FACTOR + k]);
            }
            // prior to output, the final accumulated value must be downsized to the same type
            // as was input. To do this, the final result is rounded, saturated and shifted down
            roundAcc(TP_RND, shift, accum);
            saturateAcc(accum);
            accum_srs = castAcc(accum);
            window_writeincr((output_window<TT_DATA>*)outWindow, accum_srs);
        }
        // Revert data pointer for next sample
        window_decr(inWindow, TP_FIR_LEN / TP_INTERPOLATE_FACTOR - 1);
    }
};

// specialization, static coeffs, dual output
template <typename TT_DATA,  // type of data input and output
          typename TT_COEFF, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
void fir_interpolate_asym_ref<TT_DATA,
                              TT_COEFF,
                              TP_FIR_LEN,
                              TP_INTERPOLATE_FACTOR,
                              TP_SHIFT,
                              TP_RND,
                              TP_INPUT_WINDOW_VSIZE,
                              USE_COEFF_RELOAD_FALSE,
                              2>::filter(input_window<TT_DATA>* inWindow,
                                         output_window<TT_DATA>* outWindow,
                                         output_window<TT_DATA>* outWindow2) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN / TP_INTERPOLATE_FACTOR];
    TT_DATA accum_srs;

    printf("Ref model params:\n");
    printf("TP_INTERPOLATE_FACTOR = %d\n", TP_INTERPOLATE_FACTOR);
    printf("TP_SHIFT = %lu\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_INPUT_WINDOW_SIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    const unsigned int kFirLen = TP_FIR_LEN / TP_INTERPOLATE_FACTOR;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        for (unsigned int j = 0; j < TP_FIR_LEN / TP_INTERPOLATE_FACTOR; ++j) {
            d_in[j] = window_readincr(inWindow); // read input data
        }
        for (int k = TP_INTERPOLATE_FACTOR - 1; k >= 0; --k) {
            accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
            for (unsigned int j = 0; j < TP_FIR_LEN / TP_INTERPOLATE_FACTOR; ++j) {
                multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], m_internalTapsRef[j * TP_INTERPOLATE_FACTOR + k]);
            }
            // prior to output, the final accumulated value must be downsized to the same type
            // as was input. To do this, the final result is rounded, saturated and shifted down
            roundAcc(TP_RND, shift, accum);
            saturateAcc(accum);
            accum_srs = castAcc(accum);
            window_writeincr((output_window<TT_DATA>*)outWindow, accum_srs);
            window_writeincr((output_window<TT_DATA>*)outWindow2, accum_srs);
        }
        // Revert data pointer for next sample
        window_decr(inWindow, TP_FIR_LEN / TP_INTERPOLATE_FACTOR - 1);
    }
};

// specialization, reload coeffs, single output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
void fir_interpolate_asym_ref<TT_DATA,
                              TT_COEFF,
                              TP_FIR_LEN,
                              TP_INTERPOLATE_FACTOR,
                              TP_SHIFT,
                              TP_RND,
                              TP_INPUT_WINDOW_VSIZE,
                              USE_COEFF_RELOAD_TRUE,
                              1>::filter(input_window<TT_DATA>* inWindow,
                                         output_window<TT_DATA>* outWindow,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    // Coefficient reload
    for (int i = 0; i < TP_FIR_LEN; i++) {
        m_internalTapsRef[i] = inTaps[FIR_LEN - 1 - i];
        printf("inTaps[%d] = %d\n", i, inTaps[i]);
    }
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN / TP_INTERPOLATE_FACTOR];
    TT_DATA accum_srs;

    printf("Ref model params:\n");
    printf("TP_INTERPOLATE_FACTOR = %d\n", TP_INTERPOLATE_FACTOR);
    printf("TP_SHIFT = %lu\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_INPUT_WINDOW_SIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    const unsigned int kFirLen = TP_FIR_LEN / TP_INTERPOLATE_FACTOR;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        for (unsigned int j = 0; j < TP_FIR_LEN / TP_INTERPOLATE_FACTOR; ++j) {
            d_in[j] = window_readincr(inWindow); // read input data
        }
        for (int k = TP_INTERPOLATE_FACTOR - 1; k >= 0; --k) {
            accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
            for (unsigned int j = 0; j < TP_FIR_LEN / TP_INTERPOLATE_FACTOR; ++j) {
                multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], m_internalTapsRef[j * TP_INTERPOLATE_FACTOR + k]);
            }
            // prior to output, the final accumulated value must be downsized to the same type
            // as was input. To do this, the final result is rounded, saturated and shifted down
            roundAcc(TP_RND, shift, accum);
            saturateAcc(accum);
            accum_srs = castAcc(accum);
            window_writeincr((output_window<TT_DATA>*)outWindow, accum_srs);
        }
        // Revert data pointer for next sample
        window_decr(inWindow, TP_FIR_LEN / TP_INTERPOLATE_FACTOR - 1);
    }
};

// specialization, reload coeffs, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_INTERPOLATE_FACTOR,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
void fir_interpolate_asym_ref<TT_DATA,
                              TT_COEFF,
                              TP_FIR_LEN,
                              TP_INTERPOLATE_FACTOR,
                              TP_SHIFT,
                              TP_RND,
                              TP_INPUT_WINDOW_VSIZE,
                              USE_COEFF_RELOAD_TRUE,
                              2>::filter(input_window<TT_DATA>* inWindow,
                                         output_window<TT_DATA>* outWindow,
                                         output_window<TT_DATA>* outWindow2,
                                         const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    // Coefficient reload
    for (int i = 0; i < TP_FIR_LEN; i++) {
        m_internalTapsRef[i] = inTaps[FIR_LEN - 1 - i];
        printf("inTaps[%d] = %d\n", i, inTaps[i]);
    }
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN / TP_INTERPOLATE_FACTOR];
    TT_DATA accum_srs;

    printf("Ref model params:\n");
    printf("TP_INTERPOLATE_FACTOR = %d\n", TP_INTERPOLATE_FACTOR);
    printf("TP_SHIFT = %lu\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_INPUT_WINDOW_SIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    const unsigned int kFirLen = TP_FIR_LEN / TP_INTERPOLATE_FACTOR;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        for (unsigned int j = 0; j < TP_FIR_LEN / TP_INTERPOLATE_FACTOR; ++j) {
            d_in[j] = window_readincr(inWindow); // read input data
        }
        for (int k = TP_INTERPOLATE_FACTOR - 1; k >= 0; --k) {
            accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
            for (unsigned int j = 0; j < TP_FIR_LEN / TP_INTERPOLATE_FACTOR; ++j) {
                multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], m_internalTapsRef[j * TP_INTERPOLATE_FACTOR + k]);
            }
            // prior to output, the final accumulated value must be downsized to the same type
            // as was input. To do this, the final result is rounded, saturated and shifted down
            roundAcc(TP_RND, shift, accum);
            saturateAcc(accum);
            accum_srs = castAcc(accum);
            window_writeincr((output_window<TT_DATA>*)outWindow, accum_srs);
            window_writeincr((output_window<TT_DATA>*)outWindow2, accum_srs);
        }
        // Revert data pointer for next sample
        window_decr(inWindow, TP_FIR_LEN / TP_INTERPOLATE_FACTOR - 1);
    }
};
}
}
}
}
}
/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
