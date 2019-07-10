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

#ifndef XF_BLAS_SPMV_H
#define XF_BLAS_SPMV_H

#ifndef __cplusplus
#error "BLAS Library only works with C++."
#endif

#include "ap_int.h"
#include "hls_stream.h"
#include "xf_blas/helpers.h"

namespace xf {
namespace linear_algebra {
namespace blas {

  namespace{
    template<typename t_DataType, 
      unsigned int t_LogParEntries, 
      typename t_IndexType = unsigned int>
        void preProcess(
            unsigned int p_rows,
            unsigned int p_numIter,
            hls::stream<WideType<t_DataType, 1<<t_LogParEntries> > & p_x,
            hls::stream<t_DataType> & p_data
            ) {
          for(t_IndexType i=0; i<p_n; i++)
            for(t_IndexType j=0;j<l_numIter;j++){
              #pragma HLS PIPELINE 
              WideType<t_DataType, 1<<t_LogParEntries> l_A = p_M.read();
              WideType<t_DataType, 1<<t_LogParEntries> = p_x.read();
              t_DataType l_dot[1 << t_LogParEntries];
              #pragma HLS ARRAY_PARTITION variable=l_dot complete dim=1
              for(t_IndexType k=0;k<t_ParEntries;k++){
                #pragma HLS UNROLL
                l_dot[k] =l_A[k] * l_x[k];
              }
              p_r.write(BinarySum<t_DataType, 1 << t_LogParEntries>::sum(l_dot));
            }
        }
    template<typename t_DataType, 
      unsigned int t_LogDelays, 
      typename t_IndexType = unsigned int>
        void padding(
            unsigned int p_n,
            unsigned int p_numElems,
            hls::stream<t_DataType> & p_data,
            hls::stream<t_DataType> & p_pad
            ) {
          const unsigned int l_Delays = 1 << t_LogDelays;
          const unsigned int l_numIter = (p_numElems + l_Delays -1) >> t_LogDelays;
          const unsigned int l_numExtra = (l_numIter << t_LogDelays) - p_numElems;
          hls::stream<t_DataType> l_inner;
          #pragma HLS stream variable=l_inner depth=2
          for(t_IndexType j=0; j<p_n; j++){
            for(t_IndexType i=0;i<p_numElems;i++){
              #pragma HLS PIPELINE
              p_pad.write(p_data.read());
            }
            for(t_IndexType i=0;i<l_numExtra;i++){
              #pragma HLS PIPELINE
              p_pad.write(0);
            }
          }
        }

    template<typename t_DataType, 
      unsigned int t_LogDelays, 
      typename t_IndexType = unsigned int>
        void postProcess(
            unsigned int p_n,
            unsigned int p_numElems,
            hls::stream<t_DataType> & p_pad,
            hls::stream<t_DataType> & p_sum
            ) {
          const unsigned int l_Delays = 1 << t_LogDelays;
          const unsigned int l_numIter = (p_numElems + l_Delays -1) >> t_LogDelays;
          t_DataType l_finalSum = 0;
          for(t_IndexType j=0; j<p_n; j++){
            for(t_IndexType i=0;i<l_numIter;i++){
              #pragma HLS PIPELINE II=l_Delays
              t_DataType l_input[l_Delays];
              #pragma HLS ARRAY_PARTITION variable=l_input complete dim=1
              for(t_IndexType j=0; j<l_Delays;j++){
                #pragma HLS UNROLL
                l_input[j]=p_pad.read();
              }
              l_finalSum += BinarySum<t_DataType, l_Delays>::sum(l_input);
            }
            p_sum.write(l_finalSum);
          }
        }
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
   * @param p_n the number of cols of input matrix p_M, as well as the number of entries in the input vector p_x, p_n % l_ParEntries == 0
   * @param p_M the input stream of packed Matrix entries
   * @param p_x the input stream of packed vector entries
   * @param p_y the output vector 
   */
  template<typename t_DataType, 
    unsigned int t_LogParEntries,
    typename t_IndexType=unsigned int>
      void gemv(const unsigned int p_m,
          const unsigned int p_n,
          hls::stream<WideType<t_DataType, 1<<t_LogParEntries> > & p_M,
          hls::stream<WideType<t_DataType, 1<<t_LogParEntries> > & p_x,
          hls::stream<t_DataType> &p_y
          ){
        #ifndef __SYNTHESIS__
        assert(p_n % ( 1 << t_LogParEntries) == 0);
        #endif
        const unsigned int l_numIter = p_n >> t_LogParEntries;
        hls::stream<t_DataType> l_data, l_pad;
        #pragma HLS stream variable=l_data depth=2
        #pragma HLS stream variable=l_pad depth=2
        const unsigned int l_LogDelays = AdderDelay<t_DataType>::m_logDelays;
        #pragma HLS DATAFLOW
        preProcess<t_DataType, t_LogParEntries, t_IndexType>(p_m, l_numIter, p_M, p_x, l_data);
        padding<t_DataType, l_LogDelays, t_IndexType>(p_m, l_numIter, l_data, l_pad);
        postProcess<t_DataType, l_LogDelays, t_IndexType>(p_m, l_numIter, l_pad, p_y);
      }


} //end namespace blas
} //end namspace linear_algebra
} //end namespace xf


#endif
