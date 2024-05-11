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
#ifndef _DSPLIB_REF_UTILS_HPP_
#define _DSPLIB_REF_UTILS_HPP_

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
            default:
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

#endif //_DSPLIB_REF_UTILS_HPP_
