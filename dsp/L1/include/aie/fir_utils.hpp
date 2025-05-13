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
#pragma once

#ifndef _DSPLIB_FIR_UTILS_HPP_
#define _DSPLIB_FIR_UTILS_HPP_

/* This file exists to hold utility functions for kernels in general, not specific
   to one library element. Also, the functions in this file do not use vector types
   or intrinsics so are fit for use by aiecompiler and kernel constructors.
*/

#include <stdio.h>
#include <adf.h>
#include "device_defs.h"
#include "fir_params_defaults.hpp"

using namespace adf;

#ifdef __SUPPORTS_ACC64__
using input_cascade_cacc = input_cascade<cacc64>;
using output_cascade_cacc = output_cascade<cacc64>;
#else
// else defaults to 48-bits
using input_cascade_cacc = input_cascade<cacc48>;
using output_cascade_cacc = output_cascade<cacc48>;
#endif //__SUPPORTS_ACC64__

struct empty {
    float real;
    float imag;
};

// The following maximums are the maximums tested. The function may work for larger values.
#ifndef FIR_LEN_MAX
#define FIR_LEN_MAX 240
#endif
// Realistically this FIR_RANGE_LEN_MIN, which for cfloats should be 2; but we always pad up to at least numCols so
// lower is fine.
#define FIR_LEN_MIN 1
#define SHIFT_MAX 62
#define SHIFT_MIN 0

#define SAT_MODE_MAX __SATURATION_MODES__
#define SAT_MODE_MIN 0
#define ROUND_MAX __ROUNDING_MODES__
#define ROUND_MIN 0

#define INTERPOLATE_FACTOR_MAX 16
#define INTERPOLATE_FACTOR_MIN 1

#define DECIMATE_FACTOR_MAX 7
#define DECIMATE_FACTOR_MIN 2

#define IS_ASYM 0
#define IS_SYM 1

// CEIL and TRUNC are common utilities where x is rounded up (CEIL) or down (TRUNC)
// until a value which is multiple of y is found. This may be x.
// e.g. CEIL(10,8) = 16, TRUNC(10, 8) = 8
#define CEIL(x, y) (((x + y - 1) / y) * y)
#define TRUNC(x, y) (((x) / y) * y)
#define FLOOR(X, Y) X >= 0 ? (int)((int)(X) / (int)(Y)) : (int)((int)((X) - (Y) + 1) / (int)(Y))
#define ABSDIFF(x, y) ((x) > (y) ? (x - y) : (y - x))
#define ABS(x) ((x) > 0 ? (x) : -(x))
#define MIN(x, y) ((x) > (y) ? (y) : (x))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

// Pragma unroll complains if you try to unroll(0);
// It's safe to just unroll(1) in this circumstance.
#define GUARD_ZERO(x) ((x) > 0 ? (x) : 1)

#define CASC_IN_TRUE true
#define CASC_IN_FALSE false
#define CASC_OUT_TRUE true
#define CASC_OUT_FALSE false

#define DUAL_IP_SINGLE 0
#define DUAL_IP_DUAL 1

#define USE_COEFF_RELOAD_FALSE 0
#define USE_COEFF_RELOAD_TRUE 1

#define USE_WINDOW_API 0
#define USE_STREAM_API 1

#define USE_INBUILT_SINCOS 0
#define USE_LUT_SINCOS 1

#define MIXER_MODE_0 0
#define MIXER_MODE_1 1
#define MIXER_MODE_2 2

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

