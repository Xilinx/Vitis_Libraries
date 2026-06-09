/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
    encapsulate properties of the intrinsics used by the main kernel class. Specifically,
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
#define MASK_LOWEST_BITS 0x00FFu
#define MASK_UPPER_BITS 0x7F00u
#define EXPONENT_ADJUSTMENT_CONSTANT 0x4000u
#define NUM_OF_PARALLEL_ACCESS_LUT 4

// using namespace std;
namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

// Constants for Euclidean Distance

static constexpr unsigned int kMaxBytesLoadOnAie =
    (__MAX_READ_WRITE__ / 8); // Max number of bytes per native AIE load (256/8=32 on AIE-ML, 512/8=64 on AIE-MLv2)
static constexpr unsigned int kMaxBufferLenOnAieInBytes = __DATA_MEM_BYTES__; // AIE data memory size in bytes
static constexpr unsigned int kWRegSizeBytes =
    (__W_REGSIZE__ / 8); // 256-bit AIE-1, AIE-ML and  AIE-MLv2 buffer size in Bytes
static constexpr unsigned int kXRegSizeBytes =
    (__X_REGSIZE__ / 8); // 512-bit AIE-1, AIE-ML and  AIE-MLv2 buffer size in Bytes
static constexpr unsigned int kYRegSizeBytes =
    (__Y_REGSIZE__ / 8); // 1024-bit AIE-1, AIE-ML and  AIE-MLv2 buffer size in Bytes

static constexpr unsigned int kUnrollFactor = 8; // Outer loop unroll factor for pipelining
static constexpr unsigned int kFixedDimOfED = 4; // Fixed spatial dimension: 4 coordinates per point (AOS layout)
static constexpr unsigned int kLutIndexUpshift =
    2; // Left-shift by 2 (×4) applied to mantissa bits when computing the bfloat16 LUT index
static constexpr unsigned int kNumOfKernels = 2; // Number of kernels in the ED graph (squared + sqrt)

// Function to return Maximum supported length based on given DATA TYPE.
// Max Length is being calculated regarding single buffer size
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMaxLen() {
    return (((kMaxBufferLenOnAieInBytes) / kFixedDimOfED) / sizeof(TT_DATA));
};

// Function to return Minimum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMinLen() {
    return ((kMaxBytesLoadOnAie / sizeof(TT_DATA)));
};

// Function to return true or false by checking given length is in range or not
template <typename TT_DATA, unsigned int sigLen>
constexpr bool isLenInRange() {
    unsigned int minDataLoad = (kMaxBytesLoadOnAie / sizeof(TT_DATA));
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
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float>() {
    return true;
};
#endif

#if (__HAS_ACCUM_PERMUTES__ == 0)
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTypesOfInputs<bfloat16>() {
    return true;
};
#endif
// Configuration Defensive check function to verify TP_LEN is within the valid range for P and Q input data.
// On AIE-ML/AIE-MLv2 with IS_OUTPUT_SQUARED=0, the LUT sqrt kernel processes 32 bfloat16 per iteration
// (one 512-bit X-register), so TP_LEN must additionally be a multiple of 32.
template <typename TT_DATA, unsigned int TP_SIG_LEN, unsigned int TP_IS_OUTPUT_SQUARED>
INLINE_DECL constexpr bool fnCheckLenOfData() {
    if (!isLenInRange<TT_DATA, TP_SIG_LEN>()) return false;
#if (__HAS_ACCUM_PERMUTES__ == 0)
    if (TP_IS_OUTPUT_SQUARED == 0) {
        // For sqrt output on AIE-ML/AIE-MLv2, TP_LEN must be a multiple of one X-register
        // worth of bfloat16 elements (kXRegSizeBytes / sizeof(bfloat16)).
        // A non-compliant TP_LEN produces zero output samples.
        constexpr unsigned int kLutSqrtGranularity = kXRegSizeBytes / sizeof(bfloat16);
        return (TP_SIG_LEN % kLutSqrtGranularity) == 0;
    }
#endif
    return true;
}

// Function which return true or false if Given Number is power of 2 or not
template <unsigned int TP_DATA>
INLINE_DECL constexpr bool isPowerOfTwo() {
    return (((TP_DATA) && !(TP_DATA & (TP_DATA - 1))) ? true : false);
};

// Returns true if TP_DIM is within the supported range [1, kFixedDimOfED]
template <unsigned int TP_DIM>
INLINE_DECL constexpr bool fnCheckforDimension() {
    return ((TP_DIM >= 1U && TP_DIM <= kFixedDimOfED) ? true : false);
};
} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_TRAITS_HPP_