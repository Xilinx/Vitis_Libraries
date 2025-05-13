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
#ifndef _DSPLIB_FFT_REF_UTILS_HPP_
#define _DSPLIB_FFT_REF_UTILS_HPP_

//#include "fft_ifft_dit_twiddle_lut.h"  //for legacy cint16 tables from commslib
#include "fft_ifft_dit_twiddle_lut_all.h" //for cint32, cint31, cint15 tables.
//#include "fft_ifft_dit_twiddle_lut_cint32.h" //holds cint15 and cint31 tables
#include "fir_ref_utils.hpp" //for rounding and saturation functions

// Rounding modes
#ifndef rnd_floor
#define rnd_floor 0
#endif
#ifndef rnd_ceil
#define rnd_ceil 1
#endif
#ifndef rnd_pos_inf
#define rnd_pos_inf 2
#endif
#ifndef rnd_neg_inf
#define rnd_neg_inf 3
#endif
#ifndef rnd_sym_inf
#define rnd_sym_inf 4
#endif
#ifndef rnd_sym_zero
#define rnd_sym_zero 5
#endif
#ifndef rnd_conv_even
#define rnd_conv_even 6
#endif
#ifndef rnd_conv_odd
#define rnd_conv_odd 7
#endif

// Round up to y
//#define CEIL(x,y) (((x+y-1)/y)*y)

// namespace xf { namespace dsp { namespace aie { namespace fft {
/*
Reference model utilities.
This file contains sets of overloaded, templatized and specialized templatized
functions for use by the reference model classes.
*/

static constexpr unsigned int kMaxPointSize = 4096;

//---------------------------------
// Templatized types

template <typename T_D>
struct T_int_data {};
template <>
struct T_int_data<int16> {
    int32 real;
    int32 imag;
};
template <>
struct T_int_data<cint16> {
    int32 real;
    int32 imag;
};
template <>
struct T_int_data<int32> {
    int32 real;
    int32 imag;
};
template <>
struct T_int_data<cint32> {
    int32 real;
    int32 imag;
};
template <>
struct T_int_data<float> {
    float real;
    float imag;
};
template <>
struct T_int_data<cfloat> {
    float real;
    float imag;
};

template <typename T_D>
struct T_accfftRef {};
template <>
struct T_accfftRef<int16> {
    int32 real;
    int32 imag;
};
template <>
struct T_accfftRef<cint16> {
    int32 real;
    int32 imag;
};
template <>
struct T_accfftRef<int32> {
    int32 real;
    int32 imag;
};
template <>
struct T_accfftRef<cint32> {
    int32 real;
    int32 imag;
};
template <>
struct T_accfftRef<float> {
    float real;
    float imag;
};
template <>
struct T_accfftRef<cfloat> {
    float real;
    float imag;
};

//---------------------------------
// Templatized functions
template <typename TT_TWIDDLE, unsigned int T_TW_MODE = 0>
constexpr unsigned int getTwShift() {
    printf("Error: unexpected twiddle type\n");
    return 0;
}; // default error trap
template <>
constexpr unsigned int getTwShift<cint16, 0>() {
    return 15;
};
template <>
constexpr unsigned int getTwShift<cint16, 1>() {
    return 14;
};
template <>
constexpr unsigned int getTwShift<cint32, 0>() {
    return 31;
};
template <>
constexpr unsigned int getTwShift<cint32, 1>() {
    return 30;
};
template <>
constexpr unsigned int getTwShift<cfloat, 0>() {
    return 0;
};
template <>
constexpr unsigned int getTwShift<cfloat, 1>() {
    return 0;
};

// Fn to perform log2 on TP_POINT_SIZE to get number of Radix2 ranks
template <unsigned int TP_POINT_SIZE>
inline constexpr unsigned int fnGetPointSizePower() {
    return TP_POINT_SIZE == 16
               ? 4
               : TP_POINT_SIZE == 32
                     ? 5
                     : TP_POINT_SIZE == 64
                           ? 6
                           : TP_POINT_SIZE == 128
                                 ? 7
                                 : TP_POINT_SIZE == 256
                                       ? 8
                                       : TP_POINT_SIZE == 512
                                             ? 9
                                             : TP_POINT_SIZE == 1024
                                                   ? 10
                                                   : TP_POINT_SIZE == 2048
                                                         ? 11
                                                         : TP_POINT_SIZE == 4096
                                                               ? 12
                                                               : TP_POINT_SIZE == 8192
                                                                     ? 13
                                                                     : TP_POINT_SIZE == 16384
                                                                           ? 14
                                                                           : TP_POINT_SIZE == 32768
                                                                                 ? 15
                                                                                 : TP_POINT_SIZE == 65536 ? 16 : 0;
}

