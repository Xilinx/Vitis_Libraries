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
#ifndef _DSPLIB_FFT_WINDOW_TRAITS_HPP_
#define _DSPLIB_FFT_WINDOW_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace windowfn {

static constexpr unsigned int kPointSizeMin = 16;
static constexpr unsigned int kPointSizeMax = 65536;

static constexpr unsigned int kMinPtSizePwr = 4;
static constexpr unsigned int kMaxPtSizePwr = 16;

template <int T_SSR>
INLINE_DECL constexpr unsigned int fnLogSSR() {
    return -1;
};
template <>
INLINE_DECL constexpr unsigned int fnLogSSR<1>() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnLogSSR<2>() {
    return 1;
};
template <>
INLINE_DECL constexpr unsigned int fnLogSSR<4>() {
    return 2;
};
template <>
INLINE_DECL constexpr unsigned int fnLogSSR<8>() {
    return 3;
};
template <>
INLINE_DECL constexpr unsigned int fnLogSSR<16>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnLogSSR<32>() {
    return 5;
};

template <unsigned int TP_POINT_SIZE>
INLINE_DECL constexpr int fnPointSizePower() {
    return 0;
};
template <>
INLINE_DECL constexpr int fnPointSizePower<16>() {
    return 4;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<32>() {
    return 5;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<64>() {
    return 6;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<128>() {
    return 7;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<256>() {
    return 8;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<512>() {
    return 9;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<1024>() {
    return 10;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<2048>() {
    return 11;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<4096>() {
    return 12;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<8192>() {
    return 13;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<16384>() {
    return 14;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<32768>() {
    return 15;
}
template <>
INLINE_DECL constexpr int fnPointSizePower<65536>() {
    return 16;
}
}
}
}
}
}
#endif // _DSPLIB_FFT_WINDOW_TRAITS_HPP_
