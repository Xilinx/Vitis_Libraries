
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
void fft(dataType* in, dataType* out, unsigned int pointSize) {
    twidType twid;
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
    hls::stream<cfloat_set> inData[POINT_SIZE];
    hls::stream<cfloat_set> outData[POINT_SIZE];
    std::complex<float> in[NITER * POINT_SIZE], out[NITER * POINT_SIZE];
    std::complex<float> tmpData;
    std::complex<float> rdData1, rdData2;
    std::complex<float> err;
    cfloat_set writeData;
    cfloat_set readDataSet;

    for (int nn = 0; nn < NITER; nn += 2) {
        for (int i = 0; i < POINT_SIZE; i++) {
            tmpData.real(i * 1.0f);
            tmpData.real(i * 1.0f);
            writeData.data1 = tmpData;
            writeData.data2 = tmpData;
            inData[i].write(writeData);
            in[nn * POINT_SIZE + i] = tmpData;
            in[nn * POINT_SIZE + POINT_SIZE + i] = tmpData;
        }
        fft<std::complex<float>, std::complex<float> >(&in[nn * POINT_SIZE], &out[nn * POINT_SIZE], POINT_SIZE);
        fft<std::complex<float>, std::complex<float> >(&in[(nn + 1) * POINT_SIZE], &out[(nn + 1) * POINT_SIZE],
                                                       POINT_SIZE);
        ssr_fft_wrapper(inData, outData);
        for (int i = 0; i < POINT_SIZE; i++) {
            readDataSet = outData[i].read();
            rdData1 = readDataSet.data1;
            rdData2 = readDataSet.data2;
            err.real(rdData1.real() - out[nn * POINT_SIZE + i].real());
            err.imag(rdData1.imag() - out[nn * POINT_SIZE + i].imag());
            if (err.real() > 0.00005 || err.real() < -0.00005 || err.imag() > 0.00005 || err.imag() < -0.00005) {
                printf("act = %f, %f golden = %f, %f error = %f, %f\n", rdData1.real(), rdData1.imag(),
                       out[nn * POINT_SIZE + i].real(), out[nn * POINT_SIZE + i].imag(),
                       rdData1.real() - out[nn * POINT_SIZE + i].real(),
                       rdData1.imag() - out[nn * POINT_SIZE + i].imag());
                return 1;
            }
            err.real(rdData2.real() - out[(nn + 1) * POINT_SIZE + i].real());
            err.imag(rdData2.imag() - out[(nn + 1) * POINT_SIZE + i].imag());
            if (err.real() > 0.00005 || err.real() < -0.00005 || err.imag() > 0.00005 || err.imag() < -0.00005) {
                printf("act = %f, %f golden = %f, %f error = %f, %f\n", rdData2.real(), rdData2.imag(),
                       out[nn * POINT_SIZE + i].real(), out[nn * POINT_SIZE + i].imag(),
                       rdData2.real() - out[nn * POINT_SIZE + i].real(),
                       rdData2.imag() - out[nn * POINT_SIZE + i].imag());
                return 1;
            }
        }
    }
    return 0;
}