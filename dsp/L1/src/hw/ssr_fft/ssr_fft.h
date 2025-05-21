/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#include <complex>
#include <hls_stream.h>
#include "math.h"
#include <ap_fixed.h>

//#define __SSR_FFT_DEBUG__
struct cfloat_set {
    std::complex<float> data1;
    std::complex<float> data2;
};
class ssrFFTClass {
   public:
    static constexpr int nBtfls = POINT_SIZE / 2;
    static constexpr unsigned int nStages =
        POINT_SIZE == 64 ? 6
                         : POINT_SIZE == 32 ? 5 : POINT_SIZE == 16 ? 4 : POINT_SIZE == 8 ? 3 : POINT_SIZE == 4 ? 2 : 0;

    std::complex<float> scBuff[nStages][nBtfls];
    typedef cfloat_set TT_DATA;
    typedef hls::stream<TT_DATA> TT_STREAM;

    void butterfly(std::complex<float>* inData1,
                   std::complex<float>* inData2,
                   std::complex<float>* curTwid,
                   std::complex<float>* outData1,
                   std::complex<float>* outData2) {
        // TT_DATA outData;
        // #pragma HLS inline
        std::complex<float> prod;
#pragma HLS BIND_OP variable = prod op = fmul impl = maxdsp
#pragma HLS BIND_OP variable = prod op = fadd impl = maxdsp
        prod = (*inData2) * (*curTwid);
        *outData1 = (*inData1) + prod;
        *outData2 = (*inData1) - prod;
    }

    void ssr_fft(TT_STREAM inData[POINT_SIZE], TT_STREAM outData[POINT_SIZE]) {
#pragma HLS inline

        int stride;
        std::complex<float> twid __attribute((no_ctor));
        cfloat_set curData1;
        cfloat_set curData2;
        std::complex<float> curData11 __attribute((no_ctor));
        std::complex<float> curData12 __attribute((no_ctor));
        std::complex<float> curData21 __attribute((no_ctor));
        std::complex<float> curData22 __attribute((no_ctor));
        std::complex<float> tmpBuff1[nStages][POINT_SIZE] __attribute((no_ctor));
        std::complex<float> tmpBuff2[nStages][POINT_SIZE] __attribute((no_ctor));

    STAGE_LOOP:
        for (int stage = 0; stage < nStages; stage++) {
        BTFLY_LOOP:
            for (int btfly = 0; btfly < nBtfls; btfly++) {
                int rdPos1, rdPos2;
                int wrPos1 = btfly;
                int wrPos2 = btfly + nBtfls;
                stride = nBtfls >> stage;
                if (stage == 0) {
                    rdPos1 = btfly;
                    rdPos2 = btfly + stride;
                    curData1 = inData[rdPos1].read();
                    curData2 = inData[rdPos2].read();
                } else {
                    rdPos1 = (btfly % stride) + (btfly / stride) * stride * 2;
                    rdPos2 = (btfly % stride) + (btfly / stride) * stride * 2 + stride;
                    curData11 = tmpBuff1[stage - 1][rdPos1];
                    curData12 = tmpBuff1[stage - 1][rdPos2];
                    curData21 = tmpBuff2[stage - 1][rdPos1];
                    curData22 = tmpBuff2[stage - 1][rdPos2];
                }
                twid = scBuff[stage][btfly];
                if (stage == 0) {
                    butterfly(&curData1.data1, &curData2.data1, &twid, &tmpBuff1[stage][wrPos1],
                              &tmpBuff1[stage][wrPos2]);
                    butterfly(&curData1.data2, &curData2.data2, &twid, &tmpBuff2[stage][wrPos1],
                              &tmpBuff2[stage][wrPos2]);
                } else {
                    butterfly(&curData11, &curData12, &twid, &tmpBuff1[stage][wrPos1], &tmpBuff1[stage][wrPos2]);
                    butterfly(&curData21, &curData22, &twid, &tmpBuff2[stage][wrPos1], &tmpBuff2[stage][wrPos2]);
                }
            }
            if (stage == nStages - 1) {
                for (int ss = 0; ss < POINT_SIZE; ss++) {
                    cfloat_set outDataTmp;
                    outDataTmp.data1 = tmpBuff1[stage][ss];
                    outDataTmp.data2 = tmpBuff2[stage][ss];
                    outData[ss].write(outDataTmp);
                }
            }
        }
    }
    ssrFFTClass() {
#define PI 3.1415926
        for (int st = 0; st < nStages; st++) {
            int stride = nBtfls >> st;
            for (int bt = 0; bt < nBtfls; bt++) {
                float angle = -2.0f * (float)(PI * (float)(bt / stride) * stride) / (float)POINT_SIZE;
                scBuff[st][bt].real(cosf(angle));
                scBuff[st][bt].imag(sinf(angle));
            }
        }
    }
};

void ssr_fft_wrapper(ssrFFTClass::TT_STREAM inData[POINT_SIZE], ssrFFTClass::TT_STREAM outData[POINT_SIZE]);
