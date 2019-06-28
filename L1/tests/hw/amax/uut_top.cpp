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
#ifndef UUT_TOP_H
#define UUT_TOP_H

#include "ap_int.h"
#include "hls_stream.h"
#include "amaxmin.h"
#include "xf_blas/utility.h"

using namespace xf::linear_algebra::blas;


int uut_top(
  uint32_t p_n,
  BLAS_dataType p_alpha,
  BLAS_dataType *p_x,
  BLAS_dataType *p_y,
  BLAS_dataType *p_xRes,
  BLAS_dataType *p_yRes,
  BLAS_resDataType p_goldRes
) {
  #pragma HLS INTERFACE m_axi port=p_x depth=16
  #pragma HLS INTERFACE m_axi port=p_y depth=16
  #pragma HLS INTERFACE m_axi port=p_xRes depth=16
  #pragma HLS INTERFACE m_axi port=p_yRes depth=16
  BLAS_resDataType l_res;

  hls::stream<WideType<BLAS_dataType, 1<<BLAS_logParEntries, BLAS_dataWidth> > l_str;
  #pragma HLS DATAFLOW
  readVec2Stream<BLAS_dataType,BLAS_dataWidth, 1<<BLAS_logParEntries>(p_x, p_n, l_str);
  amax<BLAS_dataType,BLAS_logParEntries,BLAS_dataWidth,BLAS_resDataType>(p_n, l_str, l_res);
  if (l_res == p_goldRes) {
    return(0);
  }
  else {
    return(-1);
  }
}
 
#endif
