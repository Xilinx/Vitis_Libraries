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
#include "gemv.h"
#include "xf_blas/helpers.h"

using namespace xf::linear_algebra::blas;


void uut_top(
  uint32_t p_m,
  uint32_t p_n,
  uint32_t p_kl,
  uint32_t p_ku,
  BLAS_dataType p_alpha,
  BLAS_dataType p_beta,
  BLAS_dataType p_a[BLAS_matrixSize],
  BLAS_dataType p_x[BLAS_vectorSize],
  BLAS_dataType p_y[BLAS_vectorSize],
  BLAS_dataType p_aRes[BLAS_matrixSize],
  BLAS_dataType p_yRes[BLAS_vectorSize]
  ) {

  hls::stream<WideType<BLAS_dataType, 1 << BLAS_logParEntries> > l_strA;
  #pragma HLS data_pack variable=l_strA
  hls::stream<WideType<BLAS_dataType, 1 << BLAS_logParEntries> > l_strX;
  #pragma HLS data_pack variable=l_strX
  hls::stream<WideType<BLAS_dataType, 1 << BLAS_logParEntries> > l_strY;
  #pragma HLS data_pack variable=l_strY
  #pragma HLS DATAFLOW
  gem2Stream<BLAS_dataType, BLAS_parEntries>(p_m, p_n, p_a, l_strA);
  vec2GemStream<BLAS_dataType, BLAS_parEntries>(p_m, p_n, p_x, l_strX);
  gemv<BLAS_dataType, BLAS_logParEntries>(p_m, p_n, l_strA, l_strX, l_strY);
  writeStream2Vec<BLAS_dataType, BLAS_parEntries>(l_strY, p_n, p_yRes);
}

#endif
