/*
Halfband decimation FIR Reference model
This file holds the body of the reference model for the above kernel class.
The reference model is agnostic of intrinsics, so is simpler and easier to validate.
It is then used as the verification golden reference for the kernel class.
*/

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
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS>
fir_decimate_hb_ref<TT_DATA,
                    TT_COEFF,
                    TP_FIR_LEN,
                    TP_SHIFT,
                    TP_RND,
                    TP_INPUT_WINDOW_VSIZE,
                    TP_USE_COEFF_RELOAD,
                    TP_NUM_OUTPUTS>::fir_decimate_hb_ref(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1]) {
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

// Constructor for static coefficients, dual output.
// This differs not at all from the default constructor, but must be explicit because this class specialization is
// required for the filter function, so has to have its own constructor. It may be possible to call the default
// constructor directly rather than this raw copy.
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
fir_decimate_hb_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, USE_COEFF_RELOAD_FALSE, 2>::
    fir_decimate_hb_ref(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1]) {
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
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
fir_decimate_hb_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, USE_COEFF_RELOAD_TRUE, 1>::
    fir_decimate_hb_ref(){};

template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
fir_decimate_hb_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, USE_COEFF_RELOAD_TRUE, 2>::
    fir_decimate_hb_ref(){};

// REF FIR function for static coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS>
void fir_decimate_hb_ref<TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         TP_USE_COEFF_RELOAD,
                         TP_NUM_OUTPUTS>::filter(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in;
    unsigned int dataReads = 0;
    TT_DATA accumSrs;

    // alternate architecture - to attempt bit-accuracy with UUT by executing float operations in the same order.
    TT_DATA d[TP_FIR_LEN];

    // The margin in the window allows the state of the FIR to be re-established at the start of each new window
    // to match the state of the FIR at the end of the previous window. This margin is the length of the FIR, but
    // padded to be a multiple of 32bytes. This additional padding is stepped over by advancing the window pointer.
    const unsigned int kFirLen = TP_FIR_LEN;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += 2) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;

        // The following form, of the reference model matches the UUT order of calculations which is necessary
        // for bit-accuracy when using float types.
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d[j] = window_readincr(inWindow);
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
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);

        // Revert data pointer for next sample
        window_decr(inWindow, dataReads - 2);
    }
};

// REF FIR function for static coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
void fir_decimate_hb_ref<TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         USE_COEFF_RELOAD_FALSE,
                         2>::filter(input_window<TT_DATA>* inWindow,
                                    output_window<TT_DATA>* outWindow,
                                    output_window<TT_DATA>* outWindow2) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in;
    unsigned int dataReads = 0;
    TT_DATA accumSrs;

    // alternate architecture - to attempt bit-accuracy with UUT by executing float operations in the same order.
    TT_DATA d[TP_FIR_LEN];

    // The margin in the window allows the state of the FIR to be re-established at the start of each new window
    // to match the state of the FIR at the end of the previous window. This margin is the length of the FIR, but
    // padded to be a multiple of 32bytes. This additional padding is stepped over by advancing the window pointer.
    const unsigned int kFirLen = TP_FIR_LEN;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += 2) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;

        // The following form, of the reference model matches the UUT order of calculations which is necessary
        // for bit-accuracy when using float types.
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d[j] = window_readincr(inWindow);
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
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
        window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);

        // Revert data pointer for next sample
        window_decr(inWindow, dataReads - 2);
    }
};

// REF FIR function for reload coefficients, single output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
void fir_decimate_hb_ref<TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         USE_COEFF_RELOAD_TRUE,
                         1>::filter(input_window<TT_DATA>* inWindow,
                                    output_window<TT_DATA>* outWindow,
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

    // The margin in the window allows the state of the FIR to be re-established at the start of each new window
    // to match the state of the FIR at the end of the previous window. This margin is the length of the FIR, but
    // padded to be a multiple of 32bytes. This additional padding is stepped over by advancing the window pointer.
    const unsigned int kFirLen = TP_FIR_LEN;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += 2) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;

        // The following form, of the reference model matches the UUT order of calculations which is necessary
        // for bit-accuracy when using float types.
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d[j] = window_readincr(inWindow);
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
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);

        // Revert data pointer for next sample
        window_decr(inWindow, dataReads - 2);
    }
};

// REF FIR function for reload coefficients, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE>
void fir_decimate_hb_ref<TT_DATA,
                         TT_COEFF,
                         TP_FIR_LEN,
                         TP_SHIFT,
                         TP_RND,
                         TP_INPUT_WINDOW_VSIZE,
                         USE_COEFF_RELOAD_TRUE,
                         2>::filter(input_window<TT_DATA>* inWindow,
                                    output_window<TT_DATA>* outWindow,
                                    output_window<TT_DATA>* outWindow2,
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

    // The margin in the window allows the state of the FIR to be re-established at the start of each new window
    // to match the state of the FIR at the end of the previous window. This margin is the length of the FIR, but
    // padded to be a multiple of 32bytes. This additional padding is stepped over by advancing the window pointer.
    const unsigned int kFirLen = TP_FIR_LEN;
    const unsigned int kFirMarginOffset = fnFirMargin<kFirLen, TT_DATA>() - kFirLen + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i += 2) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;

        // The following form, of the reference model matches the UUT order of calculations which is necessary
        // for bit-accuracy when using float types.
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d[j] = window_readincr(inWindow);
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
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
        window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);

        // Revert data pointer for next sample
        window_decr(inWindow, dataReads - 2);
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
