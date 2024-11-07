
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
#include "ifft_front_transpose.h"
#include <fstream>

int ceil(int x, int y) {
    return (((x + y - 1) / y) * y);
}
void dut() {
    using TT_STREAM = frontTransposeCls<POINT_SIZE, SSR>::TT_STREAM;
    using TT_SAMPLE = frontTransposeCls<POINT_SIZE, SSR>::TT_SAMPLE;
    constexpr int samplesPerRead = frontTransposeCls<POINT_SIZE, SSR>::samplesPerRead;
    constexpr int NSTREAM = SSR;
    TT_STREAM sig_i[NSTREAM], sig_o[NSTREAM];
    typedef ap_uint<32> real_32; // Equals two 'cint32' samples
    // Configure the same as 'host.cpp' for top level application:
    int NITER = 4;

    int loop_sel = 1;
    int cur_data1, cur_data2, cur_data3, cur_data4;
    TT_SAMPLE rd_data1, rd_data2;
    // Load stream stimulus:
    int data[NSTREAM] = {0};
    int new_data = 0;
    int ptSizeCeil = ceil(POINT_SIZE, NSTREAM);                         // 65
    unsigned numStores = frontTransposeCls<POINT_SIZE, SSR>::numStores; // 7
    int ptSizeD1 = frontTransposeCls<POINT_SIZE, SSR>::ptSizeD1;
    int ptSizeD2 = frontTransposeCls<POINT_SIZE, SSR>::ptSizeD2;
    int ptSizeD2Ceil = frontTransposeCls<POINT_SIZE, SSR>::ptSizeD2Ceil;
    int numRows = POINT_SIZE / ptSizeD1;
    int memSize = ptSizeD1 * ptSizeD2Ceil;

    TT_SAMPLE mem[memSize];
    TT_SAMPLE mem2d[ptSizeD2][ptSizeD1];
    TT_SAMPLE mem_ref[memSize];
    TT_SAMPLE mem2d_ref[ptSizeD2Ceil][ptSizeD1];
    int numReads = frontTransposeCls<POINT_SIZE, SSR>::numLoadsPtSize * ptSizeD1; // 8 * 2
    TT_SAMPLE mem_out[memSize];
    for (int ss = 0; ss < SSR; ss++) {
        data[ss] = 0;
    }
    int dd = 1;
    // create input memory to make comparison easier
    for (int mm = 0; mm < memSize; mm += 2) {
        if (mm < POINT_SIZE) {
            cur_data1 = dd;
            cur_data2 = dd++;
            cur_data3 = dd;
            cur_data4 = dd++;
            mem[mm] = (real_32(cur_data2), real_32(cur_data1));
            mem[mm + 1] = (real_32(cur_data4), real_32(cur_data3));
        } else {
            mem[mm] = (0, 0);
            mem[mm + 1] = (0, 0);
        }
    }

    // rearrange memory in 2D
    for (int rr = 0; rr < ptSizeD2; rr++) {
        for (int cc = 0; cc < ptSizeD1; cc++) {
            mem2d[rr][cc] = mem[rr + cc * ptSizeD2];
        }
    }

    for (int rr = 0; rr < ptSizeD2; rr++) {
        for (int cc = 0; cc < ptSizeD1; cc++) {
            printf("[%d, %d]\t", mem2d[rr][cc] >> 32, (mem2d[rr][cc] % (1 << 31)));
        }
        printf("\n");
    }

    for (int rr = 0; rr < ptSizeD2Ceil; rr++) {
        for (int cc = 0; cc < ptSizeD1; cc++) {
            mem_ref[rr * ptSizeD1 + cc] = mem2d[rr][cc];
        }
    }

    for (int ii = 0; ii < memSize; ii++) {
        printf("mem[%d] = [%d, %d]\n", ii, mem[ii] >> 32, (mem[ii] % (1 << 31)));
    }
    for (int ii = 0; ii < memSize; ii++) {
        printf("mem_ref[%d] = [%d, %d]\n", ii, mem_ref[ii] >> 32, (mem_ref[ii] % (1 << 31)));
    }
    printf("starting \n");
    for (int i = 0; i < NITER; i++) {
        for (int pp = 0; pp < ptSizeD1 * ptSizeD2Ceil / NSTREAM; pp += 2) { // 7
            for (int dd = 0; dd < NSTREAM; dd++) {                          // 5
                printf("index rd 1 = %d 2 = %d\n", dd + pp * NSTREAM, dd + NSTREAM + pp * NSTREAM);
                rd_data1 = mem[dd + pp * NSTREAM];           // 0, 10, 20 // 1, 11, 21
                rd_data2 = mem[dd + NSTREAM + pp * NSTREAM]; // 5, 15, 25 // 6,
                cur_data1 = rd_data1 % (1 << 31);
                cur_data2 = rd_data1 >> 32;
                cur_data3 = rd_data2 % (1 << 31);
                cur_data4 = rd_data2 >> 32;
                sig_i[dd].write((real_32(cur_data4), real_32(cur_data3), real_32(cur_data2), real_32(cur_data1)));
                std::cout << "input stream id: " << (dd) << "\tread data value 0 \t" << (cur_data1) << ", "
                          << (cur_data2) << "\tread data value 1\t" << (cur_data3) << ", " << (cur_data4) << "\n";
            }
        }
        ifft_front_transpose_wrapper(sig_i, sig_o);
        for (int ss = 0; ss < SSR; ss++) {
            data[ss] = 0;
        }
        for (int pp = 0; pp < ptSizeD1 * ptSizeD2Ceil / NSTREAM; pp += 2) { // 8
            for (int dd = 0; dd < NSTREAM; dd++) {                          // 5
                (rd_data1, rd_data2) = sig_o[dd].read();                    // o write into 0, 8,  16, 32
                int pos1 = data[dd]++;
                mem_out[dd * ptSizeD1 + pp % ptSizeD1 + (pp / ptSizeD1 * NSTREAM * ptSizeD1)] =
                    rd_data2; // 0,  16, 32, 48,
                pos1 = data[dd]++;
                mem_out[dd * ptSizeD1 + 1 + pp % ptSizeD1 + (pp / ptSizeD1 * NSTREAM * ptSizeD1)] = rd_data1; // 8
                // std::cout << "ss " << (dd) << "\t[" << (rd_data1 >> 32) << ", " << (rd_data1 % (1 << 31)) << "\t] ["
                // << (rd_data2 >> 32) << ", " << (rd_data2 % (1 << 31)) << "]\n";
            } // NSTREAM
        }     // POINT_SIZE
        for (int dd = 0; dd < POINT_SIZE; dd++) {
            if (!(mem_ref[dd] == mem_out[dd])) {
                printf("error at %d ref = [%d, %d] output = [%d, %d]\n", dd, mem_ref[dd] >> 32,
                       (mem_ref[dd] % (1 << 31)), mem_out[dd] >> 32, (mem_out[dd] % (1 << 31)));
            }
        }

    } // NITER
}

int main() {
    dut();
    return 0;
}