template <typename TT_TWIDDLE, unsigned int TP_TWIDDLE_MODE>
inline TT_TWIDDLE* fnGetTwiddleMasterBase(){};
template <>
inline cint16* fnGetTwiddleMasterBase<cint16, 0>() {
    return (cint16*)twiddle_master_cint16;
};
template <>
inline cint32* fnGetTwiddleMasterBase<cint32, 0>() {
    return (cint32*)twiddle_master_cint32;
};
template <>
inline cfloat* fnGetTwiddleMasterBase<cfloat, 0>() {
    return (cfloat*)twiddle_master_cfloat;
};
template <>
inline cint16* fnGetTwiddleMasterBase<cint16, 1>() {
    return (cint16*)twiddle_master_cint15;
};
template <>
inline cint32* fnGetTwiddleMasterBase<cint32, 1>() {
    return (cint32*)twiddle_master_cint31;
};
template <>
inline cfloat* fnGetTwiddleMasterBase<cfloat, 1>() {
    return (cfloat*)twiddle_master_cfloat;
};

// function to query type -used to avoid template specializing the whole class.
template <typename T_D>
inline constexpr bool is_cfloat() {
    return false;
};
template <>
inline constexpr bool is_cfloat<cfloat>() {
    return true;
};

//-----------------------------------------------------------------------------------------------------
// Utility functions for FFT/iFFT single channel reference model

// Function has to be fully specialized, which is excessive, so integer template parameters are used simply as
// parameters (function arguments) to reduce duplication
template <typename TT_TWIDDLE>
inline TT_TWIDDLE get_twiddle(int i, unsigned int TP_POINT_SIZE, unsigned int TP_FFT_NIFFT) {
    return 0; // never used
}
template <>
inline cint16 get_twiddle<cint16>(int i, unsigned int TP_POINT_SIZE, unsigned int TP_FFT_NIFFT) {
    int step = kMaxPointSize / TP_POINT_SIZE;
    cint16 raw_twiddle = twiddle_master_cint16[i * step];
    if (TP_FFT_NIFFT == 0) {
        raw_twiddle.imag = -raw_twiddle.imag;
    }
    return raw_twiddle;
}
template <>
inline cint32 get_twiddle<cint32>(int i, unsigned int TP_POINT_SIZE, unsigned int TP_FFT_NIFFT) {
    int step = kMaxPointSize / TP_POINT_SIZE;
    cint32 raw_twiddle = twiddle_master_cint32[i * step];
    if (TP_FFT_NIFFT == 0) {
        raw_twiddle.imag = -raw_twiddle.imag;
    }
    return raw_twiddle;
}
template <>
inline cfloat get_twiddle<cfloat>(int i, unsigned int TP_POINT_SIZE, unsigned int TP_FFT_NIFFT) {
    int step = kMaxPointSize / TP_POINT_SIZE;
    cfloat raw_twiddle = twiddle_master_cfloat[i * step];
    if (TP_FFT_NIFFT == 0) {
        raw_twiddle.imag = -raw_twiddle.imag;
    }
    return raw_twiddle;
}

//--------castInput
// converts the input type to the type used for internal data processing
template <typename TT_DATA>
inline T_int_data<TT_DATA> castInput(TT_DATA sampleIn) {
    T_int_data<TT_DATA> retVal; // default to mute warnings
    return retVal;
}
template <>
inline T_int_data<int16> castInput<int16>(int16 sampleIn) {
    T_int_data<int16> retVal;
    retVal.real = sampleIn;
    retVal.imag = 0;
    return retVal;
}
template <>
inline T_int_data<cint16> castInput<cint16>(cint16 sampleIn) {
    T_int_data<cint16> retVal;
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    return retVal;
}
template <>
inline T_int_data<int32> castInput<int32>(int32 sampleIn) {
    T_int_data<int32> retVal;
    retVal.real = sampleIn;
    retVal.imag = 0;
    return retVal;
}
template <>
inline T_int_data<cint32> castInput<cint32>(cint32 sampleIn) {
    T_int_data<cint32> retVal;
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    return retVal;
}
template <>
inline T_int_data<float> castInput<float>(float sampleIn) {
    T_int_data<float> retVal;
    retVal.real = sampleIn;
    retVal.imag = 0;
    return retVal;
}
template <>
inline T_int_data<cfloat> castInput<cfloat>(cfloat sampleIn) {
    T_int_data<cfloat> retVal;
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    return retVal;
}

