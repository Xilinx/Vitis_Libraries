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

#ifndef XF_BLAS_SBMV_H
#define XF_BLAS_SBMV_H

#ifndef __cplusplus
#error "BLAS Library only works with C++."
#endif

#include "ap_int.h"
#include "hls_stream.h"
#include "xf_blas/helpers.h"

namespace xf {
namespace linear_algebra {
namespace blas {

  /**
   * @brief gbmv function performs general banded matrix-vector multiplication 
   *
   * @tparam t_DataType the data type of the vector entries
   * @tparam t_ParEntries the number of parallelly processed entries in the input vector 
   * @tparam t_MacType the datawidth of the datatype t_DataType of the input vector 
   * @tparam t_IndexType the datatype of the index 
   *
   * @param p_n the number of entries in the input vector p_x, p_n % l_ParEntries == 0
   * @param p_x the input stream of packed vector entries
   * @param p_sum the sum, which is 0 if p_n <= 0
   */
  template<typename t_DataType, 
    unsigned int t_ParEntries,
    typename t_MacType = t_DataType,
    typename t_IndexType=unsigned int>
      void gbmv(const unsigned int p_row,
          const unsigned int p_kl,
          const unsigned int p_ku,
          hls::stream<WideType<t_DataType, t_ParEntries> > & p_A,
          hls::stream<WideType<t_DataType, t_ParEntries> > & p_x,
          hls::stream<WideType<t_MacType, t_ParEntries> > &p_y
          ){
        static const unsigned int l_delays = AdderDelay<t_DataType>::m_Delays + 4; // 4 for the latency of BRAM read and write

        static const unsigned int l_blocks =l_delays * t_ParEntries; 
        #ifndef __SYNTHESIS__
        assert(p_row % l_blocks == 0);
        #endif

        const unsigned int l_numIter = p_row/l_blocks;

        WideType<t_MacType, t_ParEntries> l_y[l_delays];
        for(t_IndexType l=0; l <l_delays; l++){
          #pragma HLS PIPELINE
          for(t_IndexType k=0;k<t_ParEntries;k++){
            #pragma HLS UNROLL
            l_y[l][k] = 0;
          }
        }

        for(t_IndexType i=0; i< l_numIter;i++){
          for(t_IndexType j=0;j<p_kl + 1 + p_ku;j++){
            for(t_IndexType l=0; l <l_delays; l++){
              #pragma HLS PIPELINE 
              WideType<t_DataType, t_ParEntries> l_A = p_A.read();
              WideType<t_DataType, t_ParEntries> l_x = p_x.read();
              for(t_IndexType k=0;k<t_ParEntries;k++){
                #pragma HLS UNROLL
                l_y[l][k] += l_A[k] * l_x[k];
              }
            }
          }
          for(t_IndexType l=0; l <l_delays; l++){
            #pragma HLS PIPELINE
            p_y.write(l_y[l]);
            for(t_IndexType k=0;k<t_ParEntries;k++){
              #pragma HLS UNROLL
              l_y[l][k] = 0;
            }
          }
        }
      }


} //end namespace blas
} //end namspace linear_algebra
} //end namespace xf


#endif