namespace xf {
namespace dsp {
namespace aie {

static constexpr unsigned int kUpdWSize = 256 / 8;    // Upd_w size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kZBuffSize = 256 / 8;   // Zbuff size in Bytes (256bit) - const for all data/coeff types
static constexpr unsigned int kXYBuffSize = 1024 / 8; // XYbuff size in Bytes (1024bit) - const for all data/coeff types
static constexpr unsigned int kBuffSize128Byte = 128; // 1024-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;   // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;   // 256-bit buffer size in Bytes

// Number of data type samples in a 1024-bit buffer
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnSamplesIn1024() {
    return kXYBuffSize / sizeof(TT_DATA);
}

// function to return base type of accumulator for FFT/Widget
template <typename TT_DATA>
struct tFFTAccBaseType {
    using type = acc48;
};

template <>
struct tFFTAccBaseType<cint16> {
    using type = cacc32;
};
template <>
struct tFFTAccBaseType<cint32> {
    using type = cacc64;
};
template <>
struct tFFTAccBaseType<cfloat> {
    using type = caccfloat;
};

template <typename TT_DATA, typename TT_COEFF>
struct tAccBaseType {
    using type = acc48;
};
#ifdef __SUPPORTS_ACC48__
template <>
struct tAccBaseType<int16, int16> {
    using type = acc48;
};
template <>
struct tAccBaseType<cint16, int16> {
    using type = cacc48;
};
template <>
struct tAccBaseType<int16, cint16> {
    using type = cacc48;
};
template <>
struct tAccBaseType<cint16, cint16> {
    using type = cacc48;
};
template <>
struct tAccBaseType<int32, int16> {
    using type = acc80;
};
template <>
struct tAccBaseType<int32, int32> {
    using type = acc80;
};
template <>
struct tAccBaseType<int32, cint16> {
    using type = cacc80;
};
template <>
struct tAccBaseType<int32, cint32> {
    using type = cacc80;
};
template <>
struct tAccBaseType<cint32, int16> {
    using type = cacc80;
};
template <>
struct tAccBaseType<cint32, cint16> {
    using type = cacc80;
};
template <>
struct tAccBaseType<cint32, int32> {
    using type = cacc80;
};
template <>
struct tAccBaseType<cint32, cint32> {
    using type = cacc80;
};
template <>
struct tAccBaseType<int16, int32> {
    using type = acc80;
};
template <>
struct tAccBaseType<int16, cint32> {
    using type = cacc80;
};
template <>
struct tAccBaseType<cint16, int32> {
    using type = cacc80;
};
template <>
struct tAccBaseType<cint16, cint32> {
    using type = cacc80;
};
template <>
struct tAccBaseType<float, float> {
    using type = accfloat;
};
template <>
struct tAccBaseType<float, cfloat> {
    using type = caccfloat;
};
template <>
struct tAccBaseType<cfloat, float> {
    using type = caccfloat;
};
template <>
struct tAccBaseType<cfloat, cfloat> {
    using type = caccfloat;
};
#endif //__SUPPORTS_ACC48__

#ifdef __SUPPORTS_ACC64__
template <>
struct tAccBaseType<int16, int16> {
    using type = acc64;
};
template <>
struct tAccBaseType<cint16, int16> {
    using type = cacc64;
};
template <>
struct tAccBaseType<cint16, cint16> {
    using type = cacc64;
};
template <>
struct tAccBaseType<int32, int16> {
    using type = acc64;
};
template <>
struct tAccBaseType<int32, int32> {
    using type = acc64;
};
template <>
struct tAccBaseType<cint32, int16> {
    using type = cacc64;
};
template <>
struct tAccBaseType<cint32, cint16> {
    using type = cacc64;
};
template <>
struct tAccBaseType<cint32, int32> {
    using type = cacc64;
};
template <>
struct tAccBaseType<cint32, cint32> {
    using type = cacc64;
};
template <>
struct tAccBaseType<int16, int32> {
    using type = acc64;
};
template <>
struct tAccBaseType<cint16, int32> {
    using type = cacc64;
};
template <>
struct tAccBaseType<cint16, cint32> {
    using type = cacc64;
};
template <>
struct tAccBaseType<float, float> {
    using type = accfloat;
};
#endif //__SUPPORTS_ACC64__

template <typename TT_DATA, typename TT_COEFF>
using tAccBaseType_t = typename tAccBaseType<TT_DATA, TT_COEFF>::type;

template <typename TT_DATA, typename TT_COEFF>
struct tAccBaseTypeMul {
    using type = cacc48;
};
#ifdef __SUPPORTS_ACC48__
template <>
struct tAccBaseTypeMul<cint32, cint16> {
    using type = cacc48;
};

template <>
struct tAccBaseTypeMul<cint32, int32> {
    using type = cacc80;
};

template <>
struct tAccBaseTypeMul<cfloat, float> {
    using type = caccfloat;
};

template <>
struct tAccBaseTypeMul<cfloat, cfloat> {
    using type = caccfloat;
};

template <>
struct tAccBaseTypeMul<cint32, cint32> {
    using type = cacc80;
};
#endif //__SUPPORTS_ACC48__

#ifdef __SUPPORTS_ACC64__
template <>
struct tAccBaseTypeMul<cint32, cint16> {
    using type = cacc64;
};

template <>
struct tAccBaseTypeMul<cfloat, cfloat> {
    using type = caccfloat;
};

#endif //__SUPPORTS_ACC64__

// function to return the size of the acc,
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnAccSize() {
    return 0;
};

#ifdef __SUPPORTS_ACC48__
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int16, int16>() {
    return 48;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint16, int16>() {
    return 48;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int16, cint16>() {
    return 48;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint16, cint16>() {
    return 48;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int32, int16>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int32, int32>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int32, cint16>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int32, cint32>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint32, int16>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint32, cint16>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint32, int32>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint32, cint32>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int16, int32>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int16, cint32>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint16, int32>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint16, cint32>() {
    return 80;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<float, float>() {
    return 32;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<float, cfloat>() {
    return 32;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cfloat, float>() {
    return 32;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cfloat, cfloat>() {
    return 32;
};
#endif //__SUPPORTS_ACC48__

#ifdef __SUPPORTS_ACC64__
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int16, int16>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint16, int16>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint16, cint16>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int32, int16>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int32, int32>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint32, int16>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint32, cint16>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint32, int32>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint32, cint32>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<int16, int32>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint16, int32>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<cint16, cint32>() {
    return 64;
};
template <>
INLINE_DECL constexpr unsigned int fnAccSize<float, float>() {
    return 32;
};
#endif //__SUPPORTS_ACC64__

#if __MIN_REGSIZE__ == 128
// function to return the number of 768-bit acc's lanes for a type combo
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumLanes() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<int16, int16>() {
    return 16;
}; // 16x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint16, int16>() {
    return 8;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint16, cint16>() {
    return 8;
}; // 8x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<int32, int16>() {
    return 8;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<int32, int32>() {
    return 8;
}; // 8x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint32, int16>() {
    return 4;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint32, cint16>() {
    return 4;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint32, int32>() {
    return 4;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint32, cint32>() {
    return 2;
}; // 2x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<int16, int32>() {
    return 8;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint16, int32>() {
    return 4;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint16, cint32>() {
    return 4;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<float, float>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cfloat, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cfloat, cfloat>() {
    return 4;
};

// function to return the number of 384-bit acc's lanes for a type combo
// 80-bit accs (for 32-bit input types) are always 768-bit, apart from cint32/cint32 which offers a short, 384-bit acc
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumLanes384() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<int16, int16>() {
    return 8;
}; // 8x4
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint16, int16>() {
    return 4;
}; // 4x4
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint16, cint16>() {
    return 4;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<int32, int16>() {
    return 8;
}; // 8x2 - 80-bit
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<int32, int32>() {
    return 4;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint32, int16>() {
    return 4;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint32, cint16>() {
    return 4;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint32, int32>() {
    return 4;
}; // 4x1 or 2x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint32, cint32>() {
    return 2;
}; // 2x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<int16, int32>() {
    return 8;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint16, int32>() {
    return 4;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint16, cint32>() {
    return 4;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<float, float>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cfloat, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cfloat, cfloat>() {
    return 4;
};
#else
// for io buffer cases
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumLanes() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<int16, int16>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<int16, int32>() {
    return 32;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint16, cint32>() {
    return 16; // 8 - API min native length is 8, 16 lanes offers better performance.
};             //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint32, int32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint32, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes<cint32, cint32>() {
    return 8;
}; //
// template<> INLINE_DECL constexpr unsigned int fnNumLanes<float, float>() { return  8;};//
// for streams
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumLanes384() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<int16, int16>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<int16, int32>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint32, int16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint32, int32>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint32, cint16>() {
    return 4;
}; //
template <>
INLINE_DECL constexpr unsigned int fnNumLanes384<cint32, cint32>() {
    return 4;
}; //
#endif

#if __MIN_REGSIZE__ == 128
// function to return the number of columns for a tall-narrow atomic intrinsic for a type combo
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumCols() {
    return sizeof(TT_COEFF) == 2 ? 2 : 1;
};
template <>
INLINE_DECL constexpr unsigned int fnNumCols<int16, int32>() {
    return 2;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnNumCols<cint16, int32>() {
    return 2;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumCols<cint16, cint32>() {
    return 1;
}; // 4x1

// function to return the number of columns for a short-wide atomic intrinsic for a type combo
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumCols384() {
    return 2 * (sizeof(TT_COEFF) == 2 ? 2 : 1);
};
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<int16, int16>() {
    return 4;
}; // 8x4
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cint16, int16>() {
    return 4;
}; // 4x4
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cint16, cint16>() {
    return 2;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<int32, int16>() {
    return 2;
}; // 8x2 - 80-bit
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<int32, int32>() {
    return 2;
}; // 4x2 or 8x1
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cint32, int16>() {
    return 2;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cint32, cint16>() {
    return 1;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cint32, int32>() {
    return 1;
}; // 4x1 or 2x2
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cint32, cint32>() {
    return 1;
}; // 2x1
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<int16, int32>() {
    return 2;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cint16, int32>() {
    return 2;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cint16, cint32>() {
    return 1;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<float, float>() {
    return 1;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cfloat, float>() {
    return 1;
};
template <>
INLINE_DECL constexpr unsigned int fnNumCols384<cfloat, cfloat>() {
    return 1;
};
#endif
#else
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumCols() {
    return 4;
};
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnNumCols384() {
    return 4;
};
#endif

#if __MIN_REGSIZE__ == 128
// Define Data Width read from stream. Data types operating on long, 768-bit accs require 256-bits of input data.
template <typename T_D, typename T_C>
INLINE_DECL constexpr unsigned int fnStreamReadWidth() {
    return 128;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<int32, int16>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cint32, int16>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cint32, int32>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cint32, cint16>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<int16, int32>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cint16, int32>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cint16, cint32>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<float, float>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cfloat, float>() {
    return 256;
};
template <>
INLINE_DECL constexpr unsigned int fnStreamReadWidth<cfloat, cfloat>() {
    return 256;
};
#else
template <typename T_D, typename T_C>
INLINE_DECL constexpr unsigned int fnStreamReadWidth() {
    return 256;
};
#endif

// check if device has permute capabilites
constexpr unsigned int fnPermuteSupport() {
    return __HAS_ACCUM_PERMUTES__;
};

namespace fir {

enum eFIRVariant {
    kSrAsym = 0,
    kSrSym = 1,
    kIntHB = 2,
    kDecHB = 3,
    kIntAsym = 4,
    kDecAsym = 5,
    kDecSym = 6,
    kResamp = 7,
    kTDM = 8
};

inline const char* firToString(eFIRVariant fir_type) {
    switch (fir_type) {
        case kSrAsym:
            return "kSrAsym ";
            break;
        case kSrSym:
            return "kSrSym  ";
            break;
        case kIntHB:
            return "kIntHB  ";
            break;
        case kDecHB:
            return "kDecHB  ";
            break;
        case kIntAsym:
            return "kIntAsym";
            break;
        case kDecAsym:
            return "kDecAsym";
            break;
        case kDecSym:
            return "kDecSym ";
            break;
        case kResamp:
            return "kResamp ";
            break;
        case kTDM:
            return "kTDM ";
            break;
        default:
            return "[Unknown FIR type]";
    }
}
// Functions to support defensive checks
enum { enumUnknownType = 0, enumInt16, enumCint16, enumInt32, enumCint32, enumFloat, enumCfloat };
// function to return an enumeration of the data or coefficient type
template <typename TT_INPUT>
INLINE_DECL constexpr unsigned int fnEnumType() {
    return enumUnknownType;
}; // returns 0 as default. This can be trapped as an error;
template <>
INLINE_DECL constexpr unsigned int fnEnumType<int16>() {
    return enumInt16;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cint16>() {
    return enumCint16;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<int32>() {
    return enumInt32;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cint32>() {
    return enumCint32;
};
template <>
INLINE_DECL constexpr unsigned int fnEnumType<float>() {
    return enumFloat;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
INLINE_DECL constexpr unsigned int fnEnumType<cfloat>() {
    return enumCfloat;
};
#endif
// Function to trap illegal precision of DATA vs COEFF
// This defaults to legal which would be fail-unsafe if not used in conjunction with fnEnumType trap
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffSize() {
    return 1;
}; // default here is a legal combo
// template<>INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffSize<int16 , int32 >() {return 0;};
// template<>INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffSize<int16 , cint32>() {return 0;};
// template<>INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffSize<cint16, int32 >() {return 0;};
// template<>INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffSize<cint16, cint32>() {return 0;};

// Function to trap illegal real DATA vs complex COEFF
// This defaults to legal which would be fail-unsage is not used in conjunction with functions above.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffCmplx() {
    return 1;
}; // default here is a legal combo
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffCmplx<int16, cint16>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffCmplx<int16, cint32>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffCmplx<int32, cint32>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffCmplx<int32, cint16>() {
    return 0;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffCmplx<float, cfloat>() {
    return 0;
};
#endif

// Function to trap illegal combo of real and float types
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt() {
    return 1;
}; // default here is a legal combo
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<int16, float>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<cint16, float>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<int32, float>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<cint32, float>() {
    return 0;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<int16, cfloat>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<cint16, cfloat>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<int32, cfloat>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<cint32, cfloat>() {
    return 0;
};
#endif

template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<float, int16>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<float, cint16>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<float, int32>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<float, cint32>() {
    return 0;
};
#if __SUPPORTS_CFLOAT__ == 1
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<cfloat, int16>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<cfloat, cint16>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<cfloat, int32>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnTypeCheckDataCoeffFltInt<cfloat, cint32>() {
    return 0;
};
#endif
// Function to trap illegal  types
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnUnsupportedTypeCombo() {
    return 1;
}; // default here is a legal combo

#if __HAS_ACCUM_PERMUTES__ == 1
// Permutes require inflexible xsquare for 1 and 2 Byte data types.
template <>
INLINE_DECL constexpr unsigned int fnUnsupportedTypeCombo<int16, int16>() {
    return 0;
};
#endif

// IF input type
template <bool T_CASC_IN, typename T_D, unsigned int T_DUAL_IP = 0>
struct T_inputIF {
    static constexpr int kdummy = 16;
    T_D* __restrict inTapsPtr;
    T_D* __restrict inWindow;
    input_async_buffer<T_D>* inWindowLin = NULL;
    input_circular_buffer<T_D, extents<inherited_extent>, margin<kdummy> >* inWindowCirc =
        NULL; // never used in this config, but referred to by client code.
    input_circular_buffer<T_D, extents<inherited_extent>, margin<kdummy> >* inWindowReverse =
        NULL; // dummy, never used, but allows optional assignment
    input_stream<T_D>* __restrict inStream;
    input_stream<T_D>* __restrict inStream2;
    input_cascade_cacc* inCascade;
};

template <bool T_CASC_IN, typename T_D>
struct T_outputIF {};
template <typename T_D>
struct T_outputIF<CASC_OUT_FALSE, T_D> {
    T_D* __restrict outWindowPtr;
    output_circular_buffer<T_D>* outWindow = NULL;
    output_circular_buffer<T_D>* outWindow2 = NULL;
    output_cascade_cacc* outCascade;
    output_async_buffer<T_D>* broadcastWindow;
    output_stream<T_D>* __restrict outStream;
    output_stream<T_D>* __restrict outStream2;
};
template <typename T_D>
struct T_outputIF<CASC_OUT_TRUE, T_D> {
    T_D* __restrict outWindowPtr;
    static constexpr int kdummy = 16;
    input_circular_buffer<T_D, extents<inherited_extent>, margin<kdummy> >* outWindow =
        NULL; // internal buffer filled through cascade
    input_circular_buffer<T_D, extents<inherited_extent>, margin<kdummy> >* outWindow2 = NULL; // dummy
    output_cascade_cacc* outCascade;
    output_async_buffer<T_D>* broadcastWindow;
    output_stream<T_D>* __restrict outStream;
    output_stream<T_D>* __restrict outStream2;
};
// Handy enum for if conditions
enum kernelPositionState {
    middle_kernel_in_chain = 0,
    first_kernel_in_chain,
    last_kernel_in_chain,
    only_kernel,
    error_kernel
};

// Generate enum defined above for a given kernelPosition and cascade length. Return error_kernel for invalid configs,
// which should have a static assert.
constexpr kernelPositionState getKernelPositionState(unsigned int kernelPosition, unsigned int casc_length) {
    if (casc_length == 1) {
        if (kernelPosition == 0) return kernelPositionState::only_kernel;
        if (kernelPosition > 0) return kernelPositionState::error_kernel;
    } else if (casc_length > 1) {
        if (kernelPosition == casc_length - 1) {
            return kernelPositionState::last_kernel_in_chain;
        } else if (kernelPosition == 0) {
            return kernelPositionState::first_kernel_in_chain;
        } else if (kernelPosition < casc_length) {
            return kernelPositionState::middle_kernel_in_chain;
        } else {
            // kernelPosiiton is greater than cascadeLength
            return kernelPositionState::error_kernel;
        }
    } else {
        return kernelPositionState::error_kernel;
    }
    // anything else is an error
    return kernelPositionState::error_kernel;
};

//----------------------------------------------------------------------
// nullElem
template <typename T_RDATA>
INLINE_DECL T_RDATA nullElem() {
    return 0;
    //    return ::aie::zero<T_RDATA>();
};

// Null cint16_t element
template <>
INLINE_DECL cint16_t nullElem() {
    cint16_t d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null cint32 element
template <>
INLINE_DECL cint32 nullElem() {
    cint32 d;
    d.real = 0;
    d.imag = 0;
    return d;
};

// Null float element
template <>
INLINE_DECL float nullElem() {
    return 0.0;
};

// Null cint32 element
#if __SUPPORTS_CFLOAT__ == 1
template <>
INLINE_DECL cfloat nullElem() {
    cfloat retVal = {0.0, 0.0};

    retVal.real = 0.0;
    retVal.imag = 0.0;
    return retVal;
};
#endif

// function to return Margin length.
template <size_t TP_FIR_LEN, typename TT_DATA, int TP_MODIFY_MARGIN_OFFSET = 0>
INLINE_DECL constexpr unsigned int fnFirMargin() {
    if
        constexpr(TP_FIR_LEN == 1) {
            return CEIL(TP_FIR_LEN - TP_MODIFY_MARGIN_OFFSET, (__ALIGN_BYTE_SIZE__ / sizeof(TT_DATA)));
        }
    else {
        return CEIL(TP_FIR_LEN, (__ALIGN_BYTE_SIZE__ / sizeof(TT_DATA)));
    }
};

// function to return Margin length.
template <size_t TP_FIR_LEN,
          typename TT_DATA,
          int TP_TDM_CHANNELS = 1,
          int TP_INPUT_WINDOW_VSIZE = 1,
          int TP_ENABLE_INTERNAL_MARGIN = 0>
INLINE_DECL constexpr unsigned int fnTDMFirMargin() {
    // If window buffer containts multiple TDM frames it may be beneficial to use margin handling external to the
    // kernel.
    // If TP_FIR_LEN * TP_TDM_CHANNELS is greater than TP_INPUT_WINDOW_VSIZE, it is more beneficial to set external
    // margin to 0, set an internal buffer and fill it with data samples.
    // if
    //     constexpr(TP_ENABLE_INTERNAL_MARGIN == 1 && TP_FIR_LEN * TP_TDM_CHANNELS > TP_INPUT_WINDOW_VSIZE) { return 0;
    //     }
    // A massive benefit of using internal margin buffer is saving on the ping-pong size.
    // With the margin extracted out of ping-pong input buffer,
    // the design can service much greater number of channels per kernel.
    // Therefore, use the internal margin whenever possible.
    if
        constexpr(TP_ENABLE_INTERNAL_MARGIN == 1) { return 0; }
    else {
        return CEIL(((TP_FIR_LEN - 1) * (TP_TDM_CHANNELS)), (32 / sizeof(TT_DATA)));
    }
};

// Truncation. This function rounds x down to the next multiple of y (which may be x)
INLINE_DECL constexpr int fnTrunc(unsigned int x, unsigned int y) {
    return TRUNC(x, y);
};

// Calculate FIR range for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, int TP_Rnd = 1>
INLINE_DECL constexpr int fnFirRange() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    return ((fnTrunc(TP_FL, TP_Rnd * TP_CL) / TP_CL) +
            ((TP_FL - fnTrunc(TP_FL, TP_Rnd * TP_CL)) >= TP_Rnd * (TP_KP + 1) ? TP_Rnd : 0));
}
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, int TP_Rnd = 1>
INLINE_DECL constexpr int fnFirRangeRem() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    // this is for last in the cascade
    return ((fnTrunc(TP_FL, TP_Rnd * TP_CL) / TP_CL) + ((TP_FL - fnTrunc(TP_FL, TP_Rnd * TP_CL)) % TP_Rnd));
}

// Calculate FIR range offset for cascaded kernel
template <unsigned int TP_FL, unsigned int TP_CL, int TP_KP, int TP_Rnd = 1, int TP_Sym = 1>
INLINE_DECL constexpr int fnFirRangeOffset() {
    // TP_FL - FIR Length, TP_CL - Cascade Length, TP_KP - Kernel Position
    return (TP_KP * (fnTrunc(TP_FL, TP_Rnd * TP_CL) / TP_CL) +
            ((TP_FL - fnTrunc(TP_FL, TP_Rnd * TP_CL)) >= TP_Rnd * TP_KP
                 ? TP_Rnd * TP_KP
                 : (fnTrunc(TP_FL, TP_Rnd) - fnTrunc(TP_FL, TP_Rnd * TP_CL)))) /
           TP_Sym;
}

//
template <typename T_D>
INLINE_DECL T_D formatUpshiftCt(T_D inVal) {
    // Do nothing for types other than 16-bit integers
    return inVal;
}

template <>
INLINE_DECL int16 formatUpshiftCt(int16 inVal) {
    const unsigned int kMaxUpshiftVal = 16;
    int16 retVal;
    // Make sure value is within UCT supported range (0 - 16).
    retVal = inVal % kMaxUpshiftVal;
    return retVal;
}
template <>
INLINE_DECL cint16 formatUpshiftCt(cint16 inVal) {
    const unsigned int kMaxUpshiftVal = 16;
    cint16 retVal;
    // Make sure value is within UCT supported range (0 - 16).
    retVal.real = inVal.real % kMaxUpshiftVal;
    retVal.imag = 0;
    return retVal;
}

template <typename T_D>
INLINE_DECL int16 getUpshiftCt(T_D inVal) {
    // Do nothing for types other than 16-bit integers
    return 0;
}

template <>
INLINE_DECL int16 getUpshiftCt(int16 inVal) {
    const unsigned int kMaxUpshiftVal = 16;
    int16 retVal;
    // Make sure value is within UCT supported range (0 - 16).
    retVal = inVal % kMaxUpshiftVal;
    return retVal;
}
template <>
INLINE_DECL int16 getUpshiftCt(cint16 inVal) {
    const unsigned int kMaxUpshiftVal = 16;
    int16 retVal;
    // Make sure value is within UCT supported range (0 - 16).
    retVal = inVal.real % kMaxUpshiftVal;
    return retVal;
}

template <bool TP_SYM_FACTOR, unsigned int TP_DUAL_IP, unsigned int TP_API, unsigned int TP_KERNEL_POSITION>
INLINE_DECL constexpr bool fnBuffIsLin() {
    if
        constexpr(TP_SYM_FACTOR == IS_ASYM) {
            if
                constexpr(TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0) { return true; }
            else {
                return false;
            }
        }
    else if (TP_SYM_FACTOR == IS_SYM) {
        if
            constexpr((TP_API == USE_WINDOW_API && TP_KERNEL_POSITION != 0) ||
                      ((TP_API == USE_STREAM_API) && (TP_DUAL_IP == DUAL_IP_DUAL) && (TP_KERNEL_POSITION != 0))) {
                return true;
            }
        else {
            return false;
        }
    }
    return false;
};
}
}
}
}
#endif // _DSPLIB_FIR_UTILS_HPP_