// Derivation of base type of a complex type.
template <typename TT_TWIDDLE = cint16>
struct T_base_type_struct {
    using T_base_type = int16;
};
template <>
struct T_base_type_struct<cint32> {
    using T_base_type = int32;
};
template <>
struct T_base_type_struct<cfloat> {
    using T_base_type = float;
};

// fnMaxPos Maximum positive value of a twiddle component. This is arcane because twiddle is a complex type, but the
// return is real.
template <typename TT_TWIDDLE = cint16>
inline typename T_base_type_struct<TT_TWIDDLE>::T_base_type fnMaxPos() {
    return C_PMAX16;
}
template <>
inline T_base_type_struct<cint32>::T_base_type fnMaxPos<cint32>() {
    return C_PMAX32;
}
template <>
inline T_base_type_struct<cfloat>::T_base_type fnMaxPos<cfloat>() {
    return 3.4028234664e+32;
}

// fnMaxNeg Maximum negative value of a twiddle component. This is arcane because twiddle is a complex type, but the
// return is real.
template <typename TT_TWIDDLE = cint16>
inline typename T_base_type_struct<TT_TWIDDLE>::T_base_type fnMaxNeg() {
    return C_NMAX16;
}
template <>
inline T_base_type_struct<cint32>::T_base_type fnMaxNeg<cint32>() {
    return C_NMAX32;
}
template <>
inline T_base_type_struct<cfloat>::T_base_type fnMaxNeg<cfloat>() {
    return -3.4028234664e+32;
}

//--------castOutput
// converts the input type to the type used for internal data processing
// TODO - castOutput used to perform saturation. This is now performed in saturateAcc, so castOutput is redundant now
// and is literally an implicit cast so could be removed
template <typename TT_DATA>
inline TT_DATA castOutput(T_int_data<TT_DATA> sampleIn, const unsigned shift = 0) {
    TT_DATA retVal; // default to mute warnings
    return retVal;
}
template <>
inline int16 castOutput<int16>(T_int_data<int16> sampleIn, const unsigned shift) {
    int16 retVal;
    // rounding is performed in the radix stages
    retVal = sampleIn.real;
    return retVal;
}
template <>
inline cint16 castOutput<cint16>(T_int_data<cint16> sampleIn, const unsigned shift) {
    cint16 retVal;
    // rounding is performed in the radix stages
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    return retVal;
}
template <>
inline int32 castOutput<int32>(T_int_data<int32> sampleIn, const unsigned shift) {
    int32 retVal;
    // rounding is performed in the radix stages
    retVal = sampleIn.real;
    return retVal;
}
template <>
inline cint32 castOutput<cint32>(T_int_data<cint32> sampleIn, const unsigned shift) {
    cint32 retVal;
    // rounding is performed in the radix stages
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    return retVal;
}
template <>
inline float castOutput<float>(T_int_data<float> sampleIn, const unsigned shift) {
    float retVal;
    retVal = sampleIn.real;
    return retVal;
}
template <>
inline cfloat castOutput<cfloat>(T_int_data<cfloat> sampleIn, const unsigned shift) {
    cfloat retVal;
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    return retVal;
}

//----------------log2 point size
template <unsigned int TP_POINT_SIZE>
inline constexpr unsigned int getPointSizePower() {
    switch (TP_POINT_SIZE) {
        case 16:
            return 4;
            break;
        case 32:
            return 5;
            break;
        case 64:
            return 6;
            break;
        case 128:
            return 7;
            break;
        case 256:
            return 8;
            break;
        case 512:
            return 9;
            break;
        case 1024:
            return 10;
            break;
        case 2048:
            return 11;
            break;
        case 4096:
            return 12;
            break;
        case 8192:
            return 13;
            break;
        case 16384:
            return 14;
            break;
        case 32768:
            return 15;
            break;
        case 65536:
            return 16;
            break;
        default:
            printf("Error in pointSizePower\n");
            return 0;
            break;
    }
}

