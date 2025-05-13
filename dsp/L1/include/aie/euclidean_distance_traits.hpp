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
#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_TRAITS_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_TRAITS_HPP_

/*
    Euclidean Distance traits.
    This file contains sets of overloaded, templatized and specialized templatized functions which
    encapsulate properties of the intrinsics used by the main kernal class. Specifically,
    this file does not contain any vector types or intrinsics since it is required for construction
    and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include <stdio.h>
#include <type_traits>
#include <adf.h>
#include "device_defs.h"
#include "aie_api/aie_adf.hpp"
#include "fir_utils.hpp"

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif
#define ROUND(X, Y) (((X % Y) > ((Y + 1) / 2)) ? ((int)(X + Y - 1) / (int)Y) : ((int)X / (int)Y))
// using namespace std;
namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

static constexpr unsigned int kMaxBitsRegY = 1024;                            // Max number of bits on Y Register.
static constexpr unsigned int kMaxBitsRegX = 512;                             // Max number of bits on X Register.
static constexpr unsigned int kMaxBitsRegW = 256;                             // Max number of bits on W Register.
static constexpr unsigned int kMaxBitsRegV = 128;                             // Max number of bits on V Register.
static constexpr unsigned int kMaxBitsLoadOnAie = 256;                        // Max number of bits Load on AIE
static constexpr unsigned int kMaxBytesLoadOnAie = 32;                        // Max number of bytes (256/8) Load on AIE
static constexpr unsigned int kMaxBufferLenOnAieInBytes = __DATA_MEM_BYTES__; // Max buffer Length
static constexpr unsigned int kShiftFactor2 = 2;                              // MulFactor2
static constexpr unsigned int kBuffSize16Byte = 16;                           // 128-bit buffer size in Bytes
static constexpr unsigned int kBuffSize32Byte = 32;                           // 256-bit buffer size in Bytes
static constexpr unsigned int kBuffSize64Byte = 64;                           // 512-bit buffer size in Bytes
static constexpr unsigned int kBuffSize128Byte = 128;                         // 1024-bit buffer size in Bytes

static constexpr unsigned int kUnrollFactor = 8;      // Unroll Factor
static constexpr unsigned int kFixedDimOfED = 4;      // Unroll Factor
static constexpr unsigned int kVecSize16OfBf16 = 16;  // v16bfloat16
static constexpr unsigned int kVecSize32OfBf16 = 32;  // v32bfloat16
static constexpr unsigned int kVecSizeOfAccum16 = 16; // v16accfloat
static constexpr unsigned int kVecSizeOfAccum32 = 32; // v32accfloat
static constexpr unsigned int kUpshiftFactor2 = 2;    // v32accfloat

// Function to return the default lanes based on choosen architecture
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnEDNumLanes() {
    return 16; // defualt return for AIE2
};

// Function to return the default MULS/MACS based on choosen architecture
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnEDNumMuls() {
    return 256; // defualt return for AIE2
};

// Function to return the default sample size of P data
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnSampleSize() {
    return 8; // defualt sample size is 8
};

// for io buffer cases
// function to return the number of acc's lanes for a type combo
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr unsigned int fnEDNumLanes<float>() {
    return 8;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int fnEDNumMuls<float>() {
    return 8;
}; //
#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
// AIE-2
template <>
INLINE_DECL constexpr unsigned int fnEDNumLanes<float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnEDNumLanes<bfloat16>() {
    return 16;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int fnEDNumMuls<float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnEDNumMuls<bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type of P Data
template <>
INLINE_DECL constexpr unsigned int fnSampleSize<float>() {
    return 32;
}; //

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnSampleSize<bfloat16>() {
    return 16;
}; //
#endif

// Function returns number of columns used by MUL/MACs in Euclidean Distance
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnGetNumofMULs() {
    return fnEDNumMuls<TT_DATA>();
};

// Function to return the number of lanes based on given data types
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnGetNumofLanes() {
    return fnEDNumLanes<TT_DATA>();
};

// Function to return the number of points based on given data types
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnGetNumofPoints() {
    return (fnEDNumMuls<TT_DATA>() / fnEDNumLanes<TT_DATA>());
};

// Function to return Maximum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMaxLen() {
    return (kMaxBufferLenOnAieInBytes >> kShiftFactor2 / sizeof(TT_DATA));
};

// Function to return Minimum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMinLen() {
    return (((kMaxBitsLoadOnAie << 1) / fnSampleSize<TT_DATA>()));
};

// Function to return true or false by checking given length is in range or not
template <typename TT_DATA, unsigned int sigLen>
constexpr bool isLenInRange() {
    unsigned int minDataLoad = (kMaxBitsLoadOnAie / fnSampleSize<TT_DATA>());
    bool checkLen = false;

    if ((sigLen >= getMinLen<TT_DATA>()) && (sigLen <= getMaxLen<TT_DATA>())) {
        checkLen = (((sigLen % minDataLoad) == 0) ? true : false);
    }
    return checkLen;
};

// float and bfloat16 are only the supported data types
// Configuration Defensive check functions
//----------------------------------------------------------------------

// Configuration Defensive check function for input data types whether supported or not
template <typename TT_DATA>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs() {
    return false;
};
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int16>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cfloat>() {
    return false;
};
#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int8>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int16>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<int32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint16>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cint32>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<cfloat>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<bfloat16>() {
    return true;
};
#endif
// Configuration Defensive check function to check Length of F Signal and G Signal
template <typename TT_DATA, unsigned int TP_SIG_LEN>
INLINE_DECL constexpr bool fnCheckLenOfData() {
    return (isLenInRange<TT_DATA, TP_SIG_LEN>() ? true : false);
}

// Function which return true or false if Given Number is power of 2 or not
template <unsigned int TP_DATA>
INLINE_DECL constexpr bool isPowerOfTwo() {
    return (((TP_DATA) && !(TP_DATA & (TP_DATA - 1))) ? true : false);
};

// Function which return true or false if Given Dimension is less than or equal to 4
template <unsigned int TP_DIM>
INLINE_DECL constexpr bool fnCheckforDimension() {
    return ((TP_DIM <= kFixedDimOfED) ? true : false);
};
} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_TRAITS_HPP_