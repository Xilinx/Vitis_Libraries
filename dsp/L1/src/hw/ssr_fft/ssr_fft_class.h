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
#ifndef SSR_FFT_CLASS
#define SSR_FFT_CLASS
#include <complex>
#include <hls_stream.h>
#include "math.h"
#include <ap_fixed.h>

struct cint16 {
    int16_t real;
    int16_t imag;
};
struct cint32 {
    int32_t real;
    int32_t imag;
};
struct cfloat {
    float real;
    float imag;
};

//#define __SSR_FFT_DEBUG__

template <typename t_data_real, typename t_twid_real>
void butterfly(std::complex<t_data_real>* inData1,
               std::complex<t_data_real>* inData2,
               std::complex<t_twid_real>* curTwid,
               std::complex<t_data_real>* outData1,
               std::complex<t_data_real>* outData2) {
#pragma HLS inline
    if
        constexpr(std::is_same<t_data_real, float>::value) {
            std::complex<float> prod;
#pragma HLS BIND_OP variable = prod op = fmul impl = maxdsp
#pragma HLS BIND_OP variable = prod op = fadd impl = maxdsp
            prod = (*inData2) * (*curTwid);
            *outData1 = (*inData1) + prod;
            *outData2 = (*inData1) - prod;
        }
    else {
        static constexpr unsigned kdataSize = sizeof(t_data_real) * 8;   // 32
        static constexpr unsigned kTwidSize = sizeof(t_twid_real) * 8;   // 32
        static constexpr unsigned kpartProdSize = kdataSize + kTwidSize; // 63
        static constexpr unsigned kpartSumSize = kdataSize + 1;          // 33
        static constexpr unsigned kProdSize = kpartProdSize + 1;
        static constexpr unsigned kRoundConst = ((int64_t)1 << (kTwidSize - 1)) - 1;
        std::complex<ap_int<kProdSize> > prod;
        ap_int<kpartProdSize> pprod1, pprod2, pprod3, pprod4;
        ap_int<kpartSumSize> psum1, psum2, psum3, psum4;
        ap_int<kProdSize> prod1_real, prod1_imag, prod2_real, prod2_imag;
        ap_int<kdataSize> prod_shift_real, prod_shift_imag;

        pprod1 = (ap_int<kpartProdSize>)(*inData2).real() * (ap_int<kpartProdSize>)(*curTwid).real();
        pprod2 = (ap_int<kpartProdSize>)(*inData2).imag() * (ap_int<kpartProdSize>)(*curTwid).imag();
        pprod3 = (ap_int<kpartProdSize>)(*inData2).real() * (ap_int<kpartProdSize>)(*curTwid).imag();
        pprod4 = (ap_int<kpartProdSize>)(*inData2).imag() * (ap_int<kpartProdSize>)(*curTwid).real();
        prod1_real = pprod1 - pprod2;
        prod1_imag = pprod3 + pprod4;
        prod_shift_real = (prod1_real + kRoundConst) >> (kTwidSize - 1);
        prod_shift_imag = (prod1_imag + kRoundConst) >> (kTwidSize - 1);
        psum1 = ((ap_int<kpartSumSize>)((*inData1).real()) + (ap_int<kpartSumSize>)prod_shift_real) >> 1;
        psum2 = ((ap_int<kpartSumSize>)((*inData1).imag()) + (ap_int<kpartSumSize>)prod_shift_imag) >> 1;
        psum3 = ((ap_int<kpartSumSize>)((*inData1).real()) - (ap_int<kpartSumSize>)(prod_shift_real) >> 1);
        psum4 = ((ap_int<kpartSumSize>)((*inData1).imag()) - (ap_int<kpartSumSize>)(prod_shift_imag) >> 1);
        (*outData1).real(psum1); // = ((*inData1) + prod)>>1;
        (*outData1).imag(psum2); // = ((*inData1) + prod)>>1;
        //*outData1.real() = ((*inData1) + prod)>>1);
        (*outData2).real(psum3);
        (*outData2).imag(psum4);
    }
}

