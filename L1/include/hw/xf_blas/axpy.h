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


/**
 * @file axpy.h
 * @brief BLAS Level 1 axpy template function implementation.
 *
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_AXPY_H
#define XF_BLAS_AXPY_H


#ifndef __cplusplus
#error "BLAS Library only works with C++."
#endif

#include "ap_int.h"
#include "hls_math.h"
#include "hls_stream.h"
#include "xf_blas/utility.h"


namespace xf {
namespace linear_algebra {
namespace blas {

  /**
   * @brief axpy function that compute Y = alpha*X + Y.
   *
   * @tparam t_DataType the data type of the vector entries
   * @tparam t_DataWidth the datawidth of the datatype t_DataType of the input vector 
   * @tparam t_LogParEntries log2 of the number of parallelly processed entries in the input vector 
   * @tparam t_IndexType the datatype of the index 
   *
   * @param p_n the number of stided entries entries in the input vector p_x, p_n % l_ParEntries == 0
   * @param p_x the input stream of packed entries of vector X
   * @param p_y the input stream of packed entries of vector Y
   * @param p_r the output stream of packed entries of result vector Y
   */
  template<typename t_DataType, 
    unsigned int t_DataWidth, 
    unsigned int t_LogParEntries, 
    typename t_IndexType=unsigned int>
      void axpy(
          unsigned int p_n,
          const t_DataType p_alpha,
          hls::stream<ap_uint<(t_DataWidth << t_LogParEntries)> > & p_x,
          hls::stream<ap_uint<(t_DataWidth << t_LogParEntries)> > & p_y,
          hls::stream<ap_uint<(t_DataWidth << t_LogParEntries)> > & p_r
          ) {
#ifndef __SYNTHESIS__
        assert(p_n % ( 1 << t_LogParEntries) == 0);
#endif
        const unsigned int l_numElem = p_n >> t_LogParEntries;
        const unsigned int l_ParEntries = 1 << t_LogParEntries;
        for(t_IndexType i=0;i<l_numElem;i++){
#pragma HLS PIPELINE
          ap_uint<(t_DataWidth << t_LogParEntries)> l_x = p_x.read();
          ap_uint<(t_DataWidth << t_LogParEntries)> l_y = p_y.read();
          ap_uint<(t_DataWidth << t_LogParEntries)> l_r = 0;
          for(t_IndexType j=0;j<l_ParEntries;j++){
#pragma HLS UNROLL
            ap_uint<t_DataWidt> l_partX = l_x.range((j+1)*t_DataWidth -1, j*t_DataWidth);
            t_DataType l_realX = *(t_DataType*)&l_partX;
            ap_uint<t_DataWidt> l_partY = l_y.range((j+1)*t_DataWidth -1, j*t_DataWidth);
            t_DataType l_realY = *(t_DataType*)&l_partY;
            t_DataType l_result = p_alpha * l_realX + l_realY;
            ap_uint<t_DataWidt> l_partR = *(ap_uint<t_DataWidth>*)&l_result;
            l_r.range((j+1)*t_DataWidth -1, j*t_DataWidth) = l_partR;
          }
          p_r.write(l_r);
        }
      }

} //end namespace blas
} //end namspace linear_algebra
} //end namespace xf
#endif
