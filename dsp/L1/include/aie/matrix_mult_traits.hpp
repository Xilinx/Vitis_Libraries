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
#ifndef _DSPLIB_MATRIX_MULT_TRAITS_HPP_
#define _DSPLIB_MATRIX_MULT_TRAITS_HPP_

#include "device_defs.h"
#include "promotions_mac.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace blas {
namespace matrix_mult {
/*
Matrix Multiply traits.
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

using namespace ::xf::dsp::aie::promotions_mac;

struct no_port {};

// Input and Output Interfaces - T_inputIF and T_outputIF
template <bool T_CASC_IN, typename T_D_A, typename T_D_B>
struct T_inputIF {
    void* __restrict inWindowA;
    void* __restrict inWindowB;
    typename std::conditional<T_CASC_IN == CASC_IN_FALSE, no_port, input_cascade<accType_t<T_D_A, T_D_B> > >::type*
        inCascade;
};

// IF output type
template <bool T_CASC_OUT, typename T_D_A, typename T_D_B, typename T_D_O>
struct T_outputIF {
    typename std::conditional<T_CASC_OUT == CASC_OUT_FALSE, T_D_O, output_cascade<accType_t<T_D_A, T_D_B> > >::
        type* __restrict outWindow;
};

// Function to return Maximum supported length based on given DATA TYPE.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int getMaxLen() {
    return (__DATA_MEM_BYTES__ / sizeof(TT_DATA));
};
}
}
}
}
}

#endif // _DSPLIB_MATRIX_MULT_TRAITS_HPP_
