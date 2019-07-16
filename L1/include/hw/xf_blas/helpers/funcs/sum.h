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
 * @file sum.h
 * @brief BLAS Level 1 sum template function implementation.
 *
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_SUM_H
#define XF_BLAS_SUM_H


#ifndef __cplusplus
#error "BLAS Library only works with C++."
#endif

#include "hls_math.h"
#include "hls_stream.h"

namespace xf {
namespace linear_algebra {
namespace blas {

  namespace{
    template<typename t_DataType, 
      unsigned int t_LogParEntries, 
      typename t_IndexType = unsigned int>
        void preProcess(
            unsigned int p_numElems,
            hls::stream<WideType<t_DataType, 1<<t_LogParEntries> > & p_x,
            hls::stream<t_DataType> & p_data,
            unsigned int p_repeat = 1

            ) {
          const unsigned int l_ParEntries = 1 << t_LogParEntries;
          for(unsigned int r=0; r<p_repeat; r++)
            for(t_IndexType i=0;i<p_numElems;i++){
              #pragma HLS PIPELINE
              WideType<t_DataType, 1<<t_LogParEntries> l_x = p_x.read();
              #pragma HLS ARRAY_PARTITION variable=l_x complete dim=1
              t_DataType l_sum;
              l_sum = BinarySum<t_DataType, l_ParEntries>::sum(l_x.getValAddr());
              p_data.write(l_sum);
            }
        }
    template<typename t_DataType, 
      unsigned int t_LogDelays, 
      typename t_IndexType = unsigned int>
        void padding(
            unsigned int p_numElems,
            hls::stream<t_DataType> & p_data,
            hls::stream<t_DataType> & p_pad,
            unsigned int p_repeat = 1

            ) {
          const unsigned int l_Delays = 1 << t_LogDelays;
          const unsigned int l_numIter = (p_numElems + l_Delays -1) >> t_LogDelays;
          const unsigned int l_numExtra = (l_numIter << t_LogDelays) - p_numElems;
          for(unsigned int r=0; r<p_repeat; r++){
            /*
          for(t_IndexType i=0;i<p_numElems;i++){
            #pragma HLS PIPELINE
            p_pad.write(p_data.read());
          }
          for(t_IndexType i=0;i<l_numExtra;i++){
            #pragma HLS PIPELINE
            p_pad.write(0);
          }
          */
            for(t_IndexType i=0;i<(l_numIter << t_LogDelays);i++){
              #pragma HLS PIPELINE
              t_DataType l_v = i<p_numElems? p_data.read() : 0;
                p_pad.write(l_v);
            }
          }
        }

    template<typename t_DataType, 
      unsigned int t_LogDelays, 
      typename t_IndexType = unsigned int>
        void postProcess(
            unsigned int p_numElems,
            hls::stream<t_DataType> & p_pad,
            hls::stream<WideType<t_DataType, 1> > & p_sum,
            unsigned int p_repeat = 1

            ) {
          const unsigned int l_Delays = 1 << t_LogDelays;
          const unsigned int l_numIter = (p_numElems + l_Delays -1) >> t_LogDelays;
          for(unsigned int r=0; r<p_repeat; r++){
            t_DataType l_finalSum = 0;
            for(t_IndexType i=0;i<l_numIter;i++){
              #pragma HLS PIPELINE II=l_Delays
              WideType<t_DataType, l_Delays> l_input;
              #pragma HLS ARRAY_PARTITION variable=l_input complete dim=1
              for(t_IndexType j=0; j<l_Delays;j++){
                #pragma HLS UNROLL
                l_input.shift(p_pad.read());
              }
              l_finalSum += BinarySum<t_DataType, l_Delays>::sum(l_input.getValAddr());
              if(i == l_numIter - 1)
                p_sum.write(l_finalSum);
            }
          }
        }
  }

  /**
   * @brief sum function that returns the sum of all the vector elements.
   *
   * @tparam t_DataType the data type of the vector entries
   * @tparam t_LogParEntries log2 of the number of parallelly processed entries in the input vector 
   * @tparam t_IndexType the datatype of the index 
   *
   * @param p_n the number of entries in the input vector p_x, p_n % l_ParEntries == 0
   * @param p_x the input stream of packed vector entries
   * @param p_repeat number of repeat 
   * @param p_sum the sum, which is 0 if p_n <= 0
   */

  template<typename t_DataType, 
    unsigned int t_LogParEntries, 
    typename t_IndexType=unsigned int>
      void sum(
          unsigned int p_n,
          hls::stream<WideType<t_DataType, 1<<t_LogParEntries> > & p_x,
          hls::stream<WideType<t_DataType, 1> > &p_sum,
          unsigned int p_repeat
          ) {
        #pragma HLS data_pack variable=p_x
        #ifndef __SYNTHESIS__
        assert(p_n % ( 1 << t_LogParEntries) == 0);
        #endif
        const unsigned int l_LogDelays = AdderDelay<t_DataType>::m_logDelays;
        #pragma HLS DATAFLOW
        hls::stream<t_DataType> l_data, l_pad;
        #pragma HLS stream variable=l_data depth=2
        #pragma HLS stream variable=l_pad depth=2
        #pragma HLS data_pack variable=l_data
        #pragma HLS data_pack variable=l_pad
        unsigned int l_numElem = p_n >> t_LogParEntries;
        preProcess<t_DataType, t_LogParEntries, t_IndexType>(l_numElem, p_x, l_data, p_repeat);
        padding<t_DataType, l_LogDelays, t_IndexType>(l_numElem, l_data, l_pad, p_repeat);
        postProcess<t_DataType, l_LogDelays, t_IndexType>(l_numElem, l_pad, p_sum, p_repeat);
      }

  /**
   * @brief sum function that returns the sum of all the vector elements.
   *
   * @tparam t_DataType the data type of the vector entries
   * @tparam t_LogParEntries log2 of the number of parallelly processed entries in the input vector 
   * @tparam t_IndexType the datatype of the index 
   *
   * @param p_n the number of entries in the input vector p_x, p_n % l_ParEntries == 0
   * @param p_x the input stream of packed vector entries
   * @param p_sum the sum, which is 0 if p_n <= 0
   */

  template<typename t_DataType, 
    unsigned int t_LogParEntries, 
    typename t_IndexType=unsigned int>
      void sum(
          unsigned int p_n,
          hls::stream<WideType<t_DataType, 1<<t_LogParEntries> > & p_x,
          t_DataType &p_sum
          ) {
        #pragma HLS data_pack variable=p_x
        hls::stream<WideType<t_DataType, 1> > p_s;
        sum<t_DataType, t_LogParEntries, t_IndexType>(p_n, p_x, p_s, 1);
        p_sum = p_s.read()[0];
      }


} //end namespace blas
} //end namspace linear_algebra
} //end namespace xf

#endif

