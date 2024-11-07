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
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#include "device_defs.h"
#include "fft_r2comb_ref.hpp"
#include "fir_ref_utils.hpp" //for rounding and saturation

// unitVector cannot be in fft_ref_utils because that is used by 2 different kernels, so leads to multiple definition.
template <typename T_D>
constexpr T_D kunitVector(){};
template <>
cint16 constexpr kunitVector<cint16>() {
    cint16 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
cint32 constexpr kunitVector<cint32>() {
    cint32 temp;
    temp.real = 1;
    temp.imag = 0;
    return temp;
};
template <>
cfloat constexpr kunitVector<cfloat>() {
    cfloat temp;
    temp.real = 1.0;
    temp.imag = 0.0;
    return temp;
};

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {
/*
  FFT/iFFT DIT r2 combiner  reference model
*/

//---------------------------------------------------------
// templatized Radix2 stage
// although this has the same name as a function in the overall FFT class, this one is the last rank whereas the other
// is the first.
template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
void fft_r2comb_ref<TT_DATA,
                    TT_TWIDDLE,
                    TP_POINT_SIZE,
                    TP_FFT_NIFFT,
                    TP_SHIFT,
                    TP_DYN_PT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_PARALLEL_POWER,
                    TP_ORIG_PAR_POWER,
                    TP_RND,
                    TP_SAT,
                    TP_TWIDDLE_MODE>::r2StageInt(TT_DATA* samplesA,
                                                 TT_DATA* samplesB,
                                                 TT_TWIDDLE* twiddles,
                                                 int pptSize,
                                                 bool inv) {
    int ptSize = TP_DYN_PT_SIZE == 0 ? TP_POINT_SIZE : pptSize;
    int loopSize = (ptSize >> TP_PARALLEL_POWER) >> 1; // each loop calc 2 samples
    T_accRef<TT_DATA> csum;

    constexpr unsigned int kRadix = 2;
    typedef typename T_base_type_struct<TT_DATA>::T_base_type TT_BASE_DATA;
    cint64 sam1, sam2, sam2rot;
    int64 presat;
    unsigned int inIndex[kRadix];
    unsigned int outIndex[kRadix];

    int64 twreal;
    int64 twimag;
    constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, TP_TWIDDLE_MODE>();
    const unsigned int round_const = (1 << (kTwShift + TP_SHIFT - 1));
    for (int op = 0; op < loopSize; op++) {
        inIndex[0] = 2 * op;
        inIndex[1] = 2 * op + 1;
        outIndex[0] = op;
        outIndex[1] = op + loopSize;
        twreal = (int64)twiddles[op].real;
        twimag = inv ? -(int64)twiddles[op].imag : (int64)twiddles[op].imag;
        sam1.real = (int64)samplesA[inIndex[0]].real << kTwShift;
        sam1.imag = (int64)samplesA[inIndex[0]].imag << kTwShift;
        sam2.real = (int64)samplesA[inIndex[1]].real;
        sam2.imag = (int64)samplesA[inIndex[1]].imag;
        sam2rot.real = (int64)sam2.real * (int64)twreal - (int64)sam2.imag * (int64)twimag;
        sam2rot.imag = (int64)sam2.real * (int64)twimag + (int64)sam2.imag * (int64)twreal;
        csum.real = (int64)sam1.real + (int64)sam2rot.real;
        csum.imag = (int64)sam1.imag + (int64)sam2rot.imag;
        roundAcc(TP_RND, kTwShift + TP_SHIFT, csum);
        saturateAcc(csum, TP_SAT);
        samplesB[outIndex[0]].real = (TT_BASE_DATA)csum.real;
        samplesB[outIndex[0]].imag = (TT_BASE_DATA)csum.imag;

        csum.real = (int64)sam1.real - (int64)sam2rot.real;
        csum.imag = (int64)sam1.imag - (int64)sam2rot.imag;
        roundAcc(TP_RND, kTwShift + TP_SHIFT, csum);
        saturateAcc(csum, TP_SAT);
        samplesB[outIndex[1]].real = (TT_BASE_DATA)csum.real;
        samplesB[outIndex[1]].imag = (TT_BASE_DATA)csum.imag;
    }
}

template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
void fft_r2comb_ref<TT_DATA,
                    TT_TWIDDLE,
                    TP_POINT_SIZE,
                    TP_FFT_NIFFT,
                    TP_SHIFT,
                    TP_DYN_PT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_PARALLEL_POWER,
                    TP_ORIG_PAR_POWER,
                    TP_RND,
                    TP_SAT,
                    TP_TWIDDLE_MODE>::r2StageFloat(TT_DATA* samplesA,
                                                   TT_DATA* samplesB,
                                                   TT_TWIDDLE* twiddles,
                                                   int pptSize,
                                                   bool inv) {
    unsigned int rank = fnGetPointSizePower<TP_POINT_SIZE>() - 1;
    constexpr unsigned int kRadix = 2;
    int ptSize = TP_DYN_PT_SIZE == 0 ? TP_POINT_SIZE : pptSize;
    int loopSize = (ptSize >> TP_PARALLEL_POWER) >> 1; // each loop calc 2 samples
    cfloat sam1, sam2, sam2rot;
    unsigned int inIndex[kRadix];
    unsigned int outIndex[kRadix];
    cfloat tw;
    unsigned int temp1, temp2, twIndex;
    for (int op = 0; op < loopSize;
         op++) { // Note that the order if inputs differs from UUT. Ref is Cooley-Tukey, UUT is Stockham
        inIndex[0] = 2 * op;
        inIndex[1] = 2 * op + 1;
        outIndex[0] = op;
        outIndex[1] = op + loopSize;
        tw.real = twiddles[op].real;
        tw.imag = inv ? -twiddles[op].imag : twiddles[op].imag;
        sam1.real = samplesA[inIndex[0]].real;
        sam1.imag = samplesA[inIndex[0]].imag;
        sam2.real = samplesA[inIndex[1]].real;
        sam2.imag = samplesA[inIndex[1]].imag;
        samplesB[outIndex[0]].real = (sam1.real + (-sam2.imag * tw.imag + sam2.real * tw.real));
        samplesB[outIndex[0]].imag = (sam1.imag + (+sam2.imag * tw.real + sam2.real * tw.imag));
        samplesB[outIndex[1]].real = (sam1.real + (+sam2.imag * tw.imag - sam2.real * tw.real));
        samplesB[outIndex[1]].imag = (sam1.imag + (-sam2.imag * tw.real - sam2.real * tw.imag));
    }
}

// Bit-accurate REF FFT DIT R2 stage function
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER,
          unsigned int TP_ORIG_PAR_POWER,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_TWIDDLE_MODE>
void fft_r2comb_ref<TT_DATA,
                    TT_TWIDDLE,
                    TP_POINT_SIZE,
                    TP_FFT_NIFFT,
                    TP_SHIFT,
                    TP_DYN_PT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_PARALLEL_POWER,
                    TP_ORIG_PAR_POWER,
                    TP_RND,
                    TP_SAT,
                    TP_TWIDDLE_MODE>::fft_r2comb_ref_main(input_buffer<TT_DATA>& inWindow,
                                                          output_buffer<TT_DATA>& outWindow) {
    if
        constexpr(TP_DYN_PT_SIZE == 1) {
            constexpr int kMinPtSizePwr = 4;
            constexpr int kMaxPtSizePwr = 16;
            constexpr unsigned int kHeaderSize = 32 / (sizeof(TT_DATA)); // dynamic point size header size in samples
            constexpr unsigned int kPtSizePwr = fnGetPointSizePower<TP_POINT_SIZE>();
            constexpr unsigned int kR2Stages =
                std::is_same<TT_DATA, cfloat>::value
                    ? kPtSizePwr
                    : (kPtSizePwr % 2 == 1 ? 1 : 0); // There is one radix 2 stage if we have an odd power of 2 point
                                                     // size, but for cfloat all stages are R2.
            constexpr unsigned int kR4Stages = std::is_same<TT_DATA, cfloat>::value ? 0 : kPtSizePwr / 2;
            constexpr unsigned int kTwShift = getTwShift<TT_TWIDDLE, TP_TWIDDLE_MODE>();
            unsigned int stageShift = 0;

            TT_DATA* headerPtr;
            TT_DATA headerVal; // a single sample from the header. Each component of the complex sample is a field
            int16 ptSizePwr =
                kPtSizePwr; // default to static point size value. May be overwritten if dynamic point size selected.
            TT_DATA dummyttdata; // used to consume blank data in header.
            unsigned int ptSize =
                TP_POINT_SIZE; // default to static point size value. May be overwritten if dynamic point size selected.
            bool inv = TP_FFT_NIFFT == 1 ? false : true; // may be overwritten if dyn_pt_size is set
            TT_DATA headerOut;
            TT_DATA* xbuff = (TT_DATA*)inWindow.data();
            TT_DATA* obuff = (TT_DATA*)outWindow.data();

            headerVal = *xbuff++; // direction field
            *obuff++ = headerVal;
            inv = headerVal.real == 0 ? true : false;
            headerVal = *xbuff++; // radix2 field
            *obuff++ = headerVal;
            ptSizePwr = (int32)headerVal.real - (TP_ORIG_PAR_POWER - TP_PARALLEL_POWER);
            for (int i = 2; i < kHeaderSize - 1; i++) {
                headerVal = *xbuff++; // reserved  fields
                *obuff++ = headerVal;
            }
            headerVal = *xbuff++; // status field

            if (ptSizePwr >= kMinPtSizePwr && ptSizePwr <= kMaxPtSizePwr && (int32)headerVal.real == 0) {
                *obuff++ = headerVal; // legal header, so copy to output

                // override values set for constant point size with values derived from the header in a dynamic point
                // size frame
                ptSize = ((unsigned int)1) << ptSizePwr;

                int tw_base = twiddle_tables[kPtSizePwr - ptSizePwr];
                int n = ptSize;

                // first, blank the whole output window (bar the header which has already been output)
                for (int i = 0; i < TP_WINDOW_VSIZE; i++) {
                    obuff[i] = nullElem<TT_DATA>();
                }

                for (int frameStart = 0; frameStart < TP_WINDOW_VSIZE;
                     frameStart += (TP_POINT_SIZE >> TP_PARALLEL_POWER)) {
                    if
                        constexpr(is_cfloat<TT_DATA>()) {
                            r2StageFloat(xbuff + frameStart, obuff + frameStart, &twiddles[tw_base], n, inv);
                        }
                    else {
                        r2StageInt(xbuff + frameStart, obuff + frameStart, &twiddles[tw_base], n,
                                   inv); // only called for the first stage so stage is implied
                    }
                }
            } else {                               // illegal dynamic point size
                *obuff++ = kunitVector<TT_DATA>(); // set status field to 1 = invalid point size
                // blank the whole output window (bar the header which has already been output)
                for (int i = 0; i < TP_WINDOW_VSIZE; i++) {
                    obuff[i] = nullElem<TT_DATA>();
                }
            }
        }
    else { // static frame size
        constexpr int n =
            (TP_POINT_SIZE >>
             TP_PARALLEL_POWER); // actually a dummy. The stage functions below use TP_POINT_SIZE directly.
        TT_DATA* xbuff = (TT_DATA*)inWindow.data();
        TT_DATA* obuff = (TT_DATA*)outWindow.data();
        constexpr bool inv = TP_FFT_NIFFT == 1 ? false : true; // may be overwritten if dyn_pt_size is set
        for (int frameStart = 0; frameStart < TP_WINDOW_VSIZE; frameStart += n) {
            if
                constexpr(is_cfloat<TT_DATA>()) {
                    r2StageFloat(xbuff + frameStart, obuff + frameStart, twiddles, n, inv);
                }
            else {
                r2StageInt(xbuff + frameStart, obuff + frameStart, twiddles, n,
                           inv); // only called for the first stage so stage is implied
            }
        }
    }
};
}
}
}
}
}
