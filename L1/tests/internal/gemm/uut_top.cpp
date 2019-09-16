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
#include "ap_int.h"
#include "hls_stream.h"
#include "xf_blas.hpp"

using namespace xf::linear_algebra::blas;

void uut_top(uint32_t p_m,
             uint32_t p_n,
             uint32_t p_k,
             uint32_t p_r,
             BLAS_dataType p_a[BLAS_r * BLAS_m * BLAS_k],
             BLAS_dataType p_b[BLAS_r * BLAS_n * BLAS_k],
             BLAS_dataType p_c[BLAS_r * BLAS_m * BLAS_n]) {
#pragma HLS ARRAY_PARTITION variable = p_a cyclic factor = 16 dim = 1
#pragma HLS ARRAY_PARTITION variable = p_b cyclic factor = 16 dim = 1
#pragma HLS ARRAY_PARTITION variable = p_c cyclic factor = 16 dim = 1
#pragma HLS DATAFLOW

    hls::stream<WideType<BLAS_dataType, BLAS_m> > l_strA;
#pragma HLS data_pack variable = l_strA
    hls::stream<WideType<BLAS_dataType, BLAS_n> > l_strB;
#pragma HLS data_pack variable = l_strB
    hls::stream<WideType<BLAS_dataType, BLAS_n> > l_strC;
#pragma HLS data_pack variable = l_strC

#pragma HLS DATAFLOW
    gem2Stream<BLAS_dataType, BLAS_m>(p_k * p_r, p_m, p_a, l_strA);
    gem2Stream<BLAS_dataType, BLAS_n>(p_k * p_r, p_n, p_b, l_strB);
    gemm<BLAS_dataType, BLAS_m, BLAS_n>(p_k, l_strA, l_strB, l_strC, p_r);
    writeStream2Vec<BLAS_dataType, BLAS_n>(l_strC, p_n * p_m * p_r, p_c);
}
