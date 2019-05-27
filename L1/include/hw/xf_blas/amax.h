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
 * @file amax.h
 * @brief BLAS Level 1 amax template function implementation.
 *
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_AMAX_H
#define XF_BLAS_AMAX_H

#ifndef __cplusplus
#error "BLAS Library only works with C++."
#endif

#include "ap_int.h"
#include "hls_stream.h"
#include "utility/utility.h"

namespace xf {
namespace linear_algebra {
namespace blas {

/**
 * @brief amax function that returns the position of the vector element that has the maximum magnitude.
 *
 * @tparam t_DataType the data type of the vector entries
 * @tparam t_N maximum number of entries in the input vector, p_N >= 1+(p_n-1)*|p_incx|
 * @tparam t_ParEntries number of parallelly processed entries in the input vector 
 *
 * @param p_n the number of stided entries entries in the input vector p_x, p_n % t_ParEntries == 0
 * @param p_x the input vector
 * @param p_result the resulting index, which is 0 if p_n <= 0
*/
template<typename t_DataType, unsigned int t_N, unsigned int t_ParEntries>
void amax (
  unsigned int p_n,
  t_DataType p_x[t_N],
  unsigned int &p_result
) {
  #pragma HLS ARRAY_PARTITION variable=p_x dim=1 cyclic factor=t_ParEntries partition
  unsigned int l_parBlocks = p_n / t_ParEntries;

  #ifndef __SYNTHESIS__
  assert (p_n > 0);
  assert ((p_n % t_ParEntries) == 0);
  #endif

  WideType<t_DataType, t_ParEntries> l_wideMax(0);
  WideType<t_IndexType, t_ParEntries> l_wideIndex(0);
  #pragma HLS ARRAY_PARTITION variable=l_wideMax complete
  #pragma HLS ARRAY_PARTITION variable=l_WideIndex complete

  for (int i=0; i<l_parBlocks; ++i) {
  #pragma HLS PIPELINE
    WideType<t_DataType, t_ParEntries> l_wideVal;
    WideType<t_DataType, t_ParEntries> l_wideValAbs;
    for (int j=0; j<t_ParEntries; ++j) {
    #pragma HLS UNROLL
      l_wideVal[j] = p_x[i*t_ParEntries + j];
      l_wideValAbs[j] = (l_wideVal[j] < 0)? -l_wideVal[j]: l_wideVal[j];
      if (l_wideValAbs[j] > l_wideMax[j]) {
        l_wideMax[j] = l_wideValAbs[j];
        l_wideIndex[j] = i*t_ParEntries + j
      }
    }
  }
  
  t_DataType l_unusedVal;
  t_IndexType l_index;

  wideCmpBiggerIndex<t_DataType, t_IndexType, t_ParEntries>(
    l_wideMax,
    l_wideIndex;
    l_unusedVal,
    l_index);
  p_result = l_index;
}

} //end namespace blas
} //end namspace linear_algebra
} //end namespace xf

#endif
