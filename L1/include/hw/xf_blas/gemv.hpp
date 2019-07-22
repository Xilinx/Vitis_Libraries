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

#ifndef XF_BLAS_GEMV_HPP
#define XF_BLAS_GEMV_HPP

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
 * @brief gemv function that returns the result vector of the mutiplication of a
 * matrix and a vector y = M * x
 *
 * @tparam t_DataType the data type of the vector entries
 * @tparam t_LogParEntries log2 of the number of parallelly processed entries in the input vector
 * @tparam t_IndexType the datatype of the index
 *
 * @param p_m the number of rows of input matrix p_M
 * @param p_n the number of cols of input matrix p_M, as well as the number of entries in the input vector p_x, p_n %
 * l_ParEntries == 0
 * @param p_M the input stream of packed Matrix entries
 * @param p_x the input stream entries of the vector
 * @param p_y the output stream of packed vector
 */
template <typename t_DataType,
          unsigned int t_LogParEntries,
          unsigned int t_NumStreams = 1 << t_LogParEntries,
          typename t_IndexType = unsigned int>
void gemv(const unsigned int p_m,
          const unsigned int p_n,
          hls::stream<WideType<t_DataType, 1 << t_LogParEntries> > p_M[t_NumStreams],
          hls::stream<WideType<t_DataType, 1 << t_LogParEntries> > p_x[t_NumStreams],
          hls::stream<WideType<t_DataType, t_NumStreams> >& p_y) {
#pragma HLS data_pack variable = p_M
#pragma HLS data_pack variable = p_x
#pragma HLS data_pack variable = p_y
#ifndef __SYNTHESIS__
    assert(p_n % (1 << t_LogParEntries) == 0);
#endif
#pragma HLS DATAFLOW
    hls::stream<WideType<t_DataType, 1 << t_LogParEntries> > l_mulStr[t_NumStreams];
#pragma HLS DATA_PACK variable = l_mulStr
    hls::stream<WideType<t_DataType, 1> > l_y[t_NumStreams];
#pragma HLS DATA_PACK variable = l_y
    for (int i = 0; i < t_NumStreams; i++) {
#pragma HLS UNROLL
        mul<t_DataType, 1 << t_LogParEntries, t_IndexType>(p_n, p_M, p_x, l_mulStr, p_m);
        sum<t_DataType, t_LogParEntries, t_IndexType>(p_n, l_mulStr, l_y, p_m);
    }
    mergeStream<t_NumStreams>(l_y, p_y);
}

/**
 * @brief gemv function that returns the result vector of the mutiplication of a
 * matrix and a vector y = M * x
 *
 * @tparam t_DataType the data type of the vector entries
 * @tparam t_LogParEntries log2 of the number of parallelly processed entries in the input vector
 * @tparam t_IndexType the datatype of the index
 *
 * @param p_m the number of rows of input matrix p_M
 * @param p_n the number of cols of input matrix p_M, as well as the number of entries in the input vector p_x, p_n %
 * l_ParEntries == 0
 * @param p_M the input stream of packed Matrix entries
 * @param p_x the input stream of packed vector entries
 * @param p_y the output vector
 */
template <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
void gemv(const unsigned int p_m,
          const unsigned int p_n,
          hls::stream<WideType<t_DataType, 1 << t_LogParEntries> >& p_M,
          hls::stream<WideType<t_DataType, 1 << t_LogParEntries> >& p_x,
          hls::stream<WideType<t_DataType, 1> >& p_y) {
#pragma HLS data_pack variable = p_M
#pragma HLS data_pack variable = p_x
#pragma HLS data_pack variable = p_y
#ifndef __SYNTHESIS__
    assert(p_n % (1 << t_LogParEntries) == 0);
#endif
#pragma HLS DATAFLOW
    hls::stream<WideType<t_DataType, 1 << t_LogParEntries> > l_mulStr;
#pragma HLS DATA_PACK variable = l_mulStr
    mul<t_DataType, 1 << t_LogParEntries, t_IndexType>(p_n, p_M, p_x, l_mulStr, p_m);
    sum<t_DataType, t_LogParEntries, t_IndexType>(p_n, l_mulStr, p_y, p_m);
}

/**
 * @brief gemv function that returns the result vector of the mutiplication of a
 * matrix and a vector y = alpha * M * x + beta * y
 *
 * @tparam t_DataType the data type of the vector entries
 * @tparam t_LogParEntries log2 of the number of parallelly processed entries in the input vector
 * @tparam t_IndexType the datatype of the index
 *
 * @param p_m the number of rows of input matrix p_M
 * @param p_n the number of cols of input matrix p_M, as well as the number of entries in the input vector p_x, p_n %
 * l_ParEntries == 0
 * @param p_alpha
 * @param p_M the input stream of packed Matrix entries
 * @param p_x the input stream of packed vector entries
 * @param p_beta
 * @param p_y the output vector
 */
template <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
void gemv(const unsigned int p_m,
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
    hls::stream<WideType<t_DataType, 1> > l_data, l_pad;
#pragma HLS stream variable = l_data depth = 2
#pragma HLS stream variable = l_pad depth = 2
    hls::stream<WideType<t_DataType, 1> > l_x, l_y;
#pragma HLS DATAFLOW
    gemv<t_DataType, t_LogParEntries, t_IndexType>(p_m, p_n, p_M, p_x, l_x);
    scal<t_DataType, 1, t_IndexType>(p_m, p_beta, p_y, l_y);
    axpy<t_DataType, 1, t_IndexType>(p_m, p_alpha, l_x, l_y, p_yr);
}

} // end namespace blas
} // namespace linear_algebra
} // end namespace xf

#endif
