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
#ifndef _SOLVERLIB_QRD_HH_TRAITS_HPP_
#define _SOLVERLIB_QRD_HH_TRAITS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions which
encapsulate properties of the intrinsics used by the main kernal class. Specifically,
this file does not contain any vector types or intrinsics since it is required for construction
and therefore must be suitable for the aie compiler graph-level compilation.
*/

#include <array>
#include "device_defs.h"
#include "fir_utils.hpp"
#include "single_mul_out_types.hpp"
#include "single_mul_acc_types.hpp"

using namespace ::xf::dsp::aie;

namespace xf {
namespace solver {
namespace aie {
namespace qrd_hh {

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

template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS, unsigned int TP_NUM_FRAMES>
INLINE_DECL constexpr bool fnCheckBufferSize() {
    if ((TP_DIM_COLS * TP_DIM_ROWS * TP_NUM_FRAMES * sizeof(TT_DATA) <= kMaxBufferLenInBytes) &&
        (TP_DIM_COLS * TP_DIM_COLS * sizeof(TT_DATA) <= kMaxBufferLenInBytes)) {
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
    if (TP_DIM >= (kMaxReadInBytes / sizeof(TT_DATA))) {
        return true;
    }
    return false;
};

template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_CASC_LEN>
INLINE_DECL constexpr bool fnCheckKernelLoadShare() {
    unsigned int rowchunks = TP_DIM_ROWS / (kMaxReadInBytes / sizeof(TT_DATA));
    if (rowchunks % TP_CASC_LEN ==
        0) { // rowchuncks must be divisible by cascade length to ensure each kernel can have equal load
        return true;
    }
    return false;
};

template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
INLINE_DECL constexpr bool fnCheckMaxSize() {
    unsigned int buffer_size_in_bytes = TP_DIM_COLS * (TP_DIM_ROWS / TP_CASC_LEN) * TP_NUM_FRAMES * sizeof(TT_DATA);

    if (buffer_size_in_bytes <= (kMaxBufferLenInBytes)) {
        return true;
    }
    return false;
};

template <typename TT_DATA, unsigned int TP_DIM_A_LEADING, unsigned int TP_DIM_Q_LEADING, unsigned int TP_DIM_R_LEADING>
INLINE_DECL constexpr bool fnCheckTiling() {
    if ((TP_DIM_A_LEADING == 1) || (TP_DIM_Q_LEADING == 1) || (TP_DIM_R_LEADING == 1)) {
        if (sizeof(TT_DATA) == 8) { // currently disabling for all 64 bits data
            return false;
        }
    }
    return true;
}

template <bool T_STREAM_EN, typename T_DATA>
struct T_inputIF {
    input_buffer<T_DATA>& __restrict inWindowA;
    T_inputIF(input_buffer<T_DATA>& winA) : inWindowA(winA) {}
};

template <typename T_DATA>
struct T_inputIF<true, T_DATA> {
    input_buffer<T_DATA>& __restrict inWindowA;
    input_stream<T_DATA>* __restrict inStream;
    T_inputIF(input_buffer<T_DATA>& winA) : inWindowA(winA) {}
};

template <bool T_STREAM_EN, bool T_ROUT_EN, typename T_DATA>
struct T_outputIF {
    output_buffer<T_DATA>& __restrict outWindowQ;
    T_outputIF(output_buffer<T_DATA>& winQ) : outWindowQ(winQ) {}
};

template <typename T_DATA>
struct T_outputIF<false, true, T_DATA> {
    output_buffer<T_DATA>& __restrict outWindowQ;
    output_buffer<T_DATA>& __restrict outWindowR;
    T_outputIF(output_buffer<T_DATA>& winQ, output_buffer<T_DATA>& winR) : outWindowQ(winQ), outWindowR(winR) {}
};

template <typename T_DATA>
struct T_outputIF<true, true, T_DATA> {
    output_buffer<T_DATA>& __restrict outWindowQ;
    output_buffer<T_DATA>& __restrict outWindowR;
    output_stream<T_DATA>* __restrict outStream;
    T_outputIF(output_buffer<T_DATA>& winQ, output_buffer<T_DATA>& winR) : outWindowQ(winQ), outWindowR(winR) {}
};

template <typename T_DATA>
struct T_outputIF<true, false, T_DATA> {
    output_buffer<T_DATA>& __restrict outWindowQ;
    output_stream<T_DATA>* __restrict outStream;
    T_outputIF(output_buffer<T_DATA>& winQ) : outWindowQ(winQ) {}
};

// constant 0 is used for the masking of the V vectors.
// constant 1 is used to set up the I matrix as an initial condition for Q matrix.
// constant 2 is used for beta calculation where the norm of the Householder column is multiplied by 2 to calculate beta

template <typename T_DATA>
struct constVals {
    static constexpr T_DATA c0 = T_DATA(0.0);
    static constexpr T_DATA c1 = T_DATA(1.0);
    static constexpr T_DATA c2 = T_DATA(2.0);
};

template <>
struct constVals<cfloat> {
    static constexpr cfloat c0 = {0.0, 0.0};
    static constexpr cfloat c1 = {1.0, 0.0};
    static constexpr cfloat c2 = {2.0, 0.0};
};

template <int N>
struct KernelRoleArrays {
    std::array<unsigned int, N> kernel_role_arr;
    std::array<unsigned int, N> col_chunk_in_calc_arr;
};

template <int m_kColChunkNum,
          int m_kRowChunkNum,
          int m_kColChunkLeadingStart,
          int m_kColChunkLeadingEnd,
          int m_kColChunkFinalStart,
          int TP_KERNEL_POS,
          int TP_CASC_LEN>
constexpr KernelRoleArrays<m_kColChunkNum> det_kernel_roles_r() {
    KernelRoleArrays<m_kColChunkNum> result = {};

    for (int col_chunk = 0; col_chunk < m_kColChunkNum; col_chunk++) {
        if (col_chunk < m_kColChunkLeadingStart - m_kRowChunkNum) {
            result.col_chunk_in_calc_arr[col_chunk] = 0;
            result.kernel_role_arr[col_chunk] = 0;
        } else if ((col_chunk >= m_kColChunkLeadingStart - m_kRowChunkNum) && (col_chunk < m_kColChunkLeadingStart)) {
            result.col_chunk_in_calc_arr[col_chunk] = 0;
            result.kernel_role_arr[col_chunk] = 2;
        } else if ((col_chunk >= m_kColChunkLeadingStart) && (col_chunk <= m_kColChunkLeadingEnd)) {
            result.col_chunk_in_calc_arr[col_chunk] = col_chunk - m_kColChunkLeadingStart;
            result.kernel_role_arr[col_chunk] = 1;
        } else if ((col_chunk >= m_kColChunkFinalStart) && (TP_KERNEL_POS != TP_CASC_LEN - 1)) {
            result.col_chunk_in_calc_arr[col_chunk] = m_kRowChunkNum;
            result.kernel_role_arr[col_chunk] = 3;
        } else if (col_chunk > m_kColChunkLeadingEnd) {
            result.col_chunk_in_calc_arr[col_chunk] = m_kRowChunkNum;
            result.kernel_role_arr[col_chunk] = 4;
        }
    }
    return result;
}

template <int m_kColChunkNum,
          int m_kRowChunkNum,
          int m_kColChunkLeadingStart,
          int m_kColChunkLeadingEnd,
          int m_kColChunkFinalStart,
          int TP_KERNEL_POS,
          int TP_CASC_LEN>
constexpr KernelRoleArrays<m_kColChunkNum> det_kernel_roles_q() {
    KernelRoleArrays<m_kColChunkNum> result = {};

    for (int col_chunk = 0; col_chunk < m_kColChunkNum; col_chunk++) {
        if (col_chunk < m_kColChunkLeadingStart -
                            m_kRowChunkNum) { // upper kernels are calculating the beta, keep working all the columns
            result.col_chunk_in_calc_arr[col_chunk] = 0;
            result.kernel_role_arr[col_chunk] = 0;
        } else if ((col_chunk >= m_kColChunkLeadingStart - m_kRowChunkNum) &&
                   (col_chunk < m_kColChunkLeadingStart)) { // the kernel after the leading
            result.col_chunk_in_calc_arr[col_chunk] = 0;
            result.kernel_role_arr[col_chunk] = 2;
        } else if ((col_chunk >= m_kColChunkLeadingStart) &&
                   (col_chunk <= m_kColChunkLeadingEnd)) { // kernel is the leading kernel
            result.col_chunk_in_calc_arr[col_chunk] = col_chunk - m_kColChunkLeadingStart;
            result.kernel_role_arr[col_chunk] = 1;
        } else if ((col_chunk >= m_kColChunkFinalStart) &&
                   (TP_KERNEL_POS != TP_CASC_LEN - 1)) { // final kernel is leading and this kernel is not last kernel
            result.col_chunk_in_calc_arr[col_chunk] = m_kRowChunkNum;
            result.kernel_role_arr[col_chunk] = 3;      // idle but no need to pass the beta
        } else if (col_chunk > m_kColChunkLeadingEnd) { // idle, circulate data only
            result.col_chunk_in_calc_arr[col_chunk] = m_kRowChunkNum;
            result.kernel_role_arr[col_chunk] = 4;
        }
    }
    return result;
}
}
}
}
}

#endif // _DSPLIB_QRD_TRAITS_HPP_
