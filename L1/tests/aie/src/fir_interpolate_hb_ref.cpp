
/*
Halfband Interpolator FIR Reference model
This file holds the body of the reference model for the above kernel class.
The reference model is agnostic of intrinsics, so is simpler and easier to validate.
It is then used as the verification golden reference for the kernel class.
*/

#include "fir_interpolate_hb_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_hb {

template <typename T_D>
inline int16 getUpshiftCt(T_D inVal) {
    // Do nothing for types other than 16-bit integers
    return 0;
}

template <>
inline int16 getUpshiftCt(int16 inVal) {
    const unsigned int kMaxUpshiftVal = 16;
    int16 retVal;
    // Make sure value is within UCT supported range (0 - 16).
    retVal = inVal % kMaxUpshiftVal;
    return retVal;
}
template <>
inline int16 getUpshiftCt(cint16 inVal) {
    const unsigned int kMaxUpshiftVal = 16;
    int16 retVal;
    // Make sure value is within UCT supported range (0 - 16).
    retVal = inVal.real % kMaxUpshiftVal;
    return retVal;
}
//------------------------------------------------------------------------
// Constructor to populate m_internalTaps array
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT>
fir_interpolate_hb_ref<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       TP_USE_COEFF_RELOAD,
                       TP_NUM_OUTPUTS,
                       TP_UPSHIFT_CT>::fir_interpolate_hb_ref(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    int inputIndex = 0;
    for (int i = 0; i < TP_FIR_LEN; ++i) {
        if (i == m_kCentreTapInternalPos) {
            if (TP_UPSHIFT_CT == 0) {
                m_internalTaps[i] = taps[m_kCentreTapInputPos];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
                m_ctShift = getUpshiftCt(taps[(TP_FIR_LEN + 1) / 4]);
            }
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

// Constructor to poulate m_internalTaps array for DUAL_OP case -just a clone of the base constructor. TODO. Find
// elegant way to call unspecializaed constructor
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT>
fir_interpolate_hb_ref<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       USE_COEFF_RELOAD_FALSE,
                       2,
                       TP_UPSHIFT_CT>::fir_interpolate_hb_ref(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    int inputIndex = 0;
    for (int i = 0; i < TP_FIR_LEN; ++i) {
        if (i == m_kCentreTapInternalPos) {
            if (TP_UPSHIFT_CT == 0) {
                m_internalTaps[i] = taps[m_kCentreTapInputPos];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
                m_ctShift = getUpshiftCt(taps[(TP_FIR_LEN + 1) / 4]);
            }
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

// Constructor reload coeffs, single output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT>
fir_interpolate_hb_ref<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       USE_COEFF_RELOAD_TRUE,
                       1,
                       TP_UPSHIFT_CT>::fir_interpolate_hb_ref(){};

// Constructor reload coeffs, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT>
fir_interpolate_hb_ref<TT_DATA,
                       TT_COEFF,
                       TP_FIR_LEN,
                       TP_SHIFT,
                       TP_RND,
                       TP_INPUT_WINDOW_VSIZE,
                       USE_COEFF_RELOAD_TRUE,
                       2,
                       TP_UPSHIFT_CT>::fir_interpolate_hb_ref(){};

//------------------------------------------------------------------------
// reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT>
void fir_interpolate_hb_ref<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT>::firReload(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    int inputIndex = 0;
    for (int i = 0; i < TP_FIR_LEN; ++i) {
        if (i == m_kCentreTapInternalPos) {
            if (TP_UPSHIFT_CT == 0) {
                m_internalTaps[i] = taps[m_kCentreTapInputPos];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
                m_ctShift = getUpshiftCt(taps[(TP_FIR_LEN + 1) / 4]);
            }
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
}

// reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT>
void fir_interpolate_hb_ref<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            TP_UPSHIFT_CT>::firReload(const TT_COEFF (&taps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    int inputIndex = 0;
    for (int i = 0; i < TP_FIR_LEN; ++i) {
        if (i == m_kCentreTapInternalPos) {
            if (TP_UPSHIFT_CT == 0) {
                m_internalTaps[i] = taps[m_kCentreTapInputPos];
            } else {
                m_internalTaps[i] = nullElem<TT_COEFF>();
                m_ctShift = getUpshiftCt(taps[(TP_FIR_LEN + 1) / 4]);
            }
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
}

//------------------------------------------------------------------------
// REF FIR function - no coefficient reload, single output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_COEFF_RELOAD,
          unsigned int TP_NUM_OUTPUTS,
          unsigned int TP_UPSHIFT_CT>
void fir_interpolate_hb_ref<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            TP_USE_COEFF_RELOAD,
                            TP_NUM_OUTPUTS,
                            TP_UPSHIFT_CT>::filter(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;  // fir first polyphase
    T_accRef<TT_DATA> accum2; // for centre tap polyphase - polyphase #2
    TT_DATA d_in;
    unsigned int dataReads = 0;
    TT_DATA accumSrs;

    const unsigned short kInterpolateFactor = 2;
    const unsigned int kFirMarginLen = (TP_FIR_LEN + 1) / kInterpolateFactor;
    const unsigned int kFirMarginOffset =
        fnFirMargin<kFirMarginLen, TT_DATA>() - kFirMarginLen + 1; // FIR Margin Offset.

    printf("Ref model params:\n");
    printf("TP_FIR_LEN = %d\n", (int)TP_FIR_LEN);
    printf("TP_SHIFT = %d\n", (int)TP_SHIFT);
    printf("TP_RND = %d\n", (int)TP_RND);
    printf("TP_INPUT_WINDOW_SIZE = %d\n", (int)TP_INPUT_WINDOW_VSIZE);
    printf("kFirMarginOffset = %d\n", kFirMarginOffset);
    printf("m_kDataSampleCentre = %d\n", m_kDataSampleCentre);
    for (int i = 0; i < TP_FIR_LEN; i++) {
        printf(" Ref Coeffs[%d]: %d \n", i, m_internalTaps[i]); // only real coeffs!
    }

    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding
    // two outputs for each input in window size
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>();  // reset accumulator at the start of the mult-add for each output sample
        accum2 = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;
        // FIR chain
        // A halfband interpolator takes in each datasample to both of 2 polyphases.
        // A sample is output per polyphase, hence the 2* rate change.
        // The top phase works on even, symmetrical  coefficients.
        // the bottom phase works on odd coefficients, which for halfband are all 0 except
        // the centre tap, hence the equation for the lower polyphase is a single operation.
        for (unsigned int j = 0; j < (TP_FIR_LEN + 1) / 2; j++) {
            d_in = window_readincr(inWindow);
            dataReads++;
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in, m_internalTaps[(TP_FIR_LEN - 1) - j * 2]);
            if (j == m_kDataSampleCentre) {
                if (m_useCentreTapShift) {
                    multiplyAccUct<TT_DATA>(accum2, d_in, m_ctShift);
                } else {
                    multiplyAcc<TT_DATA, TT_COEFF>(accum2, d_in, m_internalTaps[(TP_FIR_LEN - 1) - j * 2 + 1]);
                }
            }
        }
        // printf("final acc = %d, acc2 = %d\n",(int32_t)accum.real, (int32_t)accum2.real);

        // The centre tap is in the second polyphase. Due to the reversal of coefficients in the aie
        // processor it is necessary to read out the polyphases in reverse order.
        // Note, that the very first data sample does not have a 0 coefficient.
        // 1 2 3 4 4 3 2 1    - Even polyphase has 8 coefficients
        // 0 0 0 5 0 0 0      - Odd polyphase only has 7 coefficients

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);

        roundAcc(TP_RND, shift, accum2);
        saturateAcc(accum2);
        accumSrs = castAcc(accum2);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);

        // Revert data pointer for next sample
        window_decr(inWindow, dataReads - 1);
    }
};

// REF FIR function - no coefficient reload, dual output
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT>
void fir_interpolate_hb_ref<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            USE_COEFF_RELOAD_FALSE,
                            2,
                            TP_UPSHIFT_CT>::filter(input_window<TT_DATA>* inWindow,
                                                   output_window<TT_DATA>* outWindow,
                                                   output_window<TT_DATA>* outWindow2) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;  // fir first polyphase
    T_accRef<TT_DATA> accum2; // for centre tap polyphase - polyphase #2
    TT_DATA d_in;
    unsigned int dataReads = 0;
    TT_DATA accumSrs;

    const unsigned short kInterpolateFactor = 2;
    const unsigned int kFirMarginLen = (TP_FIR_LEN + 1) / kInterpolateFactor;
    const unsigned int kFirMarginOffset =
        fnFirMargin<kFirMarginLen, TT_DATA>() - kFirMarginLen + 1; // FIR Margin Offset.

    printf("Ref model params:\n");
    printf("TP_FIR_LEN = %d\n", (int)TP_FIR_LEN);
    printf("TP_SHIFT = %d\n", (int)TP_SHIFT);
    printf("TP_RND = %d\n", (int)TP_RND);
    printf("TP_INPUT_WINDOW_SIZE = %d\n", (int)TP_INPUT_WINDOW_VSIZE);
    printf("kFirMarginOffset = %d\n", kFirMarginOffset);
    printf("m_kDataSampleCentre = %d\n", m_kDataSampleCentre);
    for (int i = 0; i < TP_FIR_LEN; i++) {
        printf(" Ref Coeffs[%d]: %d \n", i, m_internalTaps[i]); // only real coeffs!
    }

    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding
    // two outputs for each input in window size
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>();  // reset accumulator at the start of the mult-add for each output sample
        accum2 = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;
        // FIR chain
        // A halfband interpolator takes in each datasample to both of 2 polyphases.
        // A sample is output per polyphase, hence the 2* rate change.
        // The top phase works on even, symmetrical  coefficients.
        // the bottom phase works on odd coefficients, which for halfband are all 0 except
        // the centre tap, hence the equation for the lower polyphase is a single operation.
        for (unsigned int j = 0; j < (TP_FIR_LEN + 1) / 2; j++) {
            d_in = window_readincr(inWindow);
            dataReads++;
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in, m_internalTaps[(TP_FIR_LEN - 1) - j * 2]);
            if (j == m_kDataSampleCentre) {
                if (m_useCentreTapShift) {
                    multiplyAccUct<TT_DATA>(accum2, d_in, m_ctShift);
                } else {
                    multiplyAcc<TT_DATA, TT_COEFF>(accum2, d_in, m_internalTaps[(TP_FIR_LEN - 1) - j * 2 + 1]);
                }
            }
        }
        // printf("final acc = %d, acc2 = %d\n",(int32_t)accum.real, (int32_t)accum2.real);

        // The centre tap is in the second polyphase. Due to the reversal of coefficients in the aie
        // processor it is necessary to read out the polyphases in reverse order.
        // Note, that the very first data sample does not have a 0 coefficient.
        // 1 2 3 4 4 3 2 1    - Even polyphase has 8 coefficients
        // 0 0 0 5 0 0 0      - Odd polyphase only has 7 coefficients

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
        window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);

        roundAcc(TP_RND, shift, accum2);
        saturateAcc(accum2);
        accumSrs = castAcc(accum2);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
        window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);

        // Revert data pointer for next sample
        window_decr(inWindow, dataReads - 1);
    }
};

// REF FIR function - using coefficient reload, single output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT>
void fir_interpolate_hb_ref<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            USE_COEFF_RELOAD_TRUE,
                            1,
                            TP_UPSHIFT_CT>::filter(input_window<TT_DATA>* inWindow,
                                                   output_window<TT_DATA>* outWindow,
                                                   const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    firReload(inTaps);
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;  // fir first polyphase
    T_accRef<TT_DATA> accum2; // for centre tap polyphase - polyphase #2
    TT_DATA d_in;
    unsigned int dataReads = 0;
    TT_DATA accumSrs;

    const unsigned short kInterpolateFactor = 2;
    const unsigned int kFirMarginLen = (TP_FIR_LEN + 1) / kInterpolateFactor;
    const unsigned int kFirMarginOffset =
        fnFirMargin<kFirMarginLen, TT_DATA>() - kFirMarginLen + 1; // FIR Margin Offset.

    printf("Ref model params:\n");
    printf("TP_FIR_LEN = %d\n", TP_FIR_LEN);
    printf("TP_SHIFT = %d\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_INPUT_WINDOW_SIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    printf("kFirMarginOffset = %d\n", kFirMarginOffset);
    printf("m_kDataSampleCentre = %d\n", m_kDataSampleCentre);
    for (int i = 0; i < TP_FIR_LEN; i++) {
        printf(" Ref Coeffs[%d]: %d \n", i, m_internalTaps[i]); // only real coeffs!
    }

    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding
    // two outputs for each input in window size
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>();  // reset accumulator at the start of the mult-add for each output sample
        accum2 = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;
        // FIR chain
        // A halfband interpolator takes in each datasample to both of 2 polyphases.
        // A sample is output per polyphase, hence the 2* rate change.
        // The top phase works on even, symmetrical  coefficients.
        // the bottom phase works on odd coefficients, which for halfband are all 0 except
        // the centre tap, hence the equation for the lower polyphase is a single operation.
        for (unsigned int j = 0; j < (TP_FIR_LEN + 1) / 2; j++) {
            d_in = window_readincr(inWindow);
            dataReads++;
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in, m_internalTaps[(TP_FIR_LEN - 1) - j * 2]);
            if (j == m_kDataSampleCentre) {
                if (m_useCentreTapShift) {
                    multiplyAccUct<TT_DATA>(accum2, d_in, m_ctShift);
                } else {
                    multiplyAcc<TT_DATA, TT_COEFF>(accum2, d_in, m_internalTaps[(TP_FIR_LEN - 1) - j * 2 + 1]);
                }
            }
        }

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);

        // The centre tap is in the second polyphase. Due to the reversal of coefficients in the aie
        // processor it is necessary to read out the polyphases in reverse order.
        roundAcc(TP_RND, shift, accum2);
        saturateAcc(accum2);
        accumSrs = castAcc(accum2);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);

        // Revert data pointer for next sample
        window_decr(inWindow, dataReads - 1);
    }
};

