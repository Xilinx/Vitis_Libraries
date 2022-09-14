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
#ifndef _DSPLIB_FFT_REF_UTILS_HPP_
#define _DSPLIB_FFT_REF_UTILS_HPP_

#include "fft_ifft_dit_twiddle_lut.h"

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
#define CEIL(x, y) (((x + y - 1) / y) * y)

static const int C_PMAX16 = std::numeric_limits<short int>::max();
static const int C_NMAX16 = std::numeric_limits<short int>::min();
static const int C_PMAX32 = std::numeric_limits<int32_t>::max();
static const int C_NMAX32 = std::numeric_limits<int32_t>::min();

// Accumulator types
template <typename T_D>
struct T_accRef {};
template <>
struct T_accRef<int16> {
    int64_t real;
};
template <>
struct T_accRef<int32> {
    int64_t real;
};
template <>
struct T_accRef<cint16> {
    int64_t real, imag;
};
template <>
struct T_accRef<cint32> {
    int64_t real, imag;
};
template <>
struct T_accRef<float> {
    float real;
};
template <>
struct T_accRef<cfloat> {
    float real, imag;
};

// Rounding and shift function
inline int64_t rounding(int rndMode, int shift, int64_t accum) {
    const unsigned int round_const = shift == 0 ? 0 : (1 << (shift - 1)) - 1; // 0.0111...
    // Rounding
    if (shift > 0) {
        switch (rndMode) {
            case rnd_floor:
                // not addition for truncation, just lose the bits during shift.
                break;
            case rnd_ceil: // add 0.11111...
                accum += ((1 << shift) - 1);
                break;
            case rnd_pos_inf: // add 0.1000
                accum += round_const + 1;
                break;
            case rnd_neg_inf: // add 0.01111...
                accum += round_const;
                break;
            case rnd_sym_inf:
                if (accum < 0) {
                    accum += round_const;
                } else {
                    accum += round_const + 1;
                }
                break;
            case rnd_sym_zero:
                if (accum < 0) {
                    accum += round_const + 1;
                } else {
                    accum += round_const;
                }
                break;
            case rnd_conv_even:
                if (((accum >> shift) & 1) == 0) { // even
                    accum += round_const;
                } else {
                    accum += round_const + 1;
                }
                break;
            case rnd_conv_odd:
                if (((accum >> shift) & 1) == 1) { // odd
                    accum += round_const;
                } else {
                    accum += round_const + 1;
                }
                break;
            default:;
                // unsupported, so error
                printf("Error: unrecognised value for ROUND_MODE\n");
                break;
        }
    }
    // intrinsic supports range of -1 to +62
    if (shift == -1) {
        accum = accum << 1;
    } else {
        accum = accum >> shift;
    }
    return accum;
}

//----------------------------------------------------------------------
// nullElem
template <typename T_RDATA>
inline T_RDATA nullElem() {
    return 0;
};

