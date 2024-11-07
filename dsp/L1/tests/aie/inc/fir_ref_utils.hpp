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
#ifndef _DSPLIB_FIR_REF_UTILS_HPP_
#define _DSPLIB_FIR_REF_UTILS_HPP_

#include "device_defs.h"

/*
Reference model utilities.
This file contains sets of overloaded, templatized and specialized templatized
functions for use by the reference model classes.
*/

#define CASC_IN_TRUE true
#define CASC_IN_FALSE false
#define CASC_OUT_TRUE true
#define CASC_OUT_FALSE false

#define DUAL_IP_SINGLE 0
#define DUAL_IP_DUAL 1

#define USE_WINDOW_API 0
#define USE_STREAM_API 1

#define USE_COEFF_RELOAD_FALSE 0
#define USE_COEFF_RELOAD_TRUE 1

#define MIXER_MODE_0 0
#define MIXER_MODE_1 1
#define MIXER_MODE_2 2

// Round up to y
#define CEIL(x, y) (((x + y - 1) / y) * y)

static const int C_PMAX16 = std::numeric_limits<short int>::max();
static const int C_NMAX16 = std::numeric_limits<short int>::min();
static const int C_PMAX32 = std::numeric_limits<int32_t>::max();
static const int C_NMAX32 = std::numeric_limits<int32_t>::min();

// Accumulator types
template <typename T_D>
struct T_accRef {
    int64_t real, imag;
};
// template<> struct T_accRef<int16>   {int64_t    real;};
// template<> struct T_accRef<int32>   {int64_t    real;};
// template<> struct T_accRef<cint16>  {int64_t    real, imag;};
// template<> struct T_accRef<cint32>  {int64_t    real, imag;};
template <>
struct T_accRef<float> {
    float real;
};
#ifdef _SUPPORTS_BFLOAT16_
template <>
struct T_accRef<bfloat16> {
    float real;
};
#endif
// #if __SUPPORTS_CFLOAT__ == 1
template <>
struct T_accRef<cfloat> {
    float real, imag;
};
// #endif

// Zero accumulator type
template <typename T_A>
inline T_accRef<T_A> null_accRef(){};
template <>
inline T_accRef<int16> null_accRef() {
    T_accRef<int16> retVal;
    retVal.real = (int64_t)0;
    return retVal;
};
template <>
inline T_accRef<int32> null_accRef() {
    T_accRef<int32> retVal;
    retVal.real = (int64_t)0;
    return retVal;
};
template <>
inline T_accRef<cint16> null_accRef() {
    T_accRef<cint16> retVal;
    retVal.real = 0;
    retVal.imag = 0;
    return retVal;
};
template <>
inline T_accRef<cint32> null_accRef() {
    T_accRef<cint32> retVal;
    retVal.real = 0;
    retVal.imag = 0;
    return retVal;
};
template <>
inline T_accRef<float> null_accRef() {
    T_accRef<float> retVal;
    retVal.real = 0.0;
    return retVal;
};

#ifdef _SUPPORTS_BFLOAT16_
template <>
inline T_accRef<bfloat16> null_accRef() {
    T_accRef<bfloat16> retVal;
    retVal.real = 0.0;
    return retVal;
};
#endif

// #if __SUPPORTS_CFLOAT__ == 1
template <>
inline T_accRef<cfloat> null_accRef() {
    T_accRef<cfloat> retVal;
    retVal.real = 0.0;
    retVal.imag = 0.0;
    return retVal;
};
// #endif