// REF FIR function - using coefficient reload, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          size_t TP_FIR_LEN,
          size_t TP_SHIFT,
          unsigned int TP_RND,
          unsigned TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_UPSHIFT_CT>
void fir_interpolate_hb_ref<TT_DATA,
                            TT_COEFF,
                            TP_FIR_LEN,
                            TP_SHIFT,
                            TP_RND,
                            TP_INPUT_WINDOW_VSIZE,
                            USE_COEFF_RELOAD_TRUE,
                            2,
                            TP_UPSHIFT_CT>::filter(input_window<TT_DATA>* inWindow,
                                                   output_window<TT_DATA>* outWindow,
                                                   output_window<TT_DATA>* outWindow2,
                                                   const TT_COEFF (&inTaps)[(TP_FIR_LEN + 1) / 4 + 1]) {
    firReload(inTaps);
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;  // fir first polyphase
    T_accRef<TT_DATA> accum2; // for centre tap polyphase - polyphase #2
    TT_DATA d_in;
    unsigned int dataReads = 0;
    TT_DATA accumSrs;

    const unsigned short kInterpolateFactor = 2;
    const unsigned int kFirMarginLen = (TP_FIR_LEN + 1) / kInterpolateFactor;
    const unsigned int kFirMarginOffset =
        fnFirMargin<kFirMarginLen, TT_DATA>() - kFirMarginLen + 1; // FIR Margin Offset.

    printf("Ref model params:\n");
    printf("TP_FIR_LEN = %d\n", (int)TP_FIR_LEN);
    printf("TP_SHIFT = %d\n", (int)TP_SHIFT);
    printf("TP_RND = %d\n", (int)TP_RND);
    printf("TP_INPUT_WINDOW_SIZE = %d\n", (int)TP_INPUT_WINDOW_VSIZE);
    printf("kFirMarginOffset = %d\n", kFirMarginOffset);
    printf("m_kDataSampleCentre = %d\n", m_kDataSampleCentre);
    for (int i = 0; i < TP_FIR_LEN; i++) {
        printf(" Ref Coeffs[%d]: %d \n", i, m_internalTaps[i]); // only real coeffs!
    }

    window_incr(inWindow, kFirMarginOffset); // move input data pointer past the margin padding
    // two outputs for each input in window size
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>();  // reset accumulator at the start of the mult-add for each output sample
        accum2 = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        dataReads = 0;
        // FIR chain
        // A halfband interpolator takes in each datasample to both of 2 polyphases.
        // A sample is output per polyphase, hence the 2* rate change.
        // The top phase works on even, symmetrical  coefficients.
        // the bottom phase works on odd coefficients, which for halfband are all 0 except
        // the centre tap, hence the equation for the lower polyphase is a single operation.
        for (unsigned int j = 0; j < (TP_FIR_LEN + 1) / 2; j++) {
            d_in = window_readincr(inWindow);
            dataReads++;
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in, m_internalTaps[(TP_FIR_LEN - 1) - j * 2]);
            if (j == m_kDataSampleCentre) {
                if (m_useCentreTapShift) {
                    multiplyAccUct<TT_DATA>(accum2, d_in, m_ctShift);
                } else {
                    multiplyAcc<TT_DATA, TT_COEFF>(accum2, d_in, m_internalTaps[(TP_FIR_LEN - 1) - j * 2 + 1]);
                }
            }
        }

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
        window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);

        // The centre tap is in the second polyphase. Due to the reversal of coefficients in the aie
        // processor it is necessary to read out the polyphases in reverse order.
        roundAcc(TP_RND, shift, accum2);
        saturateAcc(accum2);
        accumSrs = castAcc(accum2);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
        window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);

        // Revert data pointer for next sample
        window_decr(inWindow, dataReads - 1);
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