template <typename TT_DATA, typename TT_TWIDDLE, unsigned int TP_FFT_IFFT>
class ssrFFTClass {
   private:
   public:
    static constexpr int nBtfls = POINT_SIZE / 2;
    static constexpr unsigned int nStages =
        POINT_SIZE == 64 ? 6
                         : POINT_SIZE == 32 ? 5 : POINT_SIZE == 16 ? 4 : POINT_SIZE == 8 ? 3 : POINT_SIZE == 4 ? 2 : 0;
    typedef typename std::conditional<
        std::is_same<TT_DATA, cint32>::value,
        int32_t,
        typename std::conditional<std::is_same<TT_DATA, cint16>::value, int16_t, float>::type>::type TT_DATA_REAL;
    typedef typename std::conditional<
        std::is_same<TT_TWIDDLE, cint32>::value,
        int32_t,
        typename std::conditional<std::is_same<TT_TWIDDLE, cint16>::value, int16_t, float>::type>::type TT_TWIDDLE_REAL;
    // using TT_DATA_REAL = typename std::conditional<std::is_same<TT_DATA, cint32>::value, int32_t,
    // std::conditional<std::is_same<TT_DATA, cint16>::value, int16_t, float>>::type;
    typedef std::complex<TT_DATA_REAL> T_DATA_INT;
    std::complex<TT_DATA_REAL> scBuff[nStages][nBtfls];
    typedef hls::stream<T_DATA_INT> TT_STREAM;
    void ssr_fft(TT_STREAM inData[POINT_SIZE], TT_STREAM outData[POINT_SIZE]) {
#pragma HLS inline
        int stride;
        T_DATA_INT curData1, curData2;
        std::complex<TT_TWIDDLE_REAL> twid __attribute((no_ctor));
        std::complex<TT_DATA_REAL> tmpBuff1[nStages][POINT_SIZE] __attribute((no_ctor));
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
                    curData1 = tmpBuff1[stage - 1][rdPos1];
                    curData2 = tmpBuff1[stage - 1][rdPos2];
                }
                twid = scBuff[stage][btfly];
                if (stage == 0) {
                    butterfly<TT_DATA_REAL, TT_TWIDDLE_REAL>(&curData1, &curData2, &twid, &tmpBuff1[stage][wrPos1],
                                                             &tmpBuff1[stage][wrPos2]);
                } else {
                    butterfly<TT_DATA_REAL, TT_TWIDDLE_REAL>(&curData1, &curData2, &twid, &tmpBuff1[stage][wrPos1],
                                                             &tmpBuff1[stage][wrPos2]);
                }
            }
            if (stage == nStages - 1) {
                for (int ss = 0; ss < POINT_SIZE; ss++) {
                    T_DATA_INT outDataTmp;
                    outDataTmp = tmpBuff1[stage][ss];
                    outData[ss].write(outDataTmp);
                }
            }
        }
    }
    ssrFFTClass() {
        int inv = TP_FFT_IFFT == 1 ? -1 : 1;
        TT_TWIDDLE_REAL angle_in_fixed;
        double realD;
        double imagD;
        TT_TWIDDLE_REAL cos_int;
        TT_TWIDDLE_REAL sin_int;
#define PI 3.1415926
        for (int st = 0; st < nStages; st++) {
            int stride = nBtfls >> st;            // 2, 1
            for (int bt = 0; bt < nBtfls; bt++) { // 0, 1
                float realf, imagf;
                TT_TWIDDLE_REAL real, imag;
                float angle = inv * 2.0f * (float)(PI * (float)((float)(bt / stride) * stride)) / (float)POINT_SIZE;
                realf = (float)cosf(angle);
                imagf = (float)sinf(angle);
                if (!std::is_same<TT_TWIDDLE_REAL, float>()) {
                    double scaleFactor = std::is_same<TT_TWIDDLE_REAL, int16_t>::value ? 32768.0 : 2147483648.0;
                    double maxVal = std::is_same<TT_TWIDDLE_REAL, int16_t>::value ? 32767.0 : 2147483647.0;
                    realD = ((float)((float)realf * (double)scaleFactor));
                    imagD = ((float)((float)imagf * (double)scaleFactor));
                    if (realD > scaleFactor - 1) {
                        cos_int = scaleFactor - 1;
                    } else {
                        cos_int = (TT_TWIDDLE_REAL)(realD);
                    }
                    if (imagD > scaleFactor - 1) {
                        sin_int = scaleFactor - 1;
                    } else {
                        sin_int = (TT_TWIDDLE_REAL)(imagD);
                    }
                    scBuff[st][bt].real(cos_int);
                    scBuff[st][bt].imag(sin_int);
                } else {
                    scBuff[st][bt].real(realf);
                    scBuff[st][bt].imag(imagf);
                }
            }
        }
    }
};
#endif