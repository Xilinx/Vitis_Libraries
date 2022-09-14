/*
 * Copyright 2022 Xilinx, Inc.
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
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "fft_ifft_dit_1ch_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dit_1ch {

// unitVector cannot be in fft_ref_utils because that is used by 2 different kernels, so leads to multiple definition.
template <typename T_D>
constexpr T_D unitVector(){};
template <>
constexpr cint16 unitVector<cint16>() {
    cint16 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
constexpr cint32 unitVector<cint32>() {
    cint32 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
constexpr cfloat unitVector<cfloat>() {
    cfloat temp;
    temp.real = 1.0;
    temp.imag = 0.0;
    return temp;
};

template <typename T_D>
constexpr T_D blankVector(){};
template <>
constexpr cint16 blankVector<cint16>() {
    cint16 temp;
    temp.real = 0;
    temp.imag = 0;
    return temp;
};
template <>
constexpr cint32 blankVector<cint32>() {
    cint32 temp;
    temp.real = 0;
    temp.imag = 0;
    return temp;
};
template <>
constexpr cfloat blankVector<cfloat>() {
    cfloat temp;
    temp.real = 0.0;
    temp.imag = 0.0;
    return temp;
};

/*
  FFT/iFFT DIT single channel reference model
*/

//---------------------------------------------------------
// templatized Radix2 stage
// First stage in DIT has trivial twiddles (1,0), but this is the one twiddle which isn't exact, so cannot be treated as
// trivial.
template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
void fft_ifft_dit_1ch_ref<TT_DATA,
                          TT_TWIDDLE,
                          TP_POINT_SIZE,
                          TP_FFT_NIFFT,
                          TP_SHIFT,
                          TP_DYN_PT_SIZE,
                          TP_WINDOW_VSIZE,
                          TP_ORIG_PAR_POWER>::r2StageInt(T_int_data<TT_DATA>* samplesA,
                                                         T_int_data<TT_DATA>* samplesB,
                                                         TT_TWIDDLE* twiddles,
                                                         int pptSize,
                                                         bool inv) {
    int ptSize = TP_DYN_PT_SIZE == 0 ? TP_POINT_SIZE : pptSize;
    T_int_data<TT_DATA> sam1, sam2, sam2rot;
    int64 sum;
    TT_TWIDDLE tw;
    const unsigned int shift = 15;
    const unsigned int round_const = (1 << (shift - 1));
    for (int op = 0; op < (ptSize >> 1); op++) {
        tw.real = twiddles[0].real;
        tw.imag = inv ? -twiddles[0].imag : twiddles[0].imag;
        sam1.real = (int64)samplesA[2 * op].real << shift;
        sam1.imag = (int64)samplesA[2 * op].imag << shift;
        sam2 = samplesA[2 * op + 1];
        if (inv) {
            sam2rot.real = (int64)sam2.real * tw.real + (int64)sam2.imag * tw.imag;
            sam2rot.imag = (int64)sam2.imag * tw.real - (int64)sam2.real * tw.imag;
        } else {
            sam2rot.real = (int64)sam2.real * tw.real - (int64)sam2.imag * tw.imag;
            sam2rot.imag = (int64)sam2.real * tw.imag + (int64)sam2.imag * tw.real;
        }
        sum = (int64)sam1.real + (int64)sam2rot.real + (int64)round_const;
        samplesB[2 * op].real = (int32)(sum >> shift);
        samplesB[2 * op].imag = (int32)(((int64)sam1.imag + (int64)sam2rot.imag + (int64)round_const) >> shift);
        samplesB[2 * op + 1].real = (int32)(((int64)sam1.real - (int64)sam2rot.real + (int64)round_const) >> shift);
        samplesB[2 * op + 1].imag = (int32)(((int64)sam1.imag - (int64)sam2rot.imag + (int64)round_const) >> shift);
        // printf("in[%d] = (%d, %d) and (%d,%d), out = [%d, %d],[%d, %d]\n",op, sam1.real, sam1.imag, sam2.real,
        // sam2.imag, samplesB[2%op].real, samplesB[2%op].imag,  samplesB[2%op+1].real, samplesB[2%op+1].imag );
    }
}

