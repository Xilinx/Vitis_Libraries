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
#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_REF_UTILS_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_REF_UTILS_HPP_

/*
    EUCLIDEAN_DISTANCE reference model utility helpers
*/

#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <type_traits>
#include <adf.h>
#include "device_defs.h"
#include "fir_ref_utils.hpp"

using namespace adf;
using namespace std;

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

#define SAT_MODE_MAX 3
#define SAT_MODE_MIN 0

namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

static constexpr unsigned int kMaxSaturationMode = 3;          // Max supported SAT Mode.
static constexpr unsigned int kMinSaturationMode = 0;          // Min supported SAT Mode.
static constexpr unsigned int kUnSuppportedSaturationMode = 2; // Max number of bits on X Register.
static constexpr unsigned int kFixedDimOfED = 4;               // Max number of bits on W Register.

// Return type of default accumulator used in euclidean_distance
template <typename TT_DATA>
struct t_ED_RefAccType {
    using type = float;
};

// float
template <>
struct t_ED_RefAccType<float> {
    using type = float;
};

// bfloat16
template <>
struct t_ED_RefAccType<bfloat16> {
    using type = float;
};

template <typename T_D>
using t_ED_RefAccType_t = typename t_ED_RefAccType<T_D>::type;

template <typename TT_DATA>
INLINE_DECL constexpr bool fnCheckDType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDType<float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDType<cfloat>() {
    return true;
};

// Function to return the size of Y Reg
static constexpr unsigned MAX_WIDTH_Y_REG() {
    return 1024;
};

// Function to return the size of X Reg
static constexpr unsigned MAX_WIDTH_X_REG() {
    return 512;
};

// Function to return the size of bits to load on AIE
static constexpr unsigned maxBitsLoadOnAie() {
    return 256;
};

// Function to return the size of bits to load on AIE
static constexpr unsigned MAX_BUFFER_LEN_ON_AIE_IN_BITS() {
    return 65536;
}; // 8192bytes --> (8192*8) bits

// Lanes, Muls/Macs and data load
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int ref_ED_NumLanes() {
    return 32; // defualt return for AIE2
};
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int ref_ED_NumMuls() {
    return 256; // defualt return for AIE2
};

template <typename TT_DATA>
INLINE_DECL constexpr unsigned int ref_ED_Size() {
    return 32; // defualt multiple factor is 32 for both AIE-1 and AIE-2
};

// for io buffer cases
// function to return the number of acc's lanes for a type combo
// AIE-1
#if (__HAS_ACCUM_PERMUTES__ == 1)
template <>
INLINE_DECL constexpr unsigned int ref_ED_NumLanes<float>() {
    return 8;
}; //

// function to return the number of multiplications for a type combo
template <>
INLINE_DECL constexpr unsigned int ref_ED_NumMuls<float>() {
    return 8;
}; //
#endif

// AIE-2
#if (__HAS_ACCUM_PERMUTES__ == 0)

template <>
INLINE_DECL constexpr unsigned int ref_ED_NumLanes<float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_ED_NumLanes<bfloat16>() {
    return 16;
}; //

// function to return the number of multiplications for a type combo

template <>
INLINE_DECL constexpr unsigned int ref_ED_NumMuls<float>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int ref_ED_NumMuls<bfloat16>() {
    return 16;
}; //
#endif

// function to return number of samples for given data type
template <>
INLINE_DECL constexpr unsigned int ref_ED_Size<float>() {
    return 32;
}; //

#ifdef _SUPPORTS_BFLOAT16_
template <>
INLINE_DECL constexpr unsigned int ref_ED_Size<bfloat16>() {
    return 16;
}; //
#endif

// Function to return Maximum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMaxLen() {
    return ((MAX_BUFFER_LEN_ON_AIE_IN_BITS() / ref_ED_Size<TT_DATA>()));
};

// Function to return Minimum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMinLen() {
    return (((maxBitsLoadOnAie() << 1) / ref_ED_Size<TT_DATA>()));
};

// Function to return true or false by checking given length is in range or not
template <typename T_DATA, unsigned int TP_SIG_LEN>
constexpr bool isLenInRange() {
    unsigned int minDataLoad = (maxBitsLoadOnAie() / ref_ED_Size<T_DATA>());
    bool checkLen = false;

    if ((TP_SIG_LEN >= getMinLen<T_DATA>()) && (TP_SIG_LEN <= getMaxLen<T_DATA>())) {
        checkLen = (((TP_SIG_LEN % minDataLoad) == 0) ? true : false);
    }
    return checkLen;
};

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

// Function which return true or false if Given Dimension is less than or equal to 4
template <unsigned int TP_DIM>
INLINE_DECL constexpr bool fnCheckforDimension() {
    return ((TP_DIM <= 4) ? true : false);
};

} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_REF_UTILS_HPP_