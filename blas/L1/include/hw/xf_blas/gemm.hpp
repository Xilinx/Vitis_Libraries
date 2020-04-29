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

namespace blas {

template <typename t_DataType, unsigned int t_M, unsigned int t_N = t_M, typename t_MacDataType = t_DataType>
class SystolicArray {
   public:
    static void process_dsp(unsigned int p_k,
                            hls::stream<typename WideType<t_DataType, t_M>::t_TypeInt>& p_As,
                            hls::stream<typename WideType<t_DataType, t_N>::t_TypeInt>& p_Bs,
                            hls::stream<typename WideType<t_MacDataType, t_N>::t_TypeInt>& p_sum,
                            unsigned int p_multi = 1) {
#ifndef __SYNTHESIS__
        assert(p_k >= t_M + t_N);
#endif

        WideType<t_DataType, t_M + t_N> l_winA[t_M];
#pragma HLS ARRAY_PARTITION variable = l_winA dim = 0 complete
        WideType<t_DataType, t_M + t_N> l_winB[t_N];
#pragma HLS ARRAY_PARTITION variable = l_winB dim = 0 complete

        WideType<t_MacDataType, t_N> l_C[t_M];
#pragma HLS ARRAY_PARTITION variable = l_C dim = 0 complete
        WideType<t_MacDataType, t_N> l_Co[t_M];
#pragma HLS ARRAY_PARTITION variable = l_Co dim = 0 complete

        for (int k = 0, l = 0; l < p_multi * p_k + t_M + t_N; l++, k++) {
#pragma HLS PIPELINE
            if (k == p_k) {
                k = 0;
            }

            if (l > p_k && k >= t_N && k < t_M + t_N) {
                p_sum.write(l_Co[k - t_N]);
            }

            WideType<t_DataType, t_M> l_A = 0;
            WideType<t_DataType, t_N> l_B = 0;

            if (l < p_multi * p_k) {
                l_A = p_As.read();
                l_B = p_Bs.read();
            }

            for (int j = 0; j < t_M; j++) l_winA[j].shift(l_A[j]);
            for (int j = 0; j < t_N; j++) l_winB[j].shift(l_B[j]);
            for (int m = 0; m < t_M; m++) {
                for (int n = 0; n < t_N; n++) {
                    int l_id = m + n;
                    if (l_id == k) {
                        l_Co[m][n] = l_C[m][n];
                        l_C[m][n] = 0;
                    }
                    l_C[m][n] += l_winA[m][l_id] * l_winB[n][l_id];
                }
            }
        }
    }
};

template <typename t_DataType,
          unsigned int t_M,
          unsigned int t_N = t_M,
          typename t_IndexType = unsigned int,
          typename t_MacDataType = t_DataType>
void gemm(const unsigned int p_k,
          hls::stream<typename WideType<t_DataType, t_M>::t_TypeInt>& p_A,
          hls::stream<typename WideType<t_DataType, t_N>::t_TypeInt>& p_B,
          hls::stream<typename WideType<t_MacDataType, t_N>::t_TypeInt>& p_C,
          const unsigned int p_r = 1) {
#pragma HLS DATAFLOW
    SystolicArray<t_DataType, t_M, t_N, t_MacDataType>::process_dsp(p_k, p_A, p_B, p_C, p_r);
}

} // end namespace blas

} // end namespace xf

#endif
