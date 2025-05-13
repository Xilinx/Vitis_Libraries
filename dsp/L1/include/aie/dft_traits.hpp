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
#ifndef _DSPLIB_dft_TRAITS_HPP_
#define _DSPLIB_dft_TRAITS_HPP_

#include "device_defs.h"

#define TRUNC(x, y) (((x) / y) * y)

/*
DFT traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace dft {

//---------------------------------------
// Configuration Defensive check functions
template <typename TT_DATA>
INLINE_DECL constexpr bool fnCheckDataType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<int16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<int32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<cfloat>() {
    return true;
};

template <typename TT_IN_DATA, typename TT_OUT_DATA>
INLINE_DECL constexpr bool fnCheckDataIOType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cint16, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cint32, cint32>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataIOType<cfloat, cfloat>() {
    return true;
};

template <typename TT_TWIDDLE>
INLINE_DECL constexpr bool fnCheckTwiddleType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckTwiddleType<cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckTwiddleType<cfloat>() {
    return true;
};

template <typename TT_DATA, typename TT_TWIDDLE>
INLINE_DECL constexpr bool fnCheckDataTwiddleType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<int16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<int32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<float, cfloat>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cint16, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cint32, cint16>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataTwiddleType<cfloat, cfloat>() {
    return true;
};

template <unsigned int TP_POINT_SIZE>
INLINE_DECL constexpr bool fnCheckPointSize() {
    return (TP_POINT_SIZE >= 4 && TP_POINT_SIZE <= 128);
};

template <unsigned int TP_SHIFT>
INLINE_DECL constexpr bool fnCheckShift() {
    return (TP_SHIFT >= 0) && (TP_SHIFT <= 60);
};

template <typename TT_DATA, unsigned int TP_SHIFT>
INLINE_DECL constexpr bool fnCheckShiftFloat() {
    return !(std::is_same<TT_DATA, cfloat>::value) || // This check traps shift != 0 when data = cfloat
           (TP_SHIFT == 0);
};

// End of Defensive check functions

//---------------------------------------------------
// Functions

template <int T_X, int T_Y>
INLINE_DECL constexpr int fnCeil() {
    return ((T_X + T_Y - 1) / T_Y) * T_Y;
};

//----------------------------------------------------------------------
// nullElem
template <typename T_RDATA>
INLINE_DECL T_RDATA nullElem() {
    return 0;
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
template <>
INLINE_DECL cfloat nullElem() {
    cfloat retVal;

    retVal.real = 0.0;
    retVal.imag = 0.0;
    return retVal;
};
}
}
}
}
}

#endif // _DSPLIB_dft_TRAITS_HPP_
