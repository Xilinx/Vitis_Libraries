
/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#include "back_transpose_simple.h"
#include <fstream>

int ceil(int x, int y) {
    return (((x + y - 1) / y) * y);
}

void dut() {
    using TT_STREAM_IN = backTransposeSimpleCls<POINT_SIZE, SSR>::TT_STREAM_IN;
    using TT_STREAM_OUT = backTransposeSimpleCls<POINT_SIZE, SSR>::TT_STREAM_OUT;
    using TT_SAMPLE = backTransposeSimpleCls<POINT_SIZE, SSR>::TT_SAMPLE;
    constexpr int SAMPLES_PER_READ = backTransposeSimpleCls<POINT_SIZE, SSR>::SAMPLES_PER_READ;
    constexpr int NSTREAM_IN = SSR;
    constexpr int NSTREAM_OUT = NSTREAM_IN * SAMPLES_PER_READ;
    TT_STREAM_IN sig_i[NSTREAM_IN];
    TT_STREAM_OUT sig_o[NSTREAM_IN];
    typedef ap_uint<32> real_32; // Equals two 'cint32' samples

    // Configure the same as 'host.cpp' for top level application:
    int curData1, curData2, curData3, curData4;
    TT_SAMPLE rdData1 = 0;
    TT_SAMPLE rdData2 = 0;

    // Load stream stimulus:
    unsigned numStores = backTransposeSimpleCls<POINT_SIZE, SSR>::numStores; // 7
    int ptSizeD1 = backTransposeSimpleCls<POINT_SIZE, SSR>::ptSizeD1;
    int ptSizeD2 = POINT_SIZE / ptSizeD1;
    int memSize = POINT_SIZE;

    TT_SAMPLE mem[memSize];
    TT_SAMPLE mem2d[ptSizeD2][ptSizeD1];
    TT_SAMPLE memOutProc[memSize];
    TT_SAMPLE mem2dTmp[ptSizeD2][ptSizeD1];
    int numReads = backTransposeSimpleCls<POINT_SIZE, SSR>::numRows * ptSizeD1; // 8 * 2
    TT_SAMPLE memOut[memSize];
    int NITER = 4;
    int ddTmp = 1;
    // create input memory to make comparison easier
    for (int mm = 0; mm < memSize; mm++) {
        if (mm < POINT_SIZE) {
            curData1 = ddTmp;
            curData2 = ddTmp++;
            mem[mm] = (real_32(curData2), real_32(curData1));
        } else {
            mem[mm] = (0, 0);
            mem[mm + 1] = (0, 0);
        }
    }

    for (int i = 0; i < NITER; i++) {
        for (int pp = 0; pp < POINT_SIZE / NSTREAM_IN; pp += SAMPLES_PER_READ) { // 1024
            for (int dd = 0; dd < NSTREAM_IN; dd++) {                            // 5
                rdData1 = mem[pp * NSTREAM_IN + dd];                             // 16+40+0 = 56
                rdData2 = mem[pp * NSTREAM_IN + dd + NSTREAM_IN];                // 16+40+0 = 56
                curData1 = rdData1 % (1 << 31);
                curData2 = rdData1 >> 32;
                curData3 = rdData2 % (1 << 31);
                curData4 = rdData2 >> 32;

                // sig_i[dd].write((real_32(curData4), real_32(curData3), real_32(curData2), real_32(curData1)));
                sig_i[dd].write((real_32(curData4), real_32(curData3), real_32(curData2), real_32(curData1)));
                std::cout << "input stream id: " << (dd) << "\tread data value 0 \t" << (curData1) << ", " << (curData2)
                          << ", " << (curData3) << ", " << (curData4) << "\n";
            }
        }
        back_transpose_core_wrapper(sig_i, sig_o);

        int rdPos = 0;
        for (int pp = 0; pp < POINT_SIZE / NSTREAM_IN / SAMPLES_PER_READ; pp++) { // 8
            rdPos = pp * NSTREAM_OUT;
            for (int dd = 0; dd < NSTREAM_IN; dd++) {  // 5
                (rdData1, rdData2) = sig_o[dd].read(); // o write into 0, 8,  16, 32
                memOut[rdPos] = rdData2;               // 0,  16, 32, 48,
                memOut[rdPos + NSTREAM_IN] = rdData1;  // 0,  16, 32, 48,
                rdPos++;
                std::cout << "ss " << (dd) << "\t[" << (rdData1 >> 32) << ", " << (rdData1 % (1 << 31)) << "\t["
                          << (rdData2 >> 32) << ", " << (rdData2 % (1 << 31)) << "\t\n";
            }
        } // POINT_SIZE

        // rearrange memory in 2D
        for (int rr = 0; rr < ptSizeD2; rr++) {
            for (int cc = 0; cc < ptSizeD1; cc++) {
                mem2d[rr][cc] = memOut[rr * ptSizeD1 + cc];
                // printf("rr = %d cc = %d mem = [%d, %d]\n", rr, cc, mem2d[rr][cc]  >> 32, mem2d[rr][cc] % (1 << 31));
            }
            // printf("\n");
        }

        for (int rr = 0; rr < ptSizeD1; rr++) {
            for (int cc = 0; cc < ptSizeD2; cc++) {
                memOutProc[rr * ptSizeD2 + cc] = mem2d[cc][rr];
            }
        }

        for (int dd = 0; dd < POINT_SIZE; dd++) {
            if (!(mem[dd] == memOutProc[dd])) {
                printf("error at %d ref = [%d, %d] output = [%d, %d]\n", dd, ((mem[dd] >> 32) % (1 << 31)),
                       ((mem[dd] >> 0) % (1 << 31)), ((memOutProc[dd] >> 32) % (1 << 31)),
                       ((memOutProc[dd] >> 0) % (1 << 31)));
            }
        }
    } // NITER
}

int main() {
    dut();
    return 0;
}
