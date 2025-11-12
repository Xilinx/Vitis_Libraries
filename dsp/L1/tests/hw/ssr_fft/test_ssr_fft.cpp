
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

#include <iostream>
#include <stdlib.h>
#include "ssr_fft.h"

unsigned int bitReverse(unsigned int x, int log2n) {
    int n = 0;
    int mask = 0x1;
    for (int i = 0; i < log2n; i++) {
        n <<= 1;
        n |= (x & 1);
        x >>= 1;
    }
    return n;
}
template <typename dataType, typename twidType>
void fft(std::complex<dataType>* in, std::complex<dataType>* out, unsigned int pointSize) {
    long roundConst = std::is_same<twidType, int16_t>::value ? 32767 : 2147483647;
    std::complex<twidType> twid;
    for (int i = 0; i < pointSize; ++i) {
        out[i].real(0);
        out[i].imag(0);
        for (int j = 0; j < pointSize; ++j) {
            double scaleFactor = std::is_same<twidType, int16_t>::value ? 32768.0 : 2147483648.0;
            double cosD = round((cos(-1 * M_PI * 2.0 * (double)i * (double)j / (double)pointSize)) * scaleFactor);
            double sinD = round((sin(-1 * M_PI * 2.0 * (double)i * (double)j / (double)pointSize)) * scaleFactor);
            double maxTwid = scaleFactor - 1;
            if (cosD > maxTwid) {
                twid.real((twidType)maxTwid);
            } else {
                twid.real((twidType)cosD);
            }
            if (sinD > maxTwid) {
                twid.imag((twidType)maxTwid);
            } else {
                twid.imag((twidType)sinD);
            }
            //   printf("twiddle %d, %d\n", twid.real(), twid.imag());
            long inRotReal = (long)in[j].real() * (long)twid.real() - ((long)in[j].imag() * (long)twid.imag());
            long inRotImag = (long)in[j].real() * (long)twid.imag() + ((long)in[j].imag() * (long)twid.real());
            // do rounding and shifting
            //   printf("raw mult output %ld, %ld\n", inRotReal, inRotImag);
            inRotReal = (long)(inRotReal + roundConst) >> (sizeof(twidType) * 8 - 1);
            inRotImag = (long)(inRotImag + roundConst) >> (sizeof(twidType) * 8 - 1);
            //   printf("after rounding %ld, %ld\n", inRotReal, inRotImag);
            //   printf("after adding %ld, %ld\n", (out[i].real() + (inRotReal)), (out[i].imag() + (inRotImag)));
            // twid.real((cos(-1 * M_PI * 2.0 * (double)i * (double)j / (double)pointSize)));
            // twid.imag((sin(-1 * M_PI * 2.0 * (double)i * (double)j / (double)pointSize)));
            out[i].real((out[i].real() + (inRotReal)));
            out[i].imag((out[i].imag() + (inRotImag)));
            // printf("out[%d] = %d, %d\n", i, out[i].real(), out[i].imag());
        }
        out[i].real(out[i].real() / (pointSize));
        out[i].imag(out[i].imag() / (pointSize));
    }
}

template <>
void fft<float, float>(std::complex<float>* in, std::complex<float>* out, unsigned int pointSize) {
    std::complex<float> twid;
    for (int i = 0; i < pointSize; ++i) {
        out[i].real(0);
        out[i].imag(0);
        for (int j = 0; j < pointSize; ++j) {
            twid.real((cos(-1 * M_PI * 2.0 * (double)i * (double)j / (double)pointSize)));
            twid.imag((sin(-1 * M_PI * 2.0 * (double)i * (double)j / (double)pointSize)));
            out[i].real(out[i].real() + in[j].real() * twid.real() - (in[j].imag() * twid.imag()));
            out[i].imag(out[i].imag() + in[j].real() * twid.imag() + (in[j].imag() * twid.real()));
        }
    }
}

int main() {
    // using data_set = ssrFFTClass<DATA_TYPE, TWIDDLE_TYPE>::data_set;
    using TT_STREAM = ssrFFTClass<DATA_TYPE, TWIDDLE_TYPE>::TT_STREAM;
    using TT_DATA_REAL = ssrFFTClass<DATA_TYPE, TWIDDLE_TYPE>::TT_DATA_REAL;
    using TT_TWIDDLE_REAL = ssrFFTClass<DATA_TYPE, TWIDDLE_TYPE>::TT_TWIDDLE_REAL;
    TT_STREAM inData[POINT_SIZE];
    TT_STREAM outData[POINT_SIZE];
    std::complex<TT_DATA_REAL> in[NITER * POINT_SIZE], out[NITER * POINT_SIZE];
    std::complex<TT_DATA_REAL> tmpData;
    std::complex<TT_DATA_REAL> rdData1, rdData2;
    std::complex<TT_DATA_REAL> err;

    for (int nn = 0; nn < NITER; nn += 1) {
        for (int i = 0; i < POINT_SIZE; i++) {
            // TT_DATA_REAL
            if (i == 0) {
                tmpData.imag(0x7ffe);
                tmpData.real(0x7ffe);
            }
            if (i == 1) {
                tmpData.imag(0x7ffe);
                tmpData.real(0x7ffe);
            }
            if (i == 2) {
                tmpData.imag(0x7ffe);
                tmpData.real(0x7ffe);
            }
            if (i == 3) {
                tmpData.imag(0x7ffe);
                tmpData.real(0x7ffe);
            }
            // tmpData.real(4.0f);
            inData[i].write(tmpData);
            in[nn * POINT_SIZE + i] = tmpData;
            // in[nn * POINT_SIZE + POINT_SIZE + i] = tmpData;
        }
        fft<TT_DATA_REAL, TT_TWIDDLE_REAL>(&in[nn * POINT_SIZE], &out[nn * POINT_SIZE], POINT_SIZE);
        // fft<std::complex<DATA_TYPE>, std::complex<DATA_TYPE>>(&in[(nn + 1)* POINT_SIZE], &out[(nn + 1) * POINT_SIZE],
        // POINT_SIZE);
        ssr_fft_wrapper(inData, outData);
        for (int i = 0; i < POINT_SIZE; i++) {
            rdData1 = outData[i].read();
            err.real(rdData1.real() - out[nn * POINT_SIZE + i].real());
            err.imag(rdData1.imag() - out[nn * POINT_SIZE + i].imag());
            if (err.real() > 0.0005 || err.real() < -0.0005 || err.imag() > 0.0005 || err.imag() < -0.0005) {
                printf("i = %d act = %d, %d golden = %d, %d error = %d, %d\n", i, rdData1.real(), rdData1.imag(),
                       out[nn * POINT_SIZE + i].real(), out[nn * POINT_SIZE + i].imag(),
                       rdData1.real() - out[nn * POINT_SIZE + i].real(),
                       rdData1.imag() - out[nn * POINT_SIZE + i].imag());
                // return 1;
            }
        }
    }
    return 0;
}