// Null cint16_t element
template <>
inline cint16_t nullElem() {
    cint16_t d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null cint32 element
template <>
inline cint32 nullElem() {
    cint32 d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null float element
template <>
inline float nullElem() {
    return 0.0;
};

// Null cint32 element
template <>
inline cfloat nullElem() {
    cfloat retVal;

    retVal.real = 0.0;
    retVal.imag = 0.0;
    return retVal;
};

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
                                                   : TP_POINT_SIZE == 2048 ? 11 : TP_POINT_SIZE == 4096 ? 12 : 0;
}

template <typename TT_TWIDDLE>
inline TT_TWIDDLE* fnGetTwiddleMasterBase(){};
template <>
inline cint16* fnGetTwiddleMasterBase<cint16>() {
    return (cint16*)twiddle_master_cint16;
};
template <>
inline cfloat* fnGetTwiddleMasterBase<cfloat>() {
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
template <typename TT_DATA>
inline TT_DATA castOutput(T_int_data<TT_DATA> sampleIn, const unsigned shift) {
    TT_DATA retVal; // default to mute warnings
    return retVal;
}
template <>
inline int16 castOutput<int16>(T_int_data<int16> sampleIn, const unsigned shift) {
    int16 retVal;
    // rounding is performed in the radix stages
    // retVal = (sampleIn.real + (1 << (shift-1))) >> shift;
    retVal = sampleIn.real;
    if (retVal >= C_PMAX16) {
        retVal = C_PMAX16;
    } else if (retVal < C_NMAX16) {
        retVal = C_NMAX16;
    }
    return retVal;
}
template <>
inline cint16 castOutput<cint16>(T_int_data<cint16> sampleIn, const unsigned shift) {
    cint16 retVal;
    // rounding is performed in the radix stages
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    if (sampleIn.real >= C_PMAX16) {
        retVal.real = C_PMAX16;
    } else if (sampleIn.real < C_NMAX16) {
        retVal.real = C_NMAX16;
    }
    if (sampleIn.imag >= C_PMAX16) {
        retVal.imag = C_PMAX16;
    } else if (sampleIn.imag < C_NMAX16) {
        retVal.imag = C_NMAX16;
    }
    return retVal;
}
template <>
inline int32 castOutput<int32>(T_int_data<int32> sampleIn, const unsigned shift) {
    int32 retVal;
    // rounding is performed in the radix stages
    retVal = sampleIn.real;
    if (retVal >= C_PMAX32) {
        retVal = C_PMAX32;
    } else if (retVal < C_NMAX32) {
        retVal = C_NMAX32;
    }
    return retVal;
}
template <>
inline cint32 castOutput<cint32>(T_int_data<cint32> sampleIn, const unsigned shift) {
    cint32 retVal;
    // rounding is performed in the radix stages
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    if (retVal.real >= C_PMAX32) {
        retVal.real = C_PMAX32;
    } else if (retVal.real < C_NMAX32) {
        retVal.real = C_NMAX32;
    }
    if (retVal.imag >= C_PMAX32) {
        retVal.imag = C_PMAX32;
    } else if (retVal.imag < C_NMAX32) {
        retVal.imag = C_NMAX32;
    }
    return retVal;
}
template <>
inline float castOutput<float>(T_int_data<float> sampleIn, const unsigned shift) {
    float retVal;
    // rounding is performed in the radix stages
    // retVal = (sampleIn.real + (float)(1<<(shift-1)))/(float)(1 << shift);
    retVal = sampleIn.real;
    return retVal;
}
template <>
inline cfloat castOutput<cfloat>(T_int_data<cfloat> sampleIn, const unsigned shift) {
    cfloat retVal;
    // rounding is performed in the radix stages
    retVal.real = sampleIn.real;
    retVal.imag = sampleIn.imag;
    // retVal.real = (sampleIn.real + (float)(1<<(shift-1)))/(float)(1 << shift);
    // retVal.imag = (sampleIn.imag + (float)(1<<(shift-1)))/(float)(1 << shift);
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
template <typename TI_D, typename T_TW>
inline void btfly(T_int_data<TI_D>& q0,
                  T_int_data<TI_D>& q1,
                  T_int_data<TI_D> d0,
                  T_int_data<TI_D> d1,
                  T_TW tw,
                  bool inv,
                  unsigned int shift) {
    printf("wrong\n");
}; // assumes downshift by weight of twiddle type, e.g. 15 for cint16

template <>
inline void btfly<cfloat, cfloat>(T_int_data<cfloat>& q0,
                                  T_int_data<cfloat>& q1,
                                  T_int_data<cfloat> d0,
                                  T_int_data<cfloat> d1,
                                  cfloat tw,
                                  bool inv,
                                  unsigned int shift) {
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
                                  unsigned int shift) {
    cint32 d1up;
    cint32 d1rot;
    cint64 d1rot64;
    const int64 kRoundConst = ((int64)1 << (shift - 1));
    d1up.real = (int32)d1.real;
    d1up.imag = (int32)d1.imag;
    const unsigned int shft = 15; // from cint16 for twiddle
    d1rot64 = cmpy<cint32, cint16>(d1up, tw, inv);
    q0.real = (int32)((((int64)d0.real << shft) + d1rot64.real + kRoundConst) >> shift);
    q0.imag = (int32)((((int64)d0.imag << shft) + d1rot64.imag + kRoundConst) >> shift);
    q1.real = (int32)((((int64)d0.real << shft) - d1rot64.real + kRoundConst) >> shift);
    q1.imag = (int32)((((int64)d0.imag << shft) - d1rot64.imag + kRoundConst) >> shift);
};

template <>
inline void btfly<cint16, cint16>(T_int_data<cint16>& q0,
                                  T_int_data<cint16>& q1,
                                  T_int_data<cint16> d0,
                                  T_int_data<cint16> d1,
                                  cint16 tw,
                                  bool inv,
                                  unsigned int shift) // the upshift variant
{
    cint32 d0up;
    cint32 d1up;
    cint64 d1rot64;
    cint32 d1rot;
    const int64 kRoundConst = ((int64)1 << (shift - 1));
    const unsigned int shft =
        15; // from cint16 for twiddle.Not to be confused with shift which can include scaling factor
    d0up.real = (int32)d0.real;
    d0up.imag = (int32)d0.imag;
    d1up.real = (int32)d1.real;
    d1up.imag = (int32)d1.imag;
    d1rot64 = cmpy<cint32, cint16>(d1up, tw, inv);
    q0.real = (int32)((((int64)d0up.real << shft) + d1rot64.real + kRoundConst) >> shift);
    q0.imag = (int32)((((int64)d0up.imag << shft) + d1rot64.imag + kRoundConst) >> shift);
    q1.real = (int32)((((int64)d0up.real << shft) - d1rot64.real + kRoundConst) >> shift);
    q1.imag = (int32)((((int64)d0up.imag << shft) - d1rot64.imag + kRoundConst) >> shift);
};

//}}}} //namespace

#endif // ifdef _DSPLIB_FFT_REF_UTILS_HPP_
