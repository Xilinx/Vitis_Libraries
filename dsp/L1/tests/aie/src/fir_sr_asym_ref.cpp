/*
 * Copyright 2021 Xilinx, Inc.
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

#include "fir_sr_asym_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace sr_asym {

// REF FIR function - default/base 'specialization'
// Static coefficients
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
          unsigned int TP_API>
void fir_sr_asym_ref<TT_DATA,
                     TT_COEFF,
                     TP_FIR_LEN,
                     TP_SHIFT,
                     TP_RND,
                     TP_INPUT_WINDOW_VSIZE,
                     TP_USE_COEFF_RELOAD,
                     TP_NUM_OUTPUTS,
                     TP_DUAL_IP,
                     TP_API>::filter(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;

    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset);                                                   // read input data

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d_in[j] = window_readincr(inWindow); // read input data
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], internalTaps[TP_FIR_LEN - 1 - j]);
        }
        // Revert data pointer for next sample
        window_decr(inWindow, TP_FIR_LEN - 1);

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
    }
};

// specialization for static coeffs and dual outputs
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API>
void fir_sr_asym_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, 0, 2, 0, TP_API>::filter(
    input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow, output_window<TT_DATA>* outWindow2) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;
    int phase = 0;

    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset);                                                   // read input data

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d_in[j] = window_readincr(inWindow); // read input data
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], internalTaps[TP_FIR_LEN - 1 - j]);
        }
        // Revert data pointer for next sample
        window_decr(inWindow, TP_FIR_LEN - 1);

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);

        if (TP_API == 0) {
            window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
            window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);
        } else {
            // To max HW performance, data is interleaved with 128-bit data blocks.
            static constexpr unsigned streamBitWidth = 128;
            static constexpr unsigned streamByteWidth = streamBitWidth / 8;
            int outPhase = (phase++ * sizeof(TT_DATA)) % (2 * streamByteWidth);
            if (outPhase < streamByteWidth) {
                window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
            } else {
                window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);
            }
        }
    }
};

// REF FIR function
// Reloadable coefficients
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API>
void fir_sr_asym_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, 1, 1, 0, TP_API>::filter(
    input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow, const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    firReload(inTaps);
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;

    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset);                                                   // read input data

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d_in[j] = window_readincr(inWindow); // read input data
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], internalTaps[TP_FIR_LEN - 1 - j]);
        }
        // Revert data pointer for next sample
        window_decr(inWindow, TP_FIR_LEN - 1);

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
    }
};

// specialization for Reloadable coefficients, dual output
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API>
void fir_sr_asym_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, 1, 2, 0, TP_API>::filter(
    input_window<TT_DATA>* inWindow,
    output_window<TT_DATA>* outWindow,
    output_window<TT_DATA>* outWindow2,
    const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    firReload(inTaps);
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;
    int phase = 0;

    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    window_incr(inWindow, kFirMarginOffset);                                                   // read input data

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            d_in[j] = window_readincr(inWindow); // read input data
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], internalTaps[TP_FIR_LEN - 1 - j]);
        }
        // Revert data pointer for next sample
        window_decr(inWindow, TP_FIR_LEN - 1);

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);

        if (TP_API == 0) {
            window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
            window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);
        } else {
            // To max HW performance, data is interleaved with 128-bit data blocks.
            static constexpr unsigned streamBitWidth = 128;
            static constexpr unsigned streamByteWidth = streamBitWidth / 8;
            int outPhase = (phase++ * sizeof(TT_DATA)) % (2 * streamByteWidth);
            if (outPhase < streamByteWidth) {
                window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
            } else {
                window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);
            }
        }
    }
};

// specialization for static coeffs and dual outputs
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API>
void fir_sr_asym_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, 0, 2, 1, TP_API>::filter(
    input_window<TT_DATA>* inWindow,
    input_window<TT_DATA>* inWindow2,
    output_window<TT_DATA>* outWindow,
    output_window<TT_DATA>* outWindow2) {
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;
    int inPhase = 0;
    int outPhase = 0;
    int inReadsS1 = 0;
    int inReadsS2 = 0;
    static constexpr unsigned streamBitWidth = 128;
    static constexpr unsigned streamByteWidth = streamBitWidth / 8;

    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    const unsigned int firMarginToStream =
        (kFirMarginOffset > (16 / sizeof(TT_DATA))) ? (16 / sizeof(TT_DATA)) : kFirMarginOffset;
    const unsigned int firMarginToStream2 =
        (kFirMarginOffset > (16 / sizeof(TT_DATA))) ? kFirMarginOffset - (16 / sizeof(TT_DATA)) : 0;
    // const unsigned int inWindowOffset = fnFirMargin<TP_FIR_LEN,TT_DATA>() / 2 + kFirMarginOffset;
    // const unsigned int inWindow2Offset = fnFirMargin<TP_FIR_LEN,TT_DATA>() / 2 + kFirMarginOffset;
    const unsigned int inWindowOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() / 2 + firMarginToStream;
    const unsigned int inWindow2Offset = fnFirMargin<TP_FIR_LEN, TT_DATA>() / 2 + firMarginToStream2;

    window_incr(inWindow, inWindowOffset);   // read input data
    window_incr(inWindow2, inWindow2Offset); // kFirMarginOffset / 128-bits already moved by in the read above.
    printf("firMarginToStream = %d\n", firMarginToStream);
    printf("firMarginToStream2 = %d\n", firMarginToStream2);
    printf("inWindowOffset = %d\n", inWindowOffset);
    printf("inWindow2Offset = %d\n", inWindow2Offset);
    printf("TP_FIR_LEN = %d\n", TP_FIR_LEN);
    printf("TP_SHIFT = %d\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_INPUT_WINDOW_VSIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    printf("TP_API = %d\n", TP_API);
    printf("kFirMarginOffset=%d \n", kFirMarginOffset);

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        inReadsS1 = 0;
        inReadsS2 = 0;
        inPhase = kFirMarginOffset + i;
        int inStreamPhase = 0;

        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            inStreamPhase = (inPhase++ * sizeof(TT_DATA)) % (2 * streamByteWidth);
            if (inStreamPhase < streamByteWidth) {
                d_in[j] = window_readincr(inWindow);
                inReadsS1++;
                printf("D[%d], stream1 real=%d, imag=%d, inStreamPhase %d\n", i, d_in[j].real, d_in[j].imag,
                       inStreamPhase);
            } else {
                d_in[j] = window_readincr(inWindow2);
                inReadsS2++;
                printf("D[%d], stream2 real=%d, imag=%d, inStreamPhase %d\n", i, d_in[j].real, d_in[j].imag,
                       inStreamPhase);
            }
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], internalTaps[TP_FIR_LEN - 1 - j]);
        }
        // Prep data pointers for next sample
        int inStreamNextPhase = ((kFirMarginOffset + i) * sizeof(TT_DATA)) % (2 * streamByteWidth);
        if (inStreamNextPhase < streamByteWidth) {
            printf("D[%d], stream1 inStreamNextPhase %d\n", i, inStreamNextPhase);
            inReadsS1--;
        } else {
            printf("D[%d], stream2 inStreamNextPhase %d\n", i, inStreamNextPhase);
            inReadsS2--;
        }

        // Revert data pointer for next sample
        window_decr(inWindow, inReadsS1);
        window_decr(inWindow2, inReadsS2);

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        printf(" inReadsS1=%d, inReadsS2=%d \n", inReadsS1, inReadsS2);
        printf("ACC real=%d, imag=%d \n", accumSrs.real, accumSrs.imag);

        if (TP_API == 0) {
            window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
            window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);
        } else {
            // To max HW performance, data is interleaved with 128-bit data blocks.
            int outStreamPhase = (outPhase++ * sizeof(TT_DATA)) % (2 * streamByteWidth);
            if (outStreamPhase < streamByteWidth) {
                window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
            } else {
                window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);
            }
        }
    }
};

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_API>
void fir_sr_asym_ref<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_SHIFT, TP_RND, TP_INPUT_WINDOW_VSIZE, 1, 2, 1, TP_API>::filter(
    input_window<TT_DATA>* inWindow,
    input_window<TT_DATA>* inWindow2,
    output_window<TT_DATA>* outWindow,
    output_window<TT_DATA>* outWindow2,
    const TT_COEFF (&inTaps)[TP_FIR_LEN]) {
    firReload(inTaps);
    const unsigned int shift = TP_SHIFT;
    T_accRef<TT_DATA> accum;
    TT_DATA d_in[TP_FIR_LEN];
    TT_DATA accumSrs;
    int inPhase = 0;
    int outPhase = 0;
    int inReadsS1 = 0;
    int inReadsS2 = 0;
    static constexpr unsigned streamBitWidth = 128;
    static constexpr unsigned streamByteWidth = streamBitWidth / 8;

    const unsigned int kFirMarginOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() - TP_FIR_LEN + 1; // FIR Margin Offset.
    const unsigned int firMarginToStream =
        (kFirMarginOffset > (16 / sizeof(TT_DATA))) ? (16 / sizeof(TT_DATA)) : kFirMarginOffset;
    const unsigned int firMarginToStream2 =
        (kFirMarginOffset > (16 / sizeof(TT_DATA))) ? kFirMarginOffset - (16 / sizeof(TT_DATA)) : 0;
    // const unsigned int inWindowOffset = fnFirMargin<TP_FIR_LEN,TT_DATA>() / 2 + kFirMarginOffset;
    // const unsigned int inWindow2Offset = fnFirMargin<TP_FIR_LEN,TT_DATA>() / 2 + kFirMarginOffset;
    const unsigned int inWindowOffset = fnFirMargin<TP_FIR_LEN, TT_DATA>() / 2 + firMarginToStream;
    const unsigned int inWindow2Offset = fnFirMargin<TP_FIR_LEN, TT_DATA>() / 2 + firMarginToStream2;

    window_incr(inWindow, inWindowOffset);   // read input data
    window_incr(inWindow2, inWindow2Offset); // kFirMarginOffset / 128-bits already moved by in the read above.
    printf("firMarginToStream = %d\n", firMarginToStream);
    printf("firMarginToStream2 = %d\n", firMarginToStream2);
    printf("inWindowOffset = %d\n", inWindowOffset);
    printf("inWindow2Offset = %d\n", inWindow2Offset);
    printf("TP_FIR_LEN = %d\n", TP_FIR_LEN);
    printf("TP_SHIFT = %d\n", TP_SHIFT);
    printf("TP_RND = %d\n", TP_RND);
    printf("TP_INPUT_WINDOW_VSIZE = %d\n", TP_INPUT_WINDOW_VSIZE);
    printf("TP_API = %d\n", TP_API);
    printf("kFirMarginOffset=%d \n", kFirMarginOffset);

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        inReadsS1 = 0;
        inReadsS2 = 0;
        inPhase = kFirMarginOffset + i;
        int inStreamPhase = 0;

        accum = null_accRef<TT_DATA>(); // reset accumulator at the start of the mult-add for each output sample
        // Accumulation
        for (unsigned int j = 0; j < TP_FIR_LEN; j++) {
            inStreamPhase = (inPhase++ * sizeof(TT_DATA)) % (2 * streamByteWidth);
            if (inStreamPhase < streamByteWidth) {
                d_in[j] = window_readincr(inWindow);
                inReadsS1++;
                printf("D[%d], stream1 real=%d, imag=%d, inStreamPhase %d\n", i, d_in[j].real, d_in[j].imag,
                       inStreamPhase);
            } else {
                d_in[j] = window_readincr(inWindow2);
                inReadsS2++;
                printf("D[%d], stream2 real=%d, imag=%d, inStreamPhase %d\n", i, d_in[j].real, d_in[j].imag,
                       inStreamPhase);
            }
            // Note the coefficient index reversal. See note in constructor.
            multiplyAcc<TT_DATA, TT_COEFF>(accum, d_in[j], internalTaps[TP_FIR_LEN - 1 - j]);
        }
        // Prep data pointers for next sample
        int inStreamNextPhase = ((kFirMarginOffset + i) * sizeof(TT_DATA)) % (2 * streamByteWidth);
        if (inStreamNextPhase < streamByteWidth) {
            printf("D[%d], stream1 inStreamNextPhase %d\n", i, inStreamNextPhase);
            inReadsS1--;
        } else {
            printf("D[%d], stream2 inStreamNextPhase %d\n", i, inStreamNextPhase);
            inReadsS2--;
        }

        // Revert data pointer for next sample
        window_decr(inWindow, inReadsS1);
        window_decr(inWindow2, inReadsS2);

        roundAcc(TP_RND, shift, accum);
        saturateAcc(accum);
        accumSrs = castAcc(accum);
        printf(" inReadsS1=%d, inReadsS2=%d \n", inReadsS1, inReadsS2);
        printf("ACC real=%d, imag=%d \n", accumSrs.real, accumSrs.imag);

        if (TP_API == 0) {
            window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
            window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);
        } else {
            // To max HW performance, data is interleaved with 128-bit data blocks.
            int outStreamPhase = (outPhase++ * sizeof(TT_DATA)) % (2 * streamByteWidth);
            if (outStreamPhase < streamByteWidth) {
                window_writeincr((output_window<TT_DATA>*)outWindow, accumSrs);
            } else {
                window_writeincr((output_window<TT_DATA>*)outWindow2, accumSrs);
            }
        }
    }
};
}
}
}
}
}