template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
void fft_ifft_dit_1ch_ref<TT_DATA,
                          TT_TWIDDLE,
                          TP_POINT_SIZE,
                          TP_FFT_NIFFT,
                          TP_SHIFT,
                          TP_DYN_PT_SIZE,
                          TP_WINDOW_VSIZE,
                          TP_ORIG_PAR_POWER>::r2StageFloat(T_int_data<TT_DATA>* samplesA,
                                                           T_int_data<TT_DATA>* samplesB,
                                                           TT_TWIDDLE* twiddles,
                                                           unsigned int rank,
                                                           int pptSize,
                                                           bool inv) {
    int ptSize = TP_DYN_PT_SIZE == 0 ? TP_POINT_SIZE : pptSize;
    constexpr unsigned int kRadix = 2;
    unsigned int opLowMask = (1 << rank) - 1;
    unsigned int opHiMask = ptSize - 1 - opLowMask;
    T_int_data<cfloat> sam1, sam2, sam2rot;
    unsigned int inIndex[kRadix];
    cfloat tw;
    unsigned int temp1, temp2, twIndex;
    for (int op = 0; op < (ptSize >> 1); op++) {
        for (int i = 0; i < 2; i++) {
            inIndex[i] = ((op & opHiMask) << 1) + (i << rank) + (op & opLowMask);
        }
        temp1 = inIndex[0] << (kMaxLogPtSize - 1 - rank);
        temp2 = temp1 & ((1 << (kMaxLogPtSize - 1)) - 1);
        twIndex = temp2;
        tw.real = twiddles[twIndex].real;
        tw.imag = inv ? -twiddles[twIndex].imag : twiddles[twIndex].imag;
        sam1.real = samplesA[inIndex[0]].real;
        sam1.imag = samplesA[inIndex[0]].imag;
        sam2.real = samplesA[inIndex[1]].real;
        sam2.imag = samplesA[inIndex[1]].imag;
        // sam2rot.real = sam2.real * tw.real - sam2.imag * tw.imag;
        // sam2rot.imag = sam2.real * tw.imag + sam2.imag * tw.real;
        samplesB[inIndex[0]].real = +sam1.real + (-sam2.imag * tw.imag + sam2.real * tw.real); // sam2rot.real;
        samplesB[inIndex[0]].imag = +sam1.imag + (+sam2.imag * tw.real + sam2.real * tw.imag); // sam2rot.imag;
        samplesB[inIndex[1]].real = +sam1.real + (+sam2.imag * tw.imag - sam2.real * tw.real); // sam2rot.real;
        samplesB[inIndex[1]].imag = +sam1.imag + (-sam2.imag * tw.real - sam2.real * tw.imag); // sam2rot.imag;
    }
}

