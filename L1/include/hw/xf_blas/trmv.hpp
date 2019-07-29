/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef XF_BLAS_TRMV_HPP
#define XF_BLAS_TRMV_HPP

#ifndef __cplusplus
#error "BLAS Library only works with C++."
#endif

#include "ap_int.h"
#include "hls_stream.h"
#include "xf_blas/helpers.hpp"

namespace xf {
namespace linear_algebra {
namespace blas {

/**
 * @brief trmv function performs general banded matrix-vector multiplication
 *
 * @tparam t_DataType the data type of the vector entries
 * @tparam t_ParEntries the number of parallelly processed entries in the input
 * vector
 * @tparam t_MacType the datawidth of the datatype t_DataType of the input
 * vector
 * @tparam t_IndexType the datatype of the index
 *
 * @param p_n the number of entries in the input vector p_x
 * @param p_x the input stream of packed vector entries
 * @param p_sum the sum, which is 0 if p_n <= 0
 */

template <typename t_DataType,
          unsigned int t_ParEntries,
          typename t_IndexType = unsigned int,
          typename t_MacType = t_DataType>
void trmv(const bool mode,
          const unsigned int p_n,
          hls::stream<WideType<t_DataType, t_ParEntries> >& p_A,
          hls::stream<WideType<t_DataType, t_ParEntries> >& p_x,
          hls::stream<WideType<t_MacType, t_ParEntries> >& p_y) {
#pragma HLS DATAFLOW
    hls::stream<WideType<t_DataType, 1 << t_LogParEntries> > l_mulStr;
#pragma HLS DATA_PACK variable = l_mulStr
    if (mode) {
        for (t_IndexType i = 1; i <= p_n; i++) {
            const unsigned int l_n = ((i + t_ParEntries - 1) / t_ParEntries) * t_ParEntries;
            mul<t_DataType, 1 << t_LogParEntries, t_IndexType>(l_n, p_M, p_x, l_mulStr);
            sum<t_DataType, t_LogParEntries, t_IndexType>(l_n, l_mulStr, p_y);
        }
    } else {
        for (t_IndexType i = p_n; i > 0; i--) {
            const unsigned int l_n = ((i + t_ParEntries - 1) / t_ParEntries) * t_ParEntries;
            mul<t_DataType, 1 << t_LogParEntries, t_IndexType>(l_n, p_M, p_x, l_mulStr);
            sum<t_DataType, t_LogParEntries, t_IndexType>(l_n, l_mulStr, p_y);
        }
    }
}

} // end namespace blas
} // end namspace linear_algebra
} // end namespace xf

#endif
