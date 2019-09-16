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
#ifndef _MCENGINE_TOP_H_
#define _MCENGINE_TOP_H_

#include "xf_fintech/mc_engine.hpp"
#include "xf_fintech/utils.hpp"

typedef double TEST_DT;

#define UN 2
#define iteration 4
#define depthP 1024 * 100 * iteration
#define depthM 9 * 100

template <typename DT, int UNROLL, int Size>
void read_ddr(int depth, ap_uint<UNROLL * Size>* In, hls::stream<DT> out_strm[UNROLL]) {
    for (int i = 0; i < depth; ++i) {
#pragma HLS PIPELINE II = 1
#pragma HLS loop_tripcount min = 51200 max = 51200
        ap_uint<UNROLL* Size> out_0 = *In++;
        for (int k = 0; k < UNROLL; ++k) {
            uint64_t in = out_0((k + 1) * Size - 1, k * Size);
            out_strm[k].write(xf::fintech::internal::bitsToDouble(in));
        }
    }
};

void kernel_0_top(TEST_DT underlying,
                  TEST_DT volatility,
                  TEST_DT riskFreeRate,
                  TEST_DT dividendYield,
                  TEST_DT timeLength,
                  TEST_DT strike,
                  bool optionType,
                  ap_uint<32> seed[UN],
                  ap_uint<UN * sizeof(TEST_DT) * 8> pOut[depthP],
                  ap_uint<sizeof(TEST_DT) * 8> mOut[depthM],
                  unsigned int calibSamples,
                  unsigned int timeSteps);

#endif
