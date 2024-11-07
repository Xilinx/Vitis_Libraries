
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

#include "ifft_transpose.h"
//#include <fstream>

void dut() {
    typedef ap_uint<32> real_32; // Equals two 'cint32' samples
    using TT_STREAM = midTransposeCls<POINT_SIZE, SSR>::TT_STREAM;
    using TT_SAMPLE = midTransposeCls<POINT_SIZE, SSR>::TT_SAMPLE;
    TT_STREAM sig_i[SSR], sig_o[SSR];
    TT_SAMPLE rd_data1, rd_data2;
    // Configure the same as 'host.cpp' for top level application:
    int NITER = 4;
    int data1, data2, data3, data4;
    data1 = 0;
    data2 = 0;
    data3 = 0;
    data4 = 0;
    int data[SSR];
    for (int s = 0; s < SSR; s++) {
        data[s] = 0;
    }
    int cur_data1, cur_data2, cur_data3, cur_data4;
    // Load stream stimulus:
    int ptSizeD1 = midTransposeCls<POINT_SIZE, SSR>::ptSizeD1;
    int ptSizeD2 = midTransposeCls<POINT_SIZE, SSR>::ptSizeD2;
    int ptSizeD1Ceil = midTransposeCls<POINT_SIZE, SSR>::ptSizeD1Ceil;
    int ptSizeD1CeilRd = midTransposeCls<POINT_SIZE, SSR>::ptSizeD1CeilRd;
    int ptSizeD2Ceil = midTransposeCls<POINT_SIZE, SSR>::ptSizeD2Ceil;

    TT_SAMPLE memIn[NITER][ptSizeD2Ceil][ptSizeD1];
    TT_SAMPLE memRef[NITER][ptSizeD1Ceil][ptSizeD2Ceil];
    TT_SAMPLE memOut[NITER][ptSizeD1Ceil][ptSizeD2];
    std::cout << "ptSizeD1 " << ptSizeD1 << " ptSizeD2 " << ptSizeD2 << " ptSizeD1Ceil " << ptSizeD1Ceil
              << " ptSizeD2Ceil " << ptSizeD2Ceil << "\n";
    for (int nn = 0; nn < NITER; nn++) {
        for (int s = 0; s < SSR; s++) {
            data[s] = 0;
        }
        for (int d2 = 0; d2 < ptSizeD2Ceil; d2++) { // 64 * 65
            if (d2 < ptSizeD2) {
                for (int d1 = 0; d1 < ptSizeD1; d1 += 2) {
                    printf("d1 = %d d2 = %d\n", d1, d2);
                    if (d1 < ptSizeD1) {
                        int stream = d2 % SSR;
                        cur_data1 = 1000 * stream + data[stream]++;
                        cur_data2 = 1000 * stream + data[stream]++;
                        cur_data3 = 1000 * stream + data[stream]++;
                        cur_data4 = 1000 * stream + data[stream]++;
                        sig_i[stream].write(
                            (real_32(cur_data4), real_32(cur_data3), real_32(cur_data2), real_32(cur_data1)));
                        memIn[nn][d2][d1] = (real_32(cur_data2), real_32(cur_data1));
                        memIn[nn][d2][d1 + 1] = (real_32(cur_data4), real_32(cur_data3));
                        std::cout << "tb write\n";
                        std::cout << "stream = " << stream << "data value 0 \t" << cur_data1 << "\tdata value 1\t"
                                  << cur_data2 << "\tdata value 2 \t" << cur_data3 << "\tdata value 3\t" << cur_data4
                                  << "\n";
                    } else {
                        int stream = d2 % SSR;
                        cur_data1 = 0;
                        cur_data2 = 0;
                        cur_data3 = 0;
                        cur_data4 = 0;
                        memIn[nn][d2][d1] = (real_32(cur_data2), real_32(cur_data1));
                        memIn[nn][d2][d1 + 1] = (real_32(cur_data4), real_32(cur_data3));
                        sig_i[stream].write(
                            (real_32(cur_data4), real_32(cur_data3), real_32(cur_data2), real_32(cur_data1)));
                        std::cout << "tb write\n";
                        std::cout << "data value 0 \t" << cur_data1 << "\tdata value 1\t" << cur_data2
                                  << "\tdata value 2 \t" << cur_data3 << "\tdata value 3\t" << cur_data4 << "\n";
                    }
                }
            } else {
                for (int d1 = 0; d1 < ptSizeD1; d1 += 2) {
                    int stream = d2 % SSR;
                    cur_data1 = 0;
                    cur_data2 = 0;
                    cur_data3 = 0;
                    cur_data4 = 0;
                    memIn[nn][d2][d1] = (real_32(cur_data2), real_32(cur_data1));
                    memIn[nn][d2][d1 + 1] = (real_32(cur_data4), real_32(cur_data3));
                    sig_i[stream].write(
                        (real_32(cur_data4), real_32(cur_data3), real_32(cur_data2), real_32(cur_data1)));
                    std::cout << "tb write\n";
                    std::cout << "data value 0 \t" << cur_data1 << "\tdata value 1\t" << cur_data2
                              << "\tdata value 2 \t" << cur_data3 << "\tdata value 3\t" << cur_data4 << "\n";
                }
            }
        }

        ifft_transpose_wrapper(sig_i, sig_o);

        for (int d1 = 0; d1 < ptSizeD1CeilRd; d1++) {
            for (int d2 = 0; d2 < ptSizeD2; d2 += 2) {
                printf("nn = %d d1 = %d d2 = %d\n", nn, d1, d2);
                int stream = d1 % SSR;
                (rd_data2, rd_data1) = sig_o[stream].read();
                memOut[nn][d1][d2] = rd_data1;
                memOut[nn][d1][d2 + 1] = rd_data2;
                std::cout << "tb read\n";
                std::cout << "data value 0 \t" << (rd_data1 >> 32) << "\tdata value 1\t" << (rd_data1 % (1 << 31))
                          << "\tdata value 2 \t" << (rd_data2 >> 32) << "\tdata value 3\t" << (rd_data2 % (1 << 31))
                          << "\n";
            }
        }

        // create expected output matrix
        for (int d1 = 0; d1 < ptSizeD1; d1++) {
            for (int d2 = 0; d2 < ptSizeD2; d2++) {
                memRef[nn][d1][d2] = memIn[nn][d2][d1];
                if (memOut[nn][d1][d2] != memRef[nn][d1][d2]) {
                    printf("Error at [%d][%d], Expected: [%d, %d] Got [%d, %d]\n", d1, d2, (memRef[nn][d1][d2] >> 32),
                           (memRef[nn][d1][d2] % (1 << 31)), (memOut[nn][d1][d2] >> 32),
                           (memOut[nn][d1][d2] % (1 << 31)));
                }
            }
        }
    }
}

int main() {
    dut();
    return 0;
}
