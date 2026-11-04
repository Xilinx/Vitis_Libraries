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
#ifndef _DSPLIB_TRANSPOSE_TRAITS_HPP_
#define _DSPLIB_TRANSPOSE_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernel class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include <type_traits>
#include <adf.h>
#include "device_defs.h"

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

namespace xf {
namespace dsp {
namespace aie {
namespace transpose {

template <typename TT_DATA>
INLINE_DECL constexpr bool fnIsComplex() {
    return std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value ||
           std::is_same<TT_DATA, cfloat>::value;
}

// Largest power-of-two tile edge R such that a single RxR tile of TT_DATA fits within one
// 1024-bit vector register (the widest register used elsewhere in this library, e.g.
// matrix_mult_tiler.cpp's __AIE_API_USE_NATIVE_1024B_VECTOR__). This is the granularity at
// which ::aie::transpose() is applied by the kernel, independent of (and always a divisor of)
// the memory-access-width padding granularity from device_defs.h.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnTposeTileDim() {
    unsigned int r = 1;
    while ((r * 2) * (r * 2) * sizeof(TT_DATA) <= 128) {
        r *= 2;
    }
    return r;
}
}
}
}
}

#endif // _DSPLIB_TRANSPOSE_TRAITS_HPP_