template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
void fft_ifft_dit_1ch_ref<TT_DATA,
                          TT_TWIDDLE,
                          TP_POINT_SIZE,
                          TP_FFT_NIFFT,
                          TP_SHIFT,
                          TP_DYN_PT_SIZE,
                          TP_WINDOW_VSIZE,
                          TP_ORIG_PAR_POWER>::r4StageInt(T_int_data<TT_DATA>* samplesIn,
                                                         TT_TWIDDLE* twiddles1,
                                                         TT_TWIDDLE* twiddles2,
                                                         unsigned int n,
                                                         unsigned int r,
                                                         unsigned int shift,
                                                         unsigned int rank,
                                                         T_int_data<TT_DATA>* samplesOut,
                                                         int pptSize,
                                                         bool inv) {
    int ptSize = TP_DYN_PT_SIZE == 0 ? TP_POINT_SIZE : pptSize;
    constexpr unsigned int kMaxPointSize = 4096;
    constexpr unsigned int kRadix = 4;    // actually spoofed by 4 radix2 operations.
    constexpr unsigned int stdShift = 15; // derived from cint16's binary point position.
    T_int_data<TT_DATA> sam0, sam1, sam2, sam3;
    TT_TWIDDLE tw[kRadix];
    unsigned int inIndex[kRadix];
    unsigned int twIndex[kRadix];
    unsigned int temp1, temp2;
    cint32 sam2raw, sam3raw;
    cint64 sam2rot, sam3rot;
    cint64 a0, a1, a2, a3, o0, o1, o2, o3;
    T_int_data<TT_DATA> y0, y1, y2, y3;
    T_int_data<TT_DATA> yd0, yd1, yd2, yd3;
    T_int_data<TT_DATA> z0, z1, z2, z3;
    cint64 y1rot, y3rot;
    unsigned int opLowMask = (1 << rank) - 1;
    unsigned int opHiMask = ptSize - 1 - opLowMask;
    const unsigned int round_const = (1 << (shift - 1));

    for (int op = 0; op < (ptSize >> 2); op++) {
        for (int i = 0; i < 4; i++) {
            inIndex[i] = ((op & opHiMask) << 2) + (i << rank) + (op & opLowMask);
        }
        temp1 = inIndex[0] << (kMaxLogPtSize - 1 - rank);
        temp2 = temp1 & ((1 << (kMaxLogPtSize - 1)) - 1);
        twIndex[0] = temp2;

        temp1 = inIndex[1] << (kMaxLogPtSize - 1 - rank);
        temp2 = temp1 & ((1 << (kMaxLogPtSize - 1)) - 1);
        twIndex[1] = temp2;

        temp1 = inIndex[2] << (kMaxLogPtSize - 1 - (rank + 1));
        temp2 = temp1 & ((1 << (kMaxLogPtSize - 1)) - 1);
        twIndex[2] = temp2;

        temp1 = inIndex[3] << (kMaxLogPtSize - 1 - (rank + 1));
        temp2 = temp1 & ((1 << (kMaxLogPtSize - 1)) - 1);
        twIndex[3] = temp2;
        for (int i = 0; i < 4; i++) {
            tw[i].real = twiddles1[twIndex[i]].real;
            tw[i].imag = twiddles1[twIndex[i]].imag;
        }
        // for second rank butterflies, minus j intrinsic is used, but sometimes due to saturation, table entries in
        // different quadrants are not exactly
        // the same as each other rotated by j, so it is necessary to mimic the UUT behaviour here
        if (twIndex[2] >= kMaxPointSize >> 2) {
            twIndex[2] -= (kMaxPointSize >> 2);
            tw[2].real = twiddles2[twIndex[2]].imag;
            tw[2].imag = -twiddles2[twIndex[2]].real;
        }
        if (twIndex[3] >= kMaxPointSize >> 2) {
            twIndex[3] -= (kMaxPointSize >> 2);
            tw[3].real = twiddles2[twIndex[3]].imag;
            tw[3].imag = -twiddles2[twIndex[3]].real;
        }

        sam0 = samplesIn[inIndex[0]];
        sam1 = samplesIn[inIndex[1]];
        sam2 = samplesIn[inIndex[2]];
        sam3 = samplesIn[inIndex[3]];
        btfly<TT_DATA, TT_TWIDDLE>(y0, y1, sam0, sam1, tw[0], inv, stdShift);
        btfly<TT_DATA, TT_TWIDDLE>(y2, y3, sam2, sam3, tw[1], inv, stdShift);
        btfly<TT_DATA, TT_TWIDDLE>(z0, z2, y0, y2, tw[2], inv, shift);
        btfly<TT_DATA, TT_TWIDDLE>(z1, z3, y1, y3, tw[3], inv, shift);
        samplesOut[inIndex[0]] = z0;
        samplesOut[inIndex[1]] = z1;
        samplesOut[inIndex[2]] = z2;
        samplesOut[inIndex[3]] = z3;
    }
}

// Bit-accurate REF FFT DIT function
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
void fft_ifft_dit_1ch_ref<TT_DATA,
                          TT_TWIDDLE,
                          TP_POINT_SIZE,
                          TP_FFT_NIFFT,
                          TP_SHIFT,
                          TP_DYN_PT_SIZE,
                          TP_WINDOW_VSIZE,
                          TP_ORIG_PAR_POWER>::fft(input_window<TT_DATA>* inWindow, output_window<TT_DATA>* outWindow) {
    constexpr unsigned int kMaxPtSizePwr = 12;                   // largest is 4096 = 1<<12;
    constexpr unsigned int kMinPtSizePwr = 4;                    // largest is 16 = 1<<4;
    constexpr unsigned int kHeaderSize = 32 / (sizeof(TT_DATA)); // dynamic point size header size in samples
    constexpr unsigned int kPtSizePwr = fnGetPointSizePower<TP_POINT_SIZE>();
    constexpr unsigned int kScaleFactor = kPtSizePwr - 1; // 1 is for initial rotation factor of 1/sqrt(2).
    constexpr unsigned int kSampleRanks = kRanks + 1;

    constexpr unsigned int kR2Stages =
        std::is_same<TT_DATA, cfloat>::value ? kPtSizePwr : (kPtSizePwr % 2 == 1 ? 1 : 0); // There is one radix 2 stage
                                                                                           // if we have an odd power of
                                                                                           // 2 point size, but for
                                                                                           // cfloat all stages are R2.
    constexpr unsigned int kR4Stages = std::is_same<TT_DATA, cfloat>::value ? 0 : kPtSizePwr / 2;
    constexpr unsigned int shift = 15; // unsigned weight (binary point position) of TT_TWIDDLE
    unsigned int stageShift = 0;

    TT_DATA sampleIn;
    T_int_data<TT_DATA> rotB; // Sample B after rotation.
    TT_TWIDDLE twiddle;
    TT_TWIDDLE twiddles[1 << (kMaxPtSizePwr - 1)];
    T_int_data<TT_DATA> chess_storage(% chess_alignof(cint32)) samplesStoreA[TP_POINT_SIZE];
    T_int_data<TT_DATA> chess_storage(% chess_alignof(cint32)) samplesStoreB[TP_POINT_SIZE];
    T_int_data<TT_DATA> inSampleA, inSampleB, outSampleA, outSampleB;
    unsigned int posLoMask, posHiMask;
    unsigned int twiddleMask, twiddleIndex, twiddlePos;
    unsigned int posLo, posHi;
    unsigned int posA, posB;
    unsigned int rank = 0;
    TT_DATA* headerPtr;
    TT_DATA header;
    int16 ptSizePwr =
        kPtSizePwr;      // default to static point size value. May be overwritten if dynamic point size selected.
    TT_DATA dummyttdata; // used to consume blank data in header.
    unsigned int r2Stages =
        kR2Stages; // default to static point size value. May be overwritten if dynamic point size selected.
    unsigned int r4Stages =
        kR4Stages; // default to static point size value. May be overwritten if dynamic point size selected.
    unsigned int ptSize =
        TP_POINT_SIZE; // default to static point size value. May be overwritten if dynamic point size selected.
    bool inv = TP_FFT_NIFFT == 1 ? false : true; // may be overwritten if dyn_pt_size is set
    TT_DATA headerOut;

    T_accRef<T_int_data<TT_DATA> > accum;
    T_int_data<TT_DATA> *samplesA, *samplesB, *tempPtr;
    const TT_TWIDDLE* twiddle_master = fnGetTwiddleMasterBase<TT_TWIDDLE>();

    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            headerPtr = (TT_DATA*)inWindow->ptr;
            header = *headerPtr++; // saved for later when output to outWindow
            window_writeincr(outWindow, header);
            inv = header.real == 0 ? true : false;
            header = *headerPtr++;                              // saved for later when output to outWindow
            ptSizePwr = (int32)header.real - TP_ORIG_PAR_POWER; // Modified for case where FFT is a subframe processor.
                                                                // Then the header refers to the overall point size.
            window_writeincr(outWindow, header);
            for (int i = 2; i < kHeaderSize - 1; i++) {
                window_writeincr(outWindow, blankVector<TT_DATA>());
            }
            if ((ptSizePwr >= kMinPtSizePwr) && (ptSizePwr <= kMaxPtSizePwr)) {
                window_writeincr(outWindow, blankVector<TT_DATA>()); // Status word. 0 indicated all ok.
            } else {
                window_writeincr(outWindow, unitVector<TT_DATA>()); // Status word. 0 indicated all ok.
            }
            window_incr(inWindow, kHeaderSize);
            // override values set for constant point size with values derived from the header in a dynamic point size
            // frame
            r2Stages = std::is_same<TT_DATA, cfloat>::value
                           ? ptSizePwr
                           : (ptSizePwr % 2 == 1 ? 1 : 0); // There is one radix 2 stage if we have an odd power of 2
                                                           // point size, but for cfloat all stages are R2.
            r4Stages = std::is_same<TT_DATA, cfloat>::value ? 0 : ptSizePwr / 2;
            ptSize = ((unsigned int)1) << ptSizePwr;
        }
    for (int opIndex = 0; opIndex < TP_WINDOW_VSIZE / TP_POINT_SIZE; opIndex++) {
        rank = 0;
        if ((ptSizePwr >= kMinPtSizePwr) && (ptSizePwr <= kMaxPtSizePwr)) {
            // read samples in
            for (unsigned int i = 0; i < ptSize; i++) {
                window_readincr(inWindow, sampleIn);
                samplesStoreA[bitRev(ptSizePwr, i)] = castInput<TT_DATA>(sampleIn);
            }
            window_incr(inWindow, TP_POINT_SIZE - ptSize);
            for (unsigned int i = 0; i < (1 << (kMaxPtSizePwr - 1)); i++) {
                twiddles[i] = twiddle_master[i];
            }

            samplesA = samplesStoreA;
            samplesB = samplesStoreB;

            for (unsigned int r2StageCnt = 0; r2StageCnt < r2Stages; r2StageCnt++) {
                if
                    constexpr(is_cfloat<TT_DATA>()) {
                        r2StageFloat(samplesA, samplesB, twiddles, r2StageCnt, ptSize, inv);
                    }
                else {
                    r2StageInt(samplesA, samplesB, twiddles, ptSize,
                               inv); // only called for the first stage so stage is implied
                }
                // Now watch carefully. The pea is under the cup labelled samplesB (the output).
                tempPtr = samplesA;
                samplesA = samplesB;
                samplesB = tempPtr;
                rank++;
                // The pea is now under the cup labelled samplesA. The next rank's input or the actual output
            }

            for (int r4StageCnt = 0; r4StageCnt < r4Stages; r4StageCnt++) {
                if (r4StageCnt == r4Stages - 1) {
                    stageShift = shift + TP_SHIFT;
                } else {
                    stageShift = shift;
                }
                r4StageInt(samplesA, twiddles, twiddles, ptSize, ptSize >> 2, stageShift, rank, samplesB, ptSize,
                           inv); //<TT_DATA,TT_TWIDDLE,TP_POINT_SIZE,TP_FFT_NIFFT>, but not required because this is a
                                 // member function.

                // Now watch carefully. The pea is under the cup labelled samplesB (the output).
                tempPtr = samplesA;
                samplesA = samplesB;
                samplesB = tempPtr;
                rank += 2;
                // The pea is now under the cup labelled samplesA. The next rank's input or the actual output
            }

            // Write samples out (natural order)
            TT_DATA outSample;
            for (unsigned int i = 0; i < ptSize; i++) {
                outSample = castOutput<TT_DATA>(samplesA[i], 0);
                window_writeincr(outWindow, outSample);
            }
            for (int i = ptSize; i < TP_POINT_SIZE; i++) {
                window_writeincr(outWindow, blankVector<TT_DATA>());
            }

        } else { // ptSizePwr is out of range
            /* THis error handling has already been done in the header clause
            window_writeincr(outWindow, header);//pass input definition header to output
            for (int i = 2; i<kHeaderSize-1 ; i++) {
              window_writeincr(outWindow, blankVector<TT_DATA>());
            }
            window_writeincr(outWindow, unitVector<TT_DATA>());  //error flag out
            */
            // write out blank frame
            for (int i = 0; i < ptSize * sizeof(TT_DATA) / sizeof(int32); i++) {
                window_writeincr(outWindow, blankVector<TT_DATA>());
            }

            rank += 2;
            // The pea is now under the cup labelled samplesA. The next rank's input or the actual output
        }
    }
};

