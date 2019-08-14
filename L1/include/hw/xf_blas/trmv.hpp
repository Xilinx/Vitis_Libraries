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
#include "scal.hpp"
#include "axpy.hpp"

namespace xf {
namespace linear_algebra {
namespace blas {

/**
 * @brief trmv function that returns the result vector of the mutiplication of a
 * triangular matrix and a vector y = M * x
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
          unsigned int t_LogParEntries,
          typename t_IndexType = unsigned int,
          typename t_MacType = t_DataType>
void trmv(const bool uplo,
          const unsigned int p_n,
          hls::stream<WideType<t_DataType, 1 << t_LogParEntries> >& p_M,
          hls::stream<WideType<t_DataType, 1 << t_LogParEntries> >& p_x,
          hls::stream<WideType<t_MacType, 1> >& p_y) {
#pragma HLS DATAFLOW
    hls::stream<WideType<t_DataType, 1 << t_LogParEntries> > l_mulStr;
#pragma HLS DATA_PACK variable = l_mulStr
#pragma HLS STREAM variable = l_mulStr depth = 2
    const unsigned int l_parEntries = 1 << t_LogParEntries;
    const unsigned int l_blocks = p_n >> t_LogParEntries;
    if (uplo) {
        for (t_IndexType i = 0; i < l_blocks; i++) {
            const unsigned int l_n = (l_blocks - i) << t_LogParEntries;
            mul<t_DataType, 1 << t_LogParEntries, t_IndexType>(l_n, p_M, p_x, l_mulStr, l_parEntries);
            sum<t_DataType, t_LogParEntries, t_IndexType>(l_n, l_mulStr, p_y, l_parEntries);
        }
    } else {
        for (t_IndexType i = l_blocks; i > 0; i--) {
            const unsigned int l_n = (l_blocks + 1 - i) << t_LogParEntries;
            mul<t_DataType, 1 << t_LogParEntries, t_IndexType>(l_n, p_M, p_x, l_mulStr, l_parEntries);
            sum<t_DataType, t_LogParEntries, t_IndexType>(l_n, l_mulStr, p_y, l_parEntries);
        }
    }
}

/**
 * @trmv function that returns the result vector of the mutiplication of a
 * triangular matrix and a vector y = alpha * M * x + beta * y
 *
 * @tparam t_DataType the data type of the vector entries
 * @tparam t_LogParEntries log2 of the number of parallelly processed entries in the input vector
 * @tparam t_IndexType the datatype of the index
 *
 * @param p_n the number of cols of input matrix p_M, as well as the number of entries in the input vector p_x, p_n %
 * l_ParEntries == 0
 * @param p_alpha, scalar alpha
 * @param p_M the input stream of packed Matrix entries
 * @param p_x the input stream of packed vector entries
 * @param p_beta, scalar beta
 * @param p_y the output vector
 */
template <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
void trmv(const bool uplo,
          const unsigned int p_n,
          const t_DataType p_alpha,
          hls::stream<WideType<t_DataType, 1 << t_LogParEntries> >& p_M,
          hls::stream<WideType<t_DataType, 1 << t_LogParEntries> >& p_x,
          const t_DataType p_beta,
          hls::stream<WideType<t_DataType, 1> >& p_y,
          hls::stream<WideType<t_DataType, 1> >& p_yr) {
#pragma HLS data_pack variable = p_M
#pragma HLS data_pack variable = p_x
#pragma HLS data_pack variable = p_y
#pragma HLS data_pack variable = p_yr
#ifndef __SYNTHESIS__
    assert(p_n % (1 << t_LogParEntries) == 0);
#endif
    const unsigned int l_numIter = p_n >> t_LogParEntries;
    hls::stream<WideType<t_DataType, 1> > l_x, l_y;
#pragma HLS DATAFLOW
    trmv<t_DataType, t_LogParEntries, t_IndexType>(uplo, p_n, p_M, p_x, l_x);
    scal<t_DataType, 1, t_IndexType>(p_n, p_beta, p_y, l_y);
    axpy<t_DataType, 1, t_IndexType>(p_n, p_alpha, l_x, l_y, p_yr);
}

} // end namespace blas
} // namespace linear_algebra
} // end namespace xf

#endif
