/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_QRD_TRAITS_HPP_
#define _SOLVERLIB_QRD_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include "device_defs.h"
#include "fir_utils.hpp"
#include "single_mul_out_types.hpp"
#include "single_mul_acc_types.hpp"


using namespace ::xf::dsp::aie;

namespace xf {
namespace solver {
namespace aie {
namespace qrd {


static constexpr unsigned int kMaxBufferLenInBytes = __DATA_MEM_BYTES__; // Max buffer Length
static constexpr unsigned int kMaxReadInBytes = __MAX_READ_WRITE__ / 8;


template <typename TT_DATA>
INLINE_DECL constexpr bool fnCheckDataType() {
    return false;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<float>() {
    return true;
};
template <>
INLINE_DECL constexpr bool fnCheckDataType<cfloat>() {
    return true;
};

template <typename TT_DATA,  unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS, unsigned int TP_NUM_FRAMES>
INLINE_DECL constexpr bool fnCheckBufferSize() {
    if ((TP_DIM_COLS * TP_DIM_ROWS * TP_NUM_FRAMES * sizeof(TT_DATA) <= kMaxBufferLenInBytes) && (TP_DIM_COLS * TP_DIM_COLS * sizeof(TT_DATA) <= kMaxBufferLenInBytes)) {
        return true;
    }
    return false;
};

template <unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS>
INLINE_DECL constexpr bool fnCheckColvsRowSize() {
    if (TP_DIM_COLS <= TP_DIM_ROWS) {
        return true;
    }
    return false;
};

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL constexpr bool fnCheckChunkSize() {
    if (TP_DIM % (kMaxReadInBytes / sizeof(TT_DATA)) == 0) {
        return true;
    }
    return false;
};

template <typename TT_DATA, unsigned int TP_DIM>
INLINE_DECL constexpr bool fnCheckMinSize() {
    if  (TP_DIM >= (kMaxReadInBytes / sizeof(TT_DATA))) {
        return true;
    }
    return false;
};

template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS>
INLINE_DECL constexpr bool fnCheckKernelLoadMax(unsigned int col_dim_per_kernel) {

    unsigned int max_col_per_kernel_q = __DATA_MEM_BYTES__/(TP_DIM_ROWS * sizeof(TT_DATA));
    unsigned int max_col_per_kernel_r = __DATA_MEM_BYTES__/(TP_DIM_COLS * sizeof(TT_DATA));
    unsigned int max_col_per_kernel = (max_col_per_kernel_q < max_col_per_kernel_r) ? max_col_per_kernel_r : max_col_per_kernel_q;

    if (col_dim_per_kernel > max_col_per_kernel) {
        return false;
    } else {
        return true;
    }
}

INLINE_DECL constexpr bool fnCheckKernelLoadMin(unsigned int col_dim_per_kernel) {

    unsigned int min_col_per_kernel =  1;
    if (col_dim_per_kernel < min_col_per_kernel) {
        return false;
    } else {
        return true;

    }
}

template <unsigned int TP_DIM_COLS>
INLINE_DECL constexpr bool fnCheckDistributedLoad(unsigned int col_dist) {

    if (col_dist == TP_DIM_COLS) {
        return true;
    } else {
        return false;

    }
}

template <typename TT_DATA,
unsigned int TP_DIM_A_LEADING,
unsigned int TP_DIM_Q_LEADING,
unsigned int TP_DIM_R_LEADING>
INLINE_DECL constexpr bool fnCheckTiling() {
    //in 25.2 cfloat transpose will not be allowed for any AIE variants
    //aie1 is naturally not capable, aie2 and aie22 tests are not finishing, so disabling for now

    if ((TP_DIM_A_LEADING == 1) || (TP_DIM_Q_LEADING == 1) || (TP_DIM_R_LEADING == 1)){
        if (sizeof(TT_DATA) == 8) { //currently disabling for all 64 bits data
            return false;
        }
    }
    return true;

    // if ((TP_DIM_A_LEADING == 1) || (TP_DIM_Q_LEADING == 1) || (TP_DIM_R_LEADING == 1)){
    //     if (sizeof(TT_DATA) > __MAX_BD_DSIZE__) {
    //         return false;
    //     }
    // }
    // return true;
}


struct no_port {};

template <bool T_CASC_IN, typename TT_DATA>
struct T_inputIF {
    input_buffer<TT_DATA>& __restrict inWindowA;
    T_inputIF(input_buffer<TT_DATA>& winA) : inWindowA(winA) {}
    typename std::conditional<T_CASC_IN == CASC_IN_FALSE, no_port, input_cascade<TT_DATA> >::type* __restrict inCascade;
};

template <bool T_CASC_OUT, typename TT_DATA>
struct T_outputIF {
    output_buffer<TT_DATA>& __restrict outWindowQ;
    output_buffer<TT_DATA>& __restrict outWindowR;
    T_outputIF(output_buffer<TT_DATA>& winQ, output_buffer<TT_DATA>& winR)
        : outWindowQ(winQ), outWindowR(winR) {}
    typename std::conditional<T_CASC_OUT == CASC_OUT_FALSE, no_port, output_cascade<TT_DATA> >::type* __restrict outCascade;
};


}
}
}
}

#endif // _DSPLIB_QRD_TRAITS_HPP_
