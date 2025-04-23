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
static constexpr unsigned int kBuffSize16Byte = 16;                           // 128-bit buffer size in Bytes

static constexpr unsigned int kUnrollFactor = 8;      // Unroll Factor
static constexpr unsigned int kFixedDimOfED = 4;      // Unroll Factor
static constexpr unsigned int kVecSize16OfBf16 = 16;  // v16bfloat16
static constexpr unsigned int kVecSize32OfBf16 = 32;  // v32bfloat16
static constexpr unsigned int kVecSizeOfAccum16 = 16; // v16accfloat
static constexpr unsigned int kVecSizeOfAccum32 = 32; // v32accfloat
static constexpr unsigned int kUpshiftFactor2 = 2;    // v32accfloat

// Function to return the default lanes based on choosen architecture
template <typename TT_DATA_P, typename TT_DATA_Q>
INLINE_DECL constexpr unsigned int fnEDNumLanes() {
    return 16; // defualt return for AIE2
};

// Function to return the default MULS/MACS based on choosen architecture
template <typename TT_DATA_P, typename TT_DATA_Q>
INLINE_DECL constexpr unsigned int fnEDNumMuls() {
    return 256; // defualt return for AIE2
};

// Function to return the default sample size of P data
template <typename TT_DATA_P>
INLINE_DECL constexpr unsigned int fnSampleSizeOfpointP() {
    return 8; // defualt sample size is 8
};

// Function to return the default sample size of Q Data
template <typename TT_DATA_Q>
INLINE_DECL constexpr unsigned int fnSampleSizeOfpointQ() {
    return 8; // defualt sample size is 8
};

// for io buffer cases
// function to return the number of acc's lanes for a type combo
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr unsigned int fnEDNumLanes<float, float>() {
    return 8;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int fnEDNumMuls<float, float>() {
    return 8;
}; //
#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
// AIE-2
template <>
INLINE_DECL constexpr unsigned int fnEDNumLanes<float, float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnEDNumLanes<bfloat16, bfloat16>() {
    return 16;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int fnEDNumMuls<float, float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnEDNumMuls<bfloat16, bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type of P Data
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfpointP<float>() {
    return 32;
}; //

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfpointP<bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type of Q Data
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfpointQ<float>() {
    return 32;
}; //

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr unsigned int fnSampleSizeOfpointQ<bfloat16>() {
    return 16;
}; //
#endif

// Function returns number of columns used by MUL/MACs in Euclidean Distance
template <typename TT_DATA_P, typename TT_DATA_Q>
INLINE_DECL constexpr unsigned int fnGetNumofMULs() {
    return fnEDNumMuls<TT_DATA_P, TT_DATA_Q>();
};

// Function to return the number of lanes based on given data types
template <typename TT_DATA_P, typename TT_DATA_Q>
INLINE_DECL constexpr unsigned int fnGetNumofLanes() {
    return fnEDNumLanes<TT_DATA_P, TT_DATA_Q>();
};

// Function to return the number of points based on given data types
template <typename TT_DATA_P, typename TT_DATA_Q>
INLINE_DECL constexpr unsigned int fnGetNumofPoints() {
    return (fnEDNumMuls<TT_DATA_P, TT_DATA_Q>() / fnEDNumLanes<TT_DATA_P, TT_DATA_Q>());
};

// Function to return the number of points based on given data types
template <typename TT_DATA_P>
INLINE_DECL constexpr unsigned int fnGetNumOfPdataSamples() {
    return fnSampleSizeOfpointP<TT_DATA_P>();
};

// Function to return the number of G Sig. samples to read based on given data types
template <typename TT_DATA_Q>
INLINE_DECL constexpr unsigned int fnGetNumOfQdataSamples() {
    return fnSampleSizeOfpointQ<TT_DATA_Q>();
};

// 8x8 16x8 16x16 32x16 c16x16 c16x32 c16xc16 c32x16 c32xc16
// Configuration Defensive check functions
//----------------------------------------------------------------------
// Check Output data type whether its complex if any input is complex
template <typename TT_DATA_P, typename TT_DATA_Q, typename TT_DATA_OUT>
INLINE_DECL constexpr bool fnCheckDataOutType() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, float, float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, float, cfloat>() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, cfloat, float>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, float, float>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, cfloat, float>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, cfloat, cfloat>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, float, cfloat>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<cfloat, cfloat, cfloat>() {
    return false;
};

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckDataOutType<bfloat16, bfloat16, bfloat16>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<bfloat16, bfloat16, float>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<bfloat16, float, float>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, bfloat16, float>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<float, bfloat16, bfloat16>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataOutType<bfloat16, float, bfloat16>() {
    return false;
};
#endif

template <typename TT_DATA_P, typename TT_DATA_Q>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs() {
    return false;
};
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float, float>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<bfloat16, bfloat16>() {
    return false;
};
#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<bfloat16, bfloat16>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float, float>() {
    return true;
};

template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<bfloat16, float>() {
    return false;
};

template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float, bfloat16>() {
    return false;
};
#endif

// Configuration Defensive check function to check TT_OUT is complex if any one input is complex
template <typename TT_DATA_P, typename TT_DATA_Q, typename TT_DATA_OUT>
INLINE_DECL constexpr bool fnCheckDataTypeOfOutput() {
    return (fnCheckDataOutType<TT_DATA_P, TT_DATA_Q, TT_DATA_OUT>() ? 1 : 0);
}

// Function which return true or false if Given Number is power of 2 or not
template <unsigned int TP_DATA>
INLINE_DECL constexpr bool isPowerOfTwo() {
    return (((TP_DATA) && !(TP_DATA & (TP_DATA - 1))) ? 1 : 0);
};

// Function which return true or false if Given Dimension is less than or equal to 4
template <unsigned int TP_DIM>
INLINE_DECL constexpr bool fnCheckforDimension() {
    return ((TP_DIM <= 4) ? 1 : 0);
};

} // namespace euclidean_distance {
} // namespace aie {
} // namespace dsp {
} // namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_TRAITS_HPP_