
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
#include "mid_transpose.h"
#include <fstream>

int ceil(int x, int y) {
    return (((x + y - 1) / y) * y);
}

void dut() {
    using TT_STREAM = midTransposeCls<POINT_SIZE, SSR, DATAWIDTH>::TT_STREAM;
    using TT_SAMPLE = midTransposeCls<POINT_SIZE, SSR, DATAWIDTH>::TT_SAMPLE;
    using TT_DATA = midTransposeCls<POINT_SIZE, SSR, DATAWIDTH>::TT_DATA;
    constexpr int samplesPerRead = midTransposeCls<POINT_SIZE, SSR, DATAWIDTH>::samplesPerRead;
    TT_STREAM sig_i[SSR];
    TT_STREAM sig_o[SSR];
    typedef ap_uint<(DATAWIDTH / 2)> real_dtype;
    // Configure the same as 'host.cpp' for top level application:
    int curData1, curData2, curData3, curData4;
    TT_SAMPLE rdData1 = 0;
    TT_SAMPLE rdData2 = 0;

    // Load stream stimulus:
    int ptSizeD1 = midTransposeCls<POINT_SIZE, SSR, DATAWIDTH>::XDIM;
    int ptSizeD2 = POINT_SIZE / ptSizeD1;
    int memSize = POINT_SIZE;

    TT_SAMPLE mem[memSize];
    TT_SAMPLE memIn2d[ptSizeD2][ptSizeD1];
    TT_SAMPLE memOut2d[ptSizeD2][ptSizeD1];
    int NITER = 8;
    int ddTmp = 1;
    int ssrIn = SSR * samplesPerRead;
    // create input memory to make comparison easier
    for (int mm = 0; mm < memSize; mm++) {
        if (mm < POINT_SIZE) {
            curData1 = ddTmp;
            curData2 = ddTmp++;
            mem[mm] = (real_dtype(curData2), real_dtype(curData1));
        } else {
            mem[mm] = (0, 0);
            mem[mm + 1] = (0, 0);
        }
    }

    for (int i = 0; i < NITER; i++) {
        for (int pp = 0; pp < POINT_SIZE / SSR / samplesPerRead; pp++) {
#pragma HLS PIPELINE II = 1                    // 1024
            for (int dd = 0; dd < SSR; dd++) { // 5
                TT_DATA tmpData = 0;
                for (int samp = 0; samp < samplesPerRead; samp++) {
                    int curssr = dd * samplesPerRead + samp;
                    memIn2d[curssr + (pp / ptSizeD1) * SSR * samplesPerRead][(pp % ptSizeD1)] =
                        mem[curssr + (pp % ptSizeD1) * ptSizeD2 + (pp / ptSizeD1) * SSR * samplesPerRead];
                    tmpData += (TT_DATA)(
                        (TT_DATA)(mem[(pp / ptSizeD1) * SSR * samplesPerRead + (pp % ptSizeD1) * ptSizeD2 + curssr])
                        << (samp * DATAWIDTH));
                }
                sig_i[dd].write(tmpData);
            }
        }

        // for(int pp = 0; pp < POINT_SIZE/SSR; pp+=samplesPerRead){  // 1024
        mid_transpose_wrapper(sig_i, sig_o);
        //}
    }

    for (int i = 0; i < NITER - 1; i++) {
        // int rdPos = 0;
        for (int pp = 0; pp < POINT_SIZE / SSR / samplesPerRead; pp++) {
            // rdPos = pp%(ptSizeD2)*ptSizeD1 + pp/ptSizeD2*SSR;
            for (int dd = 0; dd < SSR; dd++) {
                if (!sig_o[0].empty()) {
                    TT_DATA tmpData = sig_o[dd].read();
                    // printf("reading in tb\n");
                    for (int samp = 0; samp < samplesPerRead; samp++) {
                        int curssr = dd * samplesPerRead + samp;
                        TT_SAMPLE tmpSamp = (tmpData >> (samp * DATAWIDTH)) % (TT_DATA)(1 << (TT_DATA)(DATAWIDTH - 1));
                        memOut2d[pp % ptSizeD2][(curssr % ptSizeD1) + (pp / ptSizeD2) * SSR * samplesPerRead] =
                            tmpSamp; // tmpData % (TT_DATA)(1 << (TT_DATA)(samp*DATAWIDTH));
                        // printf("memOut2D[%ld][%ld] = %ld, %ld from %ld, %ld\n", (pp)%ptSizeD2, (curssr %
                        // ptSizeD1)+(pp/ptSizeD2)*SSR*samplesPerRead, (long)(memOut2d[pp%ptSizeD2][(curssr %
                        // ptSizeD1)+(pp/ptSizeD2)*SSR*samplesPerRead]>>DATAWIDTH/2),
                        // (long)(memOut2d[pp%ptSizeD2][(curssr %
                        // ptSizeD1)+(pp/ptSizeD2)*SSR*samplesPerRead]%(1<<(DATAWIDTH/2-1))), (long)(tmpSamp >>
                        // DATAWIDTH/2), (long)(tmpSamp % (1 << (DATAWIDTH/2-1))));
                        // rdPos;
                    }
                }
                // rdPos++;
                //(rdData1) = sig_o[dd].read();
                // printf("ss = %ld\t %ld, %ld\n", dd, (rdData1 >> 32), (rdData1 % (1 << 31)));
            }
        } // POINT_SIZE
        for (int d2 = 0; d2 < ptSizeD2; d2++) {
            for (int d1 = 0; d1 < ptSizeD1; d1++) {
                if (!(memOut2d[d2][d1] == memIn2d[d2][d1])) {
                    printf("iter = %d: memin[%d][%d] = %ld, %ld\t memout[%d][%d] = %ld, %ld\n", i, d2, d1,
                           (long)(memIn2d[d2][d1] >> (DATAWIDTH / 2)),
                           (long)(memIn2d[d2][d1] % (1 << (DATAWIDTH / 2 - 1))), d2, d1,
                           (long)(memOut2d[d2][d1] >> (DATAWIDTH / 2)),
                           (long)(memOut2d[d2][d1] % (1 << (DATAWIDTH / 2 - 1))));
                }
            }
        }

    } // NITER
}

int main() {
    dut();
    return 0;
}
