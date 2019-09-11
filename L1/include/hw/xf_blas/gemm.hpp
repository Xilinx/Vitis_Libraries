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

#ifndef XF_BLAS_GEMM_HPP
#define XF_BLAS_GEMM_HPP

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

template <typename t_DataType, unsigned int t_M, unsigned int t_N, typename t_MacDataType = t_DataType>
class SystolicArray {
   public:
    static void process_dsp(unsigned int p_k,
                            unsigned int p_multi,
                            hls::stream<WideType<t_DataType, t_M> >& p_As,
                            hls::stream<WideType<t_DataType, t_N> >& p_Bs,
                            hls::stream<WideType<t_MacDataType, t_N> >& p_sum) {
        WideType<t_DataType, t_M + t_N> l_winA[t_M];
#pragma HLS ARRAY_PARTITION variable = l_winA dim = 0 complete
        WideType<t_DataType, t_M + t_N> l_winB[t_N];
#pragma HLS ARRAY_PARTITION variable = l_winB dim = 0 complete
        for (int i = 0; i < t_M + t_N; i++) {
#pragma HLS PIPELINE
            for (int j = 0; j < t_M; j++) l_winA[j].shift(0);
            for (int j = 0; j < t_N; j++) l_winB[j].shift(0);
        }

        WideType<t_MacDataType, t_N> l_C[t_M];
#pragma HLS ARRAY_PARTITION variable = l_C dim = 0 complete
        for (int i = 0; i < t_M; i++) l_C[i] = 0;

        for (int l = 0; l < p_multi; l++) {
            for (int k = 0; k < p_k + t_M + t_N; k++) {
#pragma HLS PIPELINE
                WideType<t_DataType, t_M> l_A = 0;
                WideType<t_DataType, t_N> l_B = 0;
                if (k < p_k) {
                    l_A = p_As.read();
                    l_B = p_Bs.read();
                }
                for (int j = 0; j < t_M; j++) l_winA[j].shift(l_A[j]);
                for (int j = 0; j < t_N; j++) l_winB[j].shift(l_B[j]);
                for (int m = 0; m < t_M; m++) {
                    for (int n = 0; n < t_N; n++) {
                        l_C[m][n] += l_winA[m][m + n] * l_winB[n][m + n];
                    }
                }
            }
            for (int i = 0; i < t_M; i++) {
#pragma HLS PIPELINE
                p_sum.write(l_C[i]);
                l_C[i] = 0;
            }
        }
    }
};
template <unsigned int t_M,
          unsigned int t_N,
          typename t_DataType,
          typename t_IndexType = unsigned int,
          typename t_MacDataType = t_DataType>
void gemm(const unsigned int p_k,
          hls::stream<WideType<t_DataType, t_M> >& p_A,
          hls::stream<WideType<t_DataType, t_N> >& p_B,
          hls::stream<WideType<t_MacDataType, t_N> >& p_C) {
#pragma HLS DATAFLOW
    SystolicArray<t_DataType, t_M, t_N, t_MacDataType>::process_dsp(p_k, 1, p_A, p_B, p_C);
}

} // end namespace blas
} // namespace linear_algebra
} // end namespace xf

#endif