// Rounding and shift function
// this function depends on the rounding mode macros to be set correctly in the tools
inline int64_t rounding(int rndMode, int shift, int64_t accum) {
    int64_t round_const = shift == 0 ? 0 : ((int64_t)1 << (shift - 1)) - 1; // 0.0111...
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
            case rnd_sym_floor:
                if (accum < 0) {
                    accum += ((1 << shift) - 1);
                } else {
                    break;
                }
                break;
            case rnd_sym_ceil:
                if (accum < 0) {
                    // not addition for truncation, just lose the bits during shift.
                    // accum += round_const;
                } else {
                    accum += ((1 << shift) - 1);
                }
                break;
            default:
                // unsupported, so error
                printf("Error: unrecognised value for ROUND_MODE %d\n", rndMode);
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

template <typename T>
inline void roundAcc(int rndMode, int shift, T_accRef<T>& accum) {}

inline void roundAcc(int rndMode, int shift, T_accRef<int16>& accum) {
    // cint64_t ret;
    accum.real = rounding(rndMode, shift, accum.real);
    // return ret;
};

inline void roundAcc(int rndMode, int shift, T_accRef<int32>& accum) {
    // cint64_t ret;
    accum.real = rounding(rndMode, shift, accum.real);
    // return ret;
};

inline void roundAcc(int rndMode, int shift, T_accRef<cint32>& accum) {
    // cint64_t ret;
    accum.real = rounding(rndMode, shift, accum.real);
    accum.imag = rounding(rndMode, shift, accum.imag);
    // return ret;
};
inline void roundAcc(int rndMode, int shift, T_accRef<cint16>& accum) {
    // cint64_t ret;
    accum.real = rounding(rndMode, shift, accum.real);
    accum.imag = rounding(rndMode, shift, accum.imag);
    // return ret;
};

// Saturation
template <typename T_A>
void saturateAcc(T_A& accum, unsigned int satMode = 1){};
template <>
inline void saturateAcc(T_accRef<int16>& accum, unsigned int satMode) {
    if (satMode == 0 || satMode == 2) {
    }                        // Mode: none or invalid
    else if (satMode == 1) { // Mode: saturate
        if (accum.real >= C_PMAX16) {
            accum.real = C_PMAX16;
        } else if (accum.real <= C_NMAX16) {
            accum.real = C_NMAX16;
        }
    } else if (satMode == 3) { // Mode: symmetric
        if (accum.real >= C_PMAX16) {
            accum.real = C_PMAX16;
        } else if (accum.real <= C_NMAX16) {
            accum.real = C_NMAX16 + 1; // Cap at int min +1
        }
    }
};

template <>
inline void saturateAcc(T_accRef<int32>& accum, unsigned int satMode) {
    // T_accRef<int32> ret;
    // Saturation
    if (satMode == 0 || satMode == 2) {
    }                        // Mode: none or invalid
    else if (satMode == 1) { // Mode: saturate
        if (accum.real >= C_PMAX32) {
            accum.real = C_PMAX32;
        } else if (accum.real <= C_NMAX32) {
            accum.real = C_NMAX32;
        }
    } else if (satMode == 3) { // Mode: symmetric
        if (accum.real >= C_PMAX32) {
            accum.real = C_PMAX32;
        } else if (accum.real <= C_NMAX32) {
            accum.real = C_NMAX32 + 1; // Cap at int min +1
        }
    }
};
template <>
inline void saturateAcc(T_accRef<cint16>& accum, unsigned int satMode) {
    // T_accRef<cint16> ret;
    if (satMode == 0 || satMode == 2) {
    }                        // Mode: none or invalid
    else if (satMode == 1) { // Mode: saturate
        if (accum.real >= C_PMAX16) {
            accum.real = C_PMAX16;
        } else if (accum.real <= C_NMAX16) {
            accum.real = C_NMAX16;
        }
        if (accum.imag >= C_PMAX16) {
            accum.imag = C_PMAX16;
        } else if (accum.imag <= C_NMAX16) {
            accum.imag = C_NMAX16;
        }
    } else if (satMode == 3) { // Mode: symmetric
        if (accum.real >= C_PMAX16) {
            accum.real = C_PMAX16;
        } else if (accum.real <= C_NMAX16) {
            accum.real = C_NMAX16 + 1; // Cap at int min +1
        }
        if (accum.imag >= C_PMAX16) {
            accum.imag = C_PMAX16;
        } else if (accum.imag <= C_NMAX16) {
            accum.imag = C_NMAX16 + 1; // Cap at int min +1
        }
    }
    // return ret;
};
template <>
inline void saturateAcc(T_accRef<cint32>& accum, unsigned int satMode) {
    // T_accRef<cint32> ret;
    if (satMode == 0 || satMode == 2) {
    }                        // Mode: none or invalid
    else if (satMode == 1) { // Mode: saturate
        if (accum.real >= C_PMAX32) {
            accum.real = C_PMAX32;
        } else if (accum.real <= C_NMAX32) {
            accum.real = C_NMAX32;
        }
        if (accum.imag >= C_PMAX32) {
            accum.imag = C_PMAX32;
        } else if (accum.imag <= C_NMAX32) {
            accum.imag = C_NMAX32;
        }
    } else if (satMode == 3) { // Mode: symmetric
        if (accum.real >= C_PMAX32) {
            accum.real = C_PMAX32;
        } else if (accum.real <= C_NMAX32) {
            accum.real = C_NMAX32 + 1; // Cap at int min +1
        }
        if (accum.imag >= C_PMAX32) {
            accum.imag = C_PMAX32;
        } else if (accum.imag <= C_NMAX32) {
            accum.imag = C_NMAX32 + 1; // Cap at int min +1
        }
    }
    // return ret;
};

// Cast
template <typename T>
T castAcc(T_accRef<T> acc) {
    return 0;
};
template <>
inline int16_t castAcc(T_accRef<int16_t> acc) {
    return (int16_t)acc.real;
};
template <>
inline int32_t castAcc(T_accRef<int32_t> acc) {
    return (int32_t)acc.real;
};
template <>
inline float castAcc(T_accRef<float> acc) {
    return (float)acc.real;
};
#ifdef _SUPPORTS_BFLOAT16_
template <>
inline bfloat16 castAcc(T_accRef<bfloat16> acc) {
    return (bfloat16)acc.real;
};
#endif
template <>
inline cint16_t castAcc(T_accRef<cint16_t> acc) {
    cint16_t cacc16;
    cacc16.real = (int16)acc.real;
    cacc16.imag = (int16)acc.imag;
    return cacc16;
};
template <>
inline cint32_t castAcc(T_accRef<cint32_t> acc) {
    cint32_t cacc32;
    cacc32.real = (int32)acc.real;
    cacc32.imag = (int32)acc.imag;
    return cacc32;
};

// #if __SUPPORTS_CFLOAT__ == 1
template <>
inline cfloat castAcc(T_accRef<cfloat> acc) {
    cfloat caccfloat;
    caccfloat.real = (float)acc.real;
    caccfloat.imag = (float)acc.imag;
    return caccfloat;
};
// #endif

// Multiply
template <typename T_D, typename T_C, typename T_A = T_D>
void multiplyAcc(T_accRef<T_A>& accum, T_D data, T_C coeff){};
template <>
inline void multiplyAcc(T_accRef<int16_t>& accum, int16_t data, int16_t coeff) {
    accum.real += (int64_t)data * coeff;
};
template <>
inline void multiplyAcc(T_accRef<cint16_t>& accum, cint16_t data, int16_t coeff) {
    accum.real += (int64_t)data.real * coeff;
    accum.imag += (int64_t)data.imag * coeff;
};
template <>
inline void multiplyAcc(T_accRef<cint16_t>& accum, int16_t data, cint16_t coeff) {
    accum.real += data * (int64_t)coeff.real;
    accum.imag += data * (int64_t)coeff.imag;
};
template <>
inline void multiplyAcc(T_accRef<cint16_t>& accum, cint16_t data, cint16_t coeff) {
    accum.real += (int64_t)coeff.real * (int64_t)data.real - (int64_t)coeff.imag * (int64_t)data.imag;
    accum.imag += (int64_t)coeff.real * (int64_t)data.imag + (int64_t)coeff.imag * (int64_t)data.real;
};
template <>
inline void multiplyAcc(T_accRef<int32_t>& accum, int16_t data, int16_t coeff) {
    accum.real += (int64_t)data * coeff;
};
template <>
inline void multiplyAcc(T_accRef<int32_t>& accum, int32_t data, int16_t coeff) {
    accum.real += (int64_t)data * coeff;
};
template <>
inline void multiplyAcc(T_accRef<int16_t>& accum, int16_t data, int32_t coeff) {
    accum.real += (int64_t)data * coeff;
};
template <>
inline void multiplyAcc(T_accRef<int32_t>& accum, int16_t data, int32_t coeff) {
    accum.real += (int64_t)data * coeff;
};
template <>
inline void multiplyAcc(T_accRef<int32_t>& accum, int32_t data, int32_t coeff) {
    accum.real += (int64_t)data * coeff;
};
template <>
inline void multiplyAcc(T_accRef<cint16_t>& accum, cint16_t data, int32_t coeff) {
    accum.real += (int64_t)data.real * (int64_t)coeff;
    accum.imag += (int64_t)data.imag * (int64_t)coeff;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, cint16_t data, int16_t coeff) {
    accum.real += (int64_t)data.real * (int64_t)coeff;
    accum.imag += (int64_t)data.imag * (int64_t)coeff;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, cint16_t data, int32_t coeff) {
    accum.real += (int64_t)data.real * (int64_t)coeff;
    accum.imag += (int64_t)data.imag * (int64_t)coeff;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, int32_t data, cint16_t coeff) {
    accum.real += (int64_t)data * (int64_t)coeff.real;
    accum.imag += (int64_t)data * (int64_t)coeff.imag;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, cint32_t data, int16_t coeff) {
    accum.real += (int64_t)data.real * coeff;
    accum.imag += (int64_t)data.imag * coeff;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, int16_t data, cint32_t coeff) {
    accum.real += (int64_t)data * (int64_t)coeff.real;
    accum.imag += (int64_t)data * (int64_t)coeff.imag;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, cint32_t data, int32_t coeff) {
    accum.real += (int64_t)data.real * coeff;
    accum.imag += (int64_t)data.imag * coeff;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, int32_t data, cint32_t coeff) {
    accum.real += (int64_t)data * (int64_t)coeff.real;
    accum.imag += (int64_t)data * (int64_t)coeff.imag;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, cint32_t data, cint16_t coeff) {
    accum.real += (int64_t)coeff.real * (int64_t)data.real - (int64_t)coeff.imag * (int64_t)data.imag;
    accum.imag += (int64_t)coeff.real * (int64_t)data.imag + (int64_t)coeff.imag * (int64_t)data.real;
};
template <>
inline void multiplyAcc(T_accRef<cint16_t>& accum, cint16_t data, cint32_t coeff) {
    accum.real += (int64_t)coeff.real * (int64_t)data.real - (int64_t)coeff.imag * (int64_t)data.imag;
    accum.imag += (int64_t)coeff.real * (int64_t)data.imag + (int64_t)coeff.imag * (int64_t)data.real;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, cint16_t data, cint16_t coeff) {
    accum.real += (int64_t)coeff.real * (int64_t)data.real - (int64_t)coeff.imag * (int64_t)data.imag;
    accum.imag += (int64_t)coeff.real * (int64_t)data.imag + (int64_t)coeff.imag * (int64_t)data.real;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, cint16_t data, cint32_t coeff) {
    accum.real += (int64_t)coeff.real * (int64_t)data.real - (int64_t)coeff.imag * (int64_t)data.imag;
    accum.imag += (int64_t)coeff.real * (int64_t)data.imag + (int64_t)coeff.imag * (int64_t)data.real;
};
template <>
inline void multiplyAcc(T_accRef<cint32_t>& accum, cint32_t data, cint32_t coeff) {
    accum.real += (int64_t)coeff.real * (int64_t)data.real - (int64_t)coeff.imag * (int64_t)data.imag;
    accum.imag += (int64_t)coeff.real * (int64_t)data.imag + (int64_t)coeff.imag * (int64_t)data.real;
};
template <>
inline void multiplyAcc(T_accRef<float>& accum, float data, float coeff) {
    accum.real += (float)data * coeff;
};

// #if __SUPPORTS_CFLOAT__ == 1
template <>
inline void multiplyAcc(T_accRef<cfloat>& accum, cfloat data, float coeff) {
    accum.real += (float)data.real * coeff;
    accum.imag += (float)data.imag * coeff;
};
template <>
inline void multiplyAcc(T_accRef<cfloat>& accum, float data, cfloat coeff) {
    accum.real += data * (float)coeff.real;
    accum.imag += data * (float)coeff.imag;
};
template <>
inline void multiplyAcc(T_accRef<cfloat>& accum, cfloat data, cfloat coeff) {
    accum.real += (float)coeff.real * (float)data.real - (float)coeff.imag * (float)data.imag;
    accum.imag += (float)coeff.real * (float)data.imag + (float)coeff.imag * (float)data.real;
};
// #endif

// Multiply Accumulate - using UCT shift.
template <typename T_D>
void multiplyAccUct(T_accRef<T_D>& accum, T_D data, unsigned int shift){};
template <>
inline void multiplyAccUct(T_accRef<int16_t>& accum, int16_t data, unsigned int shift) {
    accum.real += ((int64_t)data << shift);
};
template <>
inline void multiplyAccUct(T_accRef<cint16_t>& accum, cint16_t data, unsigned int shift) {
    accum.real += ((int64_t)data.real << shift);
    accum.imag += ((int64_t)data.imag << shift);
};
template <>
inline void multiplyAccUct(T_accRef<int32_t>& accum, int32_t data, unsigned int shift) {
    accum.real += (int64_t)data << shift;
};
template <>
inline void multiplyAccUct(T_accRef<cint32_t>& accum, cint32_t data, unsigned int shift) {
    accum.real += (int64_t)data.real << shift;
    accum.imag += (int64_t)data.imag << shift;
};
template <>
inline void multiplyAccUct(T_accRef<float>& accum, float data, unsigned int shift) {
    accum.real += (float)data * (float)(1 << shift);
};
// #if __SUPPORTS_CFLOAT__ == 1
template <>
inline void multiplyAccUct(T_accRef<cfloat>& accum, cfloat data, unsigned int shift) {
    accum.real += (float)data.real * (float)(1 << shift);
    accum.imag += (float)data.imag * (float)(1 << shift);
};
// #endif

// function to return Margin length.
template <unsigned int TP_FIR_LEN, typename TT_DATA, int TP_MODIFY_MARGIN_OFFSET = 0>
constexpr unsigned int fnFirMargin() {
    return CEIL(TP_FIR_LEN, (32 / sizeof(TT_DATA)));
};

// function to return Margin length.
template <size_t TP_FIR_LEN, typename TT_DATA, int TP_TDM_CHANNELS = 1>
constexpr unsigned int fnTDMFirMargin() {
    return CEIL(((TP_FIR_LEN - 1) * (TP_TDM_CHANNELS)), (32 / sizeof(TT_DATA)));
};

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

// #if __SUPPORTS_CFLOAT__ == 1
// Null cint32 element
template <>
inline cfloat nullElem() {
    cfloat retVal;

    retVal.real = 0.0;
    retVal.imag = 0.0;
    return retVal;
};
// #endif

template <typename TT>
struct realType {
    using type = int16;
};
template <>
struct realType<cint32> {
    using type = int32;
};
template <>
struct realType<cfloat> {
    using type = float;
};
#ifdef _SUPPORTS_BFLOAT16_
template <>
struct realType<cbfloat16> {
    using type = bfloat16;
};
#endif // _SUPPORTS_BFLOAT16_
template <typename TT>
using realType_t = typename realType<TT>::type;

template <typename TT>
struct complexType {
    using type = cint16;
};
template <>
struct complexType<int32> {
    using type = cint32;
};
template <>
struct complexType<float> {
    using type = cfloat;
};
#ifdef _SUPPORTS_BFLOAT16_
template <>
struct complexType<bfloat16> {
    using type = cbfloat16;
};
#endif // _SUPPORTS_BFLOAT16_
template <typename TT>
using complexType_t = typename complexType<TT>::type;

#endif // ifdef _DSPLIB_FIR_REF_UTILS_HPP_
