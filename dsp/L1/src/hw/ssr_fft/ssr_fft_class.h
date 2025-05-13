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

class ssrFFTClass {
   public:
    static constexpr int nBtfls = POINT_SIZE / 2;
    static constexpr unsigned int nStages =
        POINT_SIZE == 32 ? 5 : POINT_SIZE == 16 ? 4 : POINT_SIZE == 8 ? 3 : POINT_SIZE == 4 ? 2 : 0;
    std::complex<float> scBuff[nStages][nBtfls];

    void butterfly(std::complex<float>* inData1,
                   std::complex<float>* inData2,
                   std::complex<float>* curTwid,
                   std::complex<float>* outData1,
                   std::complex<float>* outData2) {
        std::complex<float> prod;
#pragma HLS BIND_OP variable = prod op = fmul impl = maxdsp
#pragma HLS BIND_OP variable = prod op = fadd impl = maxdsp
        prod = (*inData2) * (*curTwid);
        *outData1 = (*inData1) + prod;
        *outData2 = (*inData1) - prod;
    }

    void ssr_fft(hls::stream<std::complex<float> > inData[POINT_SIZE],
                 hls::stream<std::complex<float> > outData[POINT_SIZE]) {
        int stride;
        std::complex<float> twid __attribute((no_ctor));
        std::complex<float> curData1 __attribute((no_ctor));
        std::complex<float> curData2 __attribute((no_ctor));
        std::complex<float> tmpBuff[nStages][POINT_SIZE] __attribute((no_ctor));
#pragma HLS BIND_OP variable = curData1 op = fmul impl = maxdsp
#pragma HLS BIND_OP variable = curData1 op = fadd impl = maxdsp
#pragma HLS BIND_OP variable = curData2 op = fmul impl = maxdsp
#pragma HLS BIND_OP variable = curData2 op = fadd impl = maxdsp

#pragma HLS PIPELINE II = 1
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
                    curData1 = tmpBuff[stage - 1][rdPos1];
                    curData2 = tmpBuff[stage - 1][rdPos2];
                }
                twid.real(scBuff[stage][btfly].real());
                twid.imag(scBuff[stage][btfly].imag());
                butterfly(&curData1, &curData2, &twid, &tmpBuff[stage][wrPos1], &tmpBuff[stage][wrPos2]);
            }
            if (stage == nStages - 1) {
                for (int ss = 0; ss < POINT_SIZE; ss++) {
                    outData[ss].write(tmpBuff[stage][ss]);
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
