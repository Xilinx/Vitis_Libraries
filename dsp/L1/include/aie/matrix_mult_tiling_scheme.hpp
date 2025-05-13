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
#ifndef MATRIX_MULT_TILING_SCHEME_HPP
#define MATRIX_MULT_TILING_SCHEME_HPP
/*
Matrix Multiply Definition

The file holds the definition of the Matrix Multiply kernel class.

*/

/* Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>

#include <assert.h>
#include <array>
#include <cstdint>
#include <type_traits>

#include "device_defs.h"

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {

struct tilingStruct {
    unsigned int Atile;
    unsigned int ABtile;
    unsigned int Btile;
};

//-----------------------------------------------------------------------------------------------------
template <typename A, typename B>
INLINE_DECL constexpr tilingStruct fnTilingScheme() {
#if (__AIE_ARCH__ == 20)
    // int16 or int32 x int16 x int 32
    if ((std::is_same<A, int16>::value || std::is_same<A, int32>::value) &&
        (std::is_same<B, int16>::value || std::is_same<B, int32>::value)) {
        return {4, 4, 4};
    }
    // cint16 x int16
    else if (std::is_same<A, cint16>::value && std::is_same<B, int16>::value) {
        return {4, 4, 4};
    }
    // cint16 x cint16
    else if (std::is_same<A, cint16>::value && std::is_same<B, cint16>::value) {
        return {1, 4, 8};
    }
    // cint32 x cint16
    else if (std::is_same<A, cint32>::value && std::is_same<B, cint16>::value) {
        return {2, 4, 8};
    }
    // cint32 x cint32
    else if (std::is_same<A, cint32>::value && std::is_same<B, cint32>::value) {
        return {1, 2, 8};
    }
    // All other combinations are not supported
    else {
        return {1, 1, 1};
    }
#elif (__AIE_ARCH__ == 22)
    if (std::is_same<A, int8>::value && std::is_same<B, int8>::value) {
        return {4, 8, 8};
    } else if (std::is_same<A, int16>::value && std::is_same<B, int8>::value) {
        return {4, 4, 8};
    } else if (std::is_same<A, int16>::value && std::is_same<B, int16>::value) {
        return {4, 4, 8};
    } else if (std::is_same<A, int32>::value && std::is_same<B, int16>::value) {
        return {4, 2, 8};
        // return {4, 4, 8};
    } else if (std::is_same<A, int16>::value && std::is_same<B, int32>::value) {
        return {4, 4, 8};
    } else if (std::is_same<A, int32>::value && std::is_same<B, int32>::value) {
        return {4, 2, 8};
    } else if (std::is_same<A, bfloat16>::value && std::is_same<B, bfloat16>::value) {
        return {4, 8, 8};
    } else if (std::is_same<A, float>::value && std::is_same<B, float>::value) {
        return {4, 8, 4};
    } else if (std::is_same<A, cint16>::value && std::is_same<B, int16>::value) {
        return {4, 4, 8};
    } else if (std::is_same<A, cint16>::value && std::is_same<B, cint16>::value) {
        return {1, 4, 8};
        // return {2, 2, 16};
    } else if (std::is_same<A, cint32>::value && std::is_same<B, cint16>::value) {
        // return {1, 2, 4};
        return {1, 2, 8};
        // return {1, 2, 16};
    } else if (std::is_same<A, cint32>::value && std::is_same<B, cint32>::value) {
        return {1, 2, 8};
    } else {
        return {1, 1, 1};
    }
#else
    // #if (__AIE_ARCH__ == 10)
    // 16b x 16b
    if (std::is_same<A, int16>::value && std::is_same<B, int16>::value) {
        return {4, 4, 4};
    }
    // 32b x 16b
    if ((std::is_same<A, cint16>::value || std::is_same<A, int32>::value) && std::is_same<B, int16>::value) {
        return {4, 4, 2};
    }
    // 16b x 32b
    if (std::is_same<A, int16>::value && (std::is_same<B, cint16>::value || std::is_same<B, int32>::value)) {
        return {4, 2, 2};
    }
    // 32b x 32b
    if (((std::is_same<A, cint16>::value || std::is_same<A, int32>::value) &&
         (std::is_same<B, cint16>::value || std::is_same<B, int32>::value)) ||
        std::is_same<A, float>::value && std::is_same<B, float>::value) {
        return {4, 4, 2};
    }
    // 64b x 16b
    if (std::is_same<A, cint32>::value && std::is_same<B, int16>::value) {
        return {2, 4, 2};
    }
    // 16b x 64b
    if (std::is_same<A, int16>::value && std::is_same<B, cint32>::value) {
        return {2, 4, 2}; // 4, 4, 2 is also ok
    }
    // 64b x 32b
    if (std::is_same<A, cint32>::value && (std::is_same<B, cint16>::value || std::is_same<B, int32>::value)) {
        return {2, 2, 2}; // 2, 4, 2 is also ok
    }
    // 32b x 64b
    if ((std::is_same<A, cint16>::value || std::is_same<A, int32>::value) && std::is_same<B, cint32>::value) {
        return {2, 2, 2};
    }
    // 64b x 64b
    if (std::is_same<A, cint32>::value && std::is_same<B, cint32>::value) {
        return {2, 2, 2};
    }
    // Mixed Floats
    if ((std::is_same<A, cfloat>::value && std::is_same<B, float>::value) ||
        (std::is_same<A, float>::value && std::is_same<B, cfloat>::value)) {
        return {2, 4, 2}; // 2, 2, 2 is also ok
    }
    // cfloats
    if (std::is_same<A, cfloat>::value && std::is_same<B, cfloat>::value) {
        return {4, 2, 2};
    }
#endif
}; // getTilingScheme()
}
}
}
}
}

#endif // MATRIX_MULT_TILING_SCHEME_HPP