// bitrev does a bit reverse of index val for len bits wide.
inline unsigned int bitRev(unsigned int len, unsigned int val) {
    unsigned int retVal = 0;
    unsigned int ip = val;
    for (int i = 0; i < len; i++) {
        retVal = retVal << 1;
        if (ip % 2 == 1) {
            retVal++;
        }
        ip >>= 1;
    }
    return retVal;
}

inline void fftScale(int rndMode, int shift, T_accRef<int16>& accum) {
    // cint64_t ret;
    accum.real = rounding(rndMode, shift, accum.real);
    // return ret;
};

inline void fftScale(int rndMode, int shift, T_accRef<int32>& accum) {
    // cint64_t ret;
    accum.real = rounding(rndMode, shift, accum.real);
    // return ret;
};

inline void fftScale(int rndMode, int shift, T_accRef<cint32>& accum) {
    // cint64_t ret;
    accum.real = rounding(rndMode, shift, accum.real);
    accum.imag = rounding(rndMode, shift, accum.imag);
    // return ret;
};
inline void fftScale(int rndMode, int shift, T_accRef<cint16>& accum) {
    // cint64_t ret;
    accum.real = rounding(rndMode, shift, accum.real);
    accum.imag = rounding(rndMode, shift, accum.imag);
    // return ret;
};

// Rounding and shift - do apply to float in the FFT
inline void fftScale(int rndMode, int shift, T_accRef<float>& accum) {
    accum.real = (accum.real + (float)(1 << (shift - 1))) / (float)(1 << (shift));
};
inline void fftScale(int rndMode, int shift, T_accRef<cfloat>& accum) {
    accum.real = (accum.real + (float)(1 << (shift - 1))) / (float)(1 << (shift));
    accum.imag = (accum.imag + (float)(1 << (shift - 1))) / (float)(1 << (shift));
};

//------------------------------------------------------
// Templatized complex multiply, returning uniform cint64.
// inv is actually compile-time constant, but passed here as a run-time because it reduces code verbosity.
template <typename T_D, typename T_TW>
inline cint64 cmpy(T_D d, T_TW tw, bool inv) {
    return {0, 0};
};
template <>
inline cint64 cmpy<cint16, cint16>(cint16 d, cint16 tw, bool inv) {
    cint64 retVal;
    if (inv == true) {
        retVal.real = (int64)d.real * (int64)tw.real + (int64)d.imag * (int64)tw.imag;
        retVal.imag = (int64)d.imag * (int64)tw.real - (int64)d.real * (int64)tw.imag;
    } else {
        retVal.real = (int64)d.real * (int64)tw.real - (int64)d.imag * (int64)tw.imag;
        retVal.imag = (int64)d.imag * (int64)tw.real + (int64)d.real * (int64)tw.imag;
    }
    return retVal;
}
template <>
inline cint64 cmpy<cint32, cint16>(cint32 d, cint16 tw, bool inv) {
    cint64 retVal;
    if (inv == true) {
        retVal.real = (int64)d.real * (int64)tw.real + (int64)d.imag * (int64)tw.imag;
        retVal.imag = (int64)d.imag * (int64)tw.real - (int64)d.real * (int64)tw.imag;
    } else {
        retVal.real = (int64)d.real * (int64)tw.real - (int64)d.imag * (int64)tw.imag;
        retVal.imag = (int64)d.imag * (int64)tw.real + (int64)d.real * (int64)tw.imag;
    }
    return retVal;
}
// It is acknowledged that cint64 is insufficent for the return of cint32*cint32, but
// no larger type exists and errors only occur if input data has not been pre-scaled.
template <>
inline cint64 cmpy<cint32, cint32>(cint32 d, cint32 tw, bool inv) {
    cint64 retVal;
    if (inv == true) {
        retVal.real = (int64)d.real * (int64)tw.real + (int64)d.imag * (int64)tw.imag;
        retVal.imag = (int64)d.imag * (int64)tw.real - (int64)d.real * (int64)tw.imag;
    } else {
        retVal.real = (int64)d.real * (int64)tw.real - (int64)d.imag * (int64)tw.imag;
        retVal.imag = (int64)d.imag * (int64)tw.real + (int64)d.real * (int64)tw.imag;
    }
    return retVal;
}

//--------butterfly for non-bit-accurate model
template <typename TT_DATA, typename TT_TWIDDLE>
inline void btflynonbitacc(TT_TWIDDLE twiddle,
                           T_int_data<TT_DATA> A,
                           T_int_data<TT_DATA> B,
                           T_int_data<TT_DATA>& outA,
                           T_int_data<TT_DATA>& outB) {
    return;
}
template <>
inline void btflynonbitacc<cint16, cint16>(
    cint16 twiddle, T_int_data<cint16> A, T_int_data<cint16> B, T_int_data<cint16>& outA, T_int_data<cint16>& outB) {
    T_int_data<cint16> rotB, Ao, Bo;
    rotB.real = (int32)(((int64)twiddle.real * B.real - (int64)twiddle.imag * B.imag + 16384) >> 15);
    rotB.imag = (int32)(((int64)twiddle.real * B.imag + (int64)twiddle.imag * B.real + 16384) >> 15);

    Ao.real = A.real + rotB.real;
    Ao.imag = A.imag + rotB.imag;
    Bo.real = A.real - rotB.real;
    Bo.imag = A.imag - rotB.imag;
    outA = Ao;
    outB = Bo;
}

//-----------------------------------------------------------------
// templatized butterfly function.
// Twshift is passed as an argument rather than TP_TWIDDLE_MODE being passed and then
// twshift being derived. This is simply because with an extra parameter on a templatized
// function is much more verbose.
template <typename TI_D, typename T_TW>
inline void btfly(T_int_data<TI_D>& q0,
                  T_int_data<TI_D>& q1,
                  T_int_data<TI_D> d0,
                  T_int_data<TI_D> d1,
                  T_TW tw,
                  bool inv,
                  unsigned int twShift,
                  unsigned int shift,
                  unsigned int t_rnd,
                  unsigned int t_sat) {
    printf("wrong\n");
}; // assumes downshift by weight of twiddle type, e.g. 15 for cint16

template <>
inline void btfly<cfloat, cfloat>(T_int_data<cfloat>& q0,
                                  T_int_data<cfloat>& q1,
                                  T_int_data<cfloat> d0,
                                  T_int_data<cfloat> d1,
                                  cfloat tw,
                                  bool inv,
                                  unsigned int twShift,
                                  unsigned int shift,
                                  unsigned int t_rnd,
                                  unsigned int t_sat) {
    cfloat d1up;
    cfloat d1rot;
    d1rot.real = d0.real * tw.real - d0.imag * tw.imag;
    d1rot.imag = d0.imag * tw.real + d0.real * tw.imag;
    q0.real = d0.real + d1rot.real;
    q0.imag = d0.imag + d1rot.imag;
    q1.real = d0.real - d1rot.real;
    q1.imag = d0.imag - d1rot.imag;
};

template <>
inline void btfly<cint32, cint16>(T_int_data<cint32>& q0,
                                  T_int_data<cint32>& q1,
                                  T_int_data<cint32> d0,
                                  T_int_data<cint32> d1,
                                  cint16 tw,
                                  bool inv,
                                  unsigned int twShift,
                                  unsigned int shift,
                                  unsigned int t_rnd,
                                  unsigned int t_sat) {
    cint32 d1up;
    cint32 d1rot;
    cint64 d1rot64;
    T_accRef<cint32> csum;
    const int64 kRoundConst = ((int64)1 << (shift - 1));
    d1up.real = (int32)d1.real;
    d1up.imag = (int32)d1.imag;
    d1rot64 = cmpy<cint32, cint16>(d1up, tw, inv);

    csum.real = ((int64)d0.real << twShift) + d1rot64.real;
    csum.imag = ((int64)d0.imag << twShift) + d1rot64.imag;
    roundAcc(t_rnd, shift, csum);
    saturateAcc(csum, t_sat);
    q0.real = (int32)csum.real;
    q0.imag = (int32)csum.imag;

    csum.real = ((int64)d0.real << twShift) - d1rot64.real;
    csum.imag = ((int64)d0.imag << twShift) - d1rot64.imag;
    roundAcc(t_rnd, shift, csum);
    saturateAcc(csum, t_sat);
    q1.real = (int32)csum.real;
    q1.imag = (int32)csum.imag;
};