// Non Bit-accurate REF FFT DIT function
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_ORIG_PAR_POWER>
void fft_ifft_dit_1ch_ref<TT_DATA,
                          TT_TWIDDLE,
                          TP_POINT_SIZE,
                          TP_FFT_NIFFT,
                          TP_SHIFT,
                          TP_DYN_PT_SIZE,
                          TP_WINDOW_VSIZE,
                          TP_ORIG_PAR_POWER>::nonBitAccfft(input_window<TT_DATA>* inWindow,
                                                           output_window<TT_DATA>* outWindow) {
    constexpr unsigned int kPtSizePwr = fnGetPointSizePower<TP_POINT_SIZE>();
    constexpr unsigned int kScaleFactor = TP_SHIFT; // was kPtSizePwr -1; //1 is for initial rotation factor of
                                                    // 1/sqrt(2), but with TP_SHIFT this is user-config
    constexpr unsigned int kSampleRanks = kRanks + 1;

    TT_DATA sampleIn;
    T_int_data<TT_DATA> rotB; // Sample B after rotation.
    TT_TWIDDLE twiddle;
    TT_TWIDDLE twiddles[TP_POINT_SIZE / 2];
    T_int_data<TT_DATA> chess_storage(% chess_alignof(cint16)) samplesStoreA[TP_POINT_SIZE];
    T_int_data<TT_DATA> chess_storage(% chess_alignof(cint16)) samplesStoreB[TP_POINT_SIZE];
    T_int_data<TT_DATA> inSampleA, inSampleB, outSampleA, outSampleB;
    unsigned int posLoMask, posHiMask;
    unsigned int twiddleMask, twiddleIndex, twiddlePos;
    unsigned int posLo, posHi;
    unsigned int posA, posB;
    T_accRef<T_int_data<TT_DATA> > accum;
    T_int_data<TT_DATA> *samplesA, *samplesB, *tempPtr;

    // Form twiddle table for this point size;
    for (int i = 0; i < TP_POINT_SIZE / 2; i++) {
        twiddles[i] = get_twiddle<TT_TWIDDLE>(i, TP_POINT_SIZE, TP_FFT_NIFFT);
    }

    // A window may contain multiple FFT data sets. This dilutes the overheads
    for (int iter = 0; iter < TP_WINDOW_VSIZE / TP_POINT_SIZE; iter++) {
        // read samples in
        for (unsigned int i = 0; i < TP_POINT_SIZE; i++) {
            window_readincr(inWindow, sampleIn);
            samplesStoreA[i] = castInput<TT_DATA>(sampleIn);
        }

        samplesA = samplesStoreA;
        samplesB = samplesStoreB;

        // Perform FFT
        for (unsigned int rank = 0; rank < kRanks; rank++) {
            posLoMask = (1 << (kRanks - 1 - rank)) - 1;      // e.g. 000111
            posHiMask = (1 << (kRanks - 1)) - 1 - posLoMask; // e.g. 111000
            for (unsigned int op = 0; op < TP_POINT_SIZE / 2; op++) {
                posLo = op & posLoMask;
                posHi = op & posHiMask;
                posA = (posHi << 1) + 0 + posLo;
                posB = (posHi << 1) + (1 << (kRanks - 1 - rank)) + posLo;
                if (posA < TP_POINT_SIZE && posB < TP_POINT_SIZE) {
                    inSampleA = samplesA[posA];
                    inSampleB = samplesA[posB];
                }
                twiddleMask = TP_POINT_SIZE / 2 - 1;
                twiddleIndex = bitRev(kPtSizePwr - 1, op) << (kRanks - rank - 1);
                twiddlePos = twiddleIndex & twiddleMask; // << (kRanks-rank-1);
                twiddle = twiddles[twiddlePos];
                // printf("twiddlePos = %2d, twiddle = (%6d, %6d) ",
                //       twiddlePos,       twiddle.real, twiddle.imag);
                btflynonbitacc<TT_DATA, TT_TWIDDLE>(twiddle, inSampleA, inSampleB, outSampleA, outSampleB);
                if (posA < TP_POINT_SIZE && posB < TP_POINT_SIZE) {
                    samplesB[posA] = outSampleA;
                    samplesB[posB] = outSampleB;
                }
            }
            // Now watch carefully. The pea is under the cup labelled samplesB (the output).
            tempPtr = samplesA;
            samplesA = samplesB;
            samplesB = tempPtr;
            // The pea is now under the cup labelled samplesA. The next rank's input or the actual output
        }
        // Write samples out (natural order)
        for (unsigned int i = 0; i < TP_POINT_SIZE; i++) {
            window_writeincr((output_window<TT_DATA>*)outWindow,
                             castOutput<TT_DATA>(samplesA[bitRev(kPtSizePwr, i)], kScaleFactor));
        }
    }
};
}
}
}
}
}
