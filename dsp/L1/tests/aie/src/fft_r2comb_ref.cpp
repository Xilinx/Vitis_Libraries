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
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include "fft_r2comb_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace r2comb {
/*
  FFT/iFFT DIT r2 combiner  reference model
*/

template <typename T_D>
T_D saturate(int64 inD){};
template <>
int32 saturate<int32>(int64 inD) {
    int32 retval = inD;
    if (inD >= C_PMAX32) {
        retval = C_PMAX32;
    } else if (inD < C_NMAX32) {
        retval = C_NMAX32;
    }
    return retval;
};
template <>
int16 saturate<int16>(int64 inD) {
    int32 retval = inD;
    if (inD >= C_PMAX16) {
        retval = C_PMAX16;
    } else if (inD < C_NMAX16) {
        retval = C_NMAX16;
    }
    return retval;
};

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
          unsigned int TP_PARALLEL_POWER>
void fft_r2comb_ref<TT_DATA,
                    TT_TWIDDLE,
                    TP_POINT_SIZE,
                    TP_FFT_NIFFT,
                    TP_SHIFT,
                    TP_DYN_PT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_PARALLEL_POWER>::r2StageInt(TT_DATA* samplesA,
                                                   TT_DATA* samplesB,
                                                   TT_TWIDDLE* twiddles,
                                                   int pptSize,
                                                   bool inv) {
    int ptSize = TP_DYN_PT_SIZE == 0 ? TP_POINT_SIZE : pptSize;
    int loopSize = (ptSize >> TP_PARALLEL_POWER) >> 1; // each loop calc 2 samples
    constexpr unsigned int kRadix = 2;
    typedef typename T_base_type_struct<TT_DATA>::T_base_type TT_BASE_DATA;
    cint64 sam1, sam2, sam2rot;
    int64 presat;
    unsigned int inIndex[kRadix];
    cint32 tw;                     // necessary to accommodate 0,-32768 when conjugated
    const unsigned int shift = 15; // just to compensate for binary point in cint16 twiddles
    const unsigned int round_const = (1 << (shift + TP_SHIFT - 1));
    for (int op = 0; op < loopSize; op++) {
        inIndex[0] = op;
        inIndex[1] = op + loopSize;
        tw.real = (int32)twiddles[op].real;
        tw.imag = inv ? -(int32)twiddles[op].imag : (int32)twiddles[op].imag;
        sam1.real = (int64)samplesA[inIndex[0]].real << shift;
        sam1.imag = (int64)samplesA[inIndex[0]].imag << shift;
        sam2.real = (int64)samplesA[inIndex[1]].real;
        sam2.imag = (int64)samplesA[inIndex[1]].imag;
        sam2rot.real = (int64)sam2.real * (int64)tw.real - (int64)sam2.imag * (int64)tw.imag;
        sam2rot.imag = (int64)sam2.real * (int64)tw.imag + (int64)sam2.imag * (int64)tw.real;
        presat = ((int64)sam1.real + (int64)sam2rot.real + (int64)round_const) >> (shift + TP_SHIFT);
        samplesB[inIndex[0]].real = saturate<TT_BASE_DATA>(presat);
        presat = ((int64)sam1.imag + (int64)sam2rot.imag + (int64)round_const) >> (shift + TP_SHIFT);
        samplesB[inIndex[0]].imag = saturate<TT_BASE_DATA>(presat);
        presat = ((int64)sam1.real - (int64)sam2rot.real + (int64)round_const) >> (shift + TP_SHIFT);
        samplesB[inIndex[1]].real = saturate<TT_BASE_DATA>(presat);
        presat = ((int64)sam1.imag - (int64)sam2rot.imag + (int64)round_const) >> (shift + TP_SHIFT);
        samplesB[inIndex[1]].imag = saturate<TT_BASE_DATA>(presat);
    }
}

template <typename TT_DATA,
          typename TT_TWIDDLE, // type of coefficients           (e.g. int16, cint32)
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_PARALLEL_POWER>
void fft_r2comb_ref<TT_DATA,
                    TT_TWIDDLE,
                    TP_POINT_SIZE,
                    TP_FFT_NIFFT,
                    TP_SHIFT,
                    TP_DYN_PT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_PARALLEL_POWER>::r2StageFloat(TT_DATA* samplesA,
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
        inIndex[0] = op;
        inIndex[1] = op + loopSize;
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
          unsigned int TP_PARALLEL_POWER>
void fft_r2comb_ref<TT_DATA,
                    TT_TWIDDLE,
                    TP_POINT_SIZE,
                    TP_FFT_NIFFT,
                    TP_SHIFT,
                    TP_DYN_PT_SIZE,
                    TP_WINDOW_VSIZE,
                    TP_PARALLEL_POWER>::fft_r2comb_ref_main(input_window<TT_DATA>* inWindow,
                                                            output_window<TT_DATA>* outWindow) {
    TT_DATA* xbuff = (TT_DATA*)inWindow->ptr;
    TT_DATA* obuff = (TT_DATA*)outWindow->ptr;
    bool inv = TP_FFT_NIFFT == 1 ? false : true; // may be overwritten if dyn_pt_size is set
    if
        constexpr(is_cfloat<TT_DATA>()) { r2StageFloat(xbuff, obuff, twiddles, TP_POINT_SIZE, inv); }
    else {
        r2StageInt(xbuff, obuff, twiddles, TP_POINT_SIZE, inv); // only called for the first stage so stage is implied
    }
};
}
}
}
}
}