template <>
inline void btfly<cint16, cint16>(T_int_data<cint16>& q0,
                                  T_int_data<cint16>& q1,
                                  T_int_data<cint16> d0,
                                  T_int_data<cint16> d1,
                                  cint16 tw,
                                  bool inv,
                                  unsigned int twShift,
                                  unsigned int shift,
                                  unsigned int t_rnd,
                                  unsigned int t_sat) // the upshift variant
{
    cint32 d0up;
    cint32 d1up;
    cint64 d1rot64;
    cint32 d1rot;
    T_accRef<cint32> csum; // internal data type is cint32.
    const int64 kRoundConst = ((int64)1 << (shift - 1));
    d0up.real = (int32)d0.real;
    d0up.imag = (int32)d0.imag;
    d1up.real = (int32)d1.real;
    d1up.imag = (int32)d1.imag;
    d1rot64 = cmpy<cint32, cint16>(d1up, tw, inv);

    csum.real = ((int64)d0up.real << twShift) + d1rot64.real;
    csum.imag = ((int64)d0up.imag << twShift) + d1rot64.imag;
    roundAcc(t_rnd, shift, csum);
    saturateAcc(csum, t_sat);
    q0.real = (int32)csum.real;
    q0.imag = (int32)csum.imag;

    csum.real = ((int64)d0up.real << twShift) - d1rot64.real;
    csum.imag = ((int64)d0up.imag << twShift) - d1rot64.imag;
    roundAcc(t_rnd, shift, csum);
    saturateAcc(csum, t_sat);
    q1.real = (int32)csum.real;
    q1.imag = (int32)csum.imag;
};

template <>
inline void btfly<cint32, cint32>(T_int_data<cint32>& q0,
                                  T_int_data<cint32>& q1,
                                  T_int_data<cint32> d0,
                                  T_int_data<cint32> d1,
                                  cint32 tw,
                                  bool inv,
                                  unsigned int twShift,
                                  unsigned int shift,
                                  unsigned int t_rnd,
                                  unsigned int t_sat) {
    cint32 d1up;
    cint32 d1rot;
    cint64 d1rot64;
    T_accRef<cint32> csum;
    const int64 kRoundConst = ((int64)1 << (shift - 1));
    d1up.real = (int32)d1.real;
    d1up.imag = (int32)d1.imag;
    d1rot64 = cmpy<cint32, cint32>(d1up, tw, inv);

    csum.real = ((int64)d0.real << twShift) + d1rot64.real;
    csum.imag = ((int64)d0.imag << twShift) + d1rot64.imag;
    roundAcc(t_rnd, shift, csum);
    saturateAcc(csum, t_sat);
    q0.real = (int32)csum.real;
    q0.imag = (int32)csum.imag;

    csum.real = ((int64)d0.real << twShift) - d1rot64.real;
    csum.imag = ((int64)d0.imag << twShift) - d1rot64.imag;
    roundAcc(t_rnd, shift, csum);
    saturateAcc(csum, t_sat);
    q1.real = (int32)csum.real;
    q1.imag = (int32)csum.imag;
};

template <>
inline void btfly<cint16, cint32>(T_int_data<cint16>& q0,
                                  T_int_data<cint16>& q1,
                                  T_int_data<cint16> d0,
                                  T_int_data<cint16> d1,
                                  cint32 tw,
                                  bool inv,
                                  unsigned int twShift,
                                  unsigned int shift,
                                  unsigned int t_rnd,
                                  unsigned int t_sat) // the upshift variant
{
    cint32 d0up;
    cint32 d1up;
    cint64 d1rot64;
    cint32 d1rot;
    T_accRef<cint32> csum; // internal data type is cint32.
    const int64 kRoundConst = ((int64)1 << (shift - 1));
    d0up.real = (int32)d0.real;
    d0up.imag = (int32)d0.imag;
    d1up.real = (int32)d1.real;
    d1up.imag = (int32)d1.imag;
    d1rot64 = cmpy<cint32, cint32>(d1up, tw, inv);

    csum.real = ((int64)d0up.real << twShift) + d1rot64.real;
    csum.imag = ((int64)d0up.imag << twShift) + d1rot64.imag;
    roundAcc(t_rnd, shift, csum);
    saturateAcc(csum, t_sat);
    q0.real = (int32)csum.real;
    q0.imag = (int32)csum.imag;

    csum.real = ((int64)d0up.real << twShift) - d1rot64.real;
    csum.imag = ((int64)d0up.imag << twShift) - d1rot64.imag;
    roundAcc(t_rnd, shift, csum);
    saturateAcc(csum, t_sat);
    q1.real = (int32)csum.real;
    q1.imag = (int32)csum.imag;
};

//}}}} //namespace

#endif // ifdef _DSPLIB_FFT_REF_UTILS_HPP_
