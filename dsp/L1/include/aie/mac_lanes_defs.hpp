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

#ifndef _DSPLIB_MAC_LANES_DEFS_HPP_
#define _DSPLIB_MAC_LANES_DEFS_HPP_

/* This file exists to hold utility functions for kernels in general, not specific
   to one library element. Also, the functions in this file do not use vector types
   or intrinsics so are fit for use by aiecompiler and kernel constructors.
*/

#include <stdio.h>
#include <adf.h>
#include "device_defs.h"
#include "fir_params_defaults.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {

#if (__AIE_ARCH__ == 20)
// for io buffer cases
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnMacLanes() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint16, cint32>() {
    return 16; // 8 - API min native length is 8, 16 lanes offers better performance.
};             //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, int16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, int32>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, cint16>() {
    return 8;
}; //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, cint32>() {
    return 16;
}; //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<float, float>() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<bfloat16, bfloat16>() {
    return 16;
};

#elif (__AIE_ARCH__ == 21) || (__AIE_ARCH__ == 22)
// 32 x 4 for all cases that use 64-bit accs.
// 16, 32 x 1 for float cases.
// 32, 64 x 1 for bfloat16 cases.
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnMacLanes() {
    return 16;
};
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<int16, int16>() {
    return 32; // 32 x 4
};             //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<int16, int32>() {
    return 32; // 32 x 1
};             //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<int32, int16>() {
    return 32; // 32 x 1
};             //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<int32, int32>() {
    return 32; // 32 x 1
};             //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint16, int16>() {
    return 32; // 32 x 1
};             //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, int16>() {
    return 16; // 32 x 1
};             //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, int32>() {
    return 16; // 32 x 1
};             //
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<float, float>() {
#if (__AIE_ARCH__ == 21)
    return 32;
#endif
#if (__AIE_ARCH__ == 22)
    return 16;
#endif
};
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<bfloat16, bfloat16>() {
    return 16;
};

#else // #if (__AIE_ARCH__ == 10)

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL constexpr unsigned int fnMacLanes() {
    return 0;
};
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<int16, int16>() {
    return 16;
}; // 16x2
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint16, int16>() {
    return 8;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint16, cint16>() {
    return 8;
}; // 8x1
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<int32, int16>() {
    return 8;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<int32, int32>() {
    return 8;
}; // 8x1
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, int16>() {
    return 4;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, cint16>() {
    return 4;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, int32>() {
    return 4;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint32, cint32>() {
    return 2;
}; // 2x1
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<int16, int32>() {
    return 8;
}; // 8x2
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint16, int32>() {
    return 4;
}; // 4x2
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cint16, cint32>() {
    return 4;
}; // 4x1
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<float, float>() {
    return 8;
};
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cfloat, float>() {
    return 4;
};
template <>
INLINE_DECL constexpr unsigned int fnMacLanes<cfloat, cfloat>() {
    return 4;
};

#endif
}
}
}

#endif // _DSPLIB_MAC_LANES_DEFS_HPP_