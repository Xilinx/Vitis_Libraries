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

#include "asum_top.h"
using namespace xf::linear_algebra::blas;
void asum_top(
  unsigned int p_n,
  hls::stream<WideType<BLAS_dataType, BLAS_parEntries, BLAS_dataWidth > > &p_x,
  BLAS_dataType &p_result 
) {
  BLAS_op <
    BLAS_dataType,
    mylog2(BLAS_parEntries),
    BLAS_dataWidth,
    BLAS_indexType> (
    p_n, p_x, p_result
  );
}

void readVec2Stream(
  BLAS_dataType p_in[BLAS_size],
  unsigned int p_n,
  hls::stream<WideType<BLAS_dataType, BLAS_parEntries, BLAS_dataWidth > > &p_out
) {
#pragma HLS ARRAY_PARTITION variable=p_in cyclic factor=2 dim=1
  #ifndef __SYNTHESIS__
    assert ((p_n % BLAS_parEntries) == 0);
  #endif
  unsigned int l_parBlocks = p_n / BLAS_parEntries;
  for (unsigned int i=0; i<l_parBlocks; ++i) {
  #pragma HLS PIPELINE
    BitConv<BLAS_dataType> l_bitConv;
    WideType<BLAS_dataType, BLAS_parEntries, BLAS_dataWidth > l_val;
    for (unsigned int j=0; j<BLAS_parEntries; ++j) {
      l_val[j] = p_in[i*BLAS_parEntries + j];
    }
    p_out.write(l_val);
  }
}

void UUT_Top(
  BLAS_dataType p_in[BLAS_size],
  unsigned int p_n,
  BLAS_dataType &p_result
){
  hls::stream<WideType<BLAS_dataType, BLAS_parEntries, BLAS_dataWidth> > l_str;
  #pragma HLS DATAFLOW
  readVec2Stream(
    p_in,
    p_n,
    l_str
  );
  asum_top(
    p_n,
    l_str,
    p_result 
  );
}
