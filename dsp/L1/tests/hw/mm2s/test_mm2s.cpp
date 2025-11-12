
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

#include "mm2s.cpp"
#include <fstream>
using namespace mm2s;
#define Q(x) #x
#define QUOTE(x) Q(x)
void dut() {
    TT_DATA ddr4_o[NITER][memSizeAct];
    TT_STREAM sig_o[NSTREAM];
    TT_DATA rd_data;
    constexpr int nbits = 128;
    // constexpr int DATAWIDTH = mm2s::DATAWIDTH;
    const int real_dataWidth = DATAWIDTH / 2;
    int numRealSamp = nbits / real_dataWidth;
    // Configure the same as 'host.cpp' for top level application:
    int data = 0;
    int cur_data1, cur_data2;
    int i = 0;
    TT_DATA inDataTmp;
    // Load stream stimulus:
    for (int i = 0; i < NITER; i++) {
        data = 0;
        for (int dd = 0; dd < memSizeAct; dd++) {
            for (int dsamp = 0; dsamp < numRealSamp; dsamp++) {
                if (dsamp == 0) {
                    inDataTmp = 0;
                }
                inDataTmp += (TT_DATA(data++) << (dsamp * real_dataWidth));
                printf("%ld check = %d \n", (data),
                       (inDataTmp >> (dsamp * real_dataWidth)) % (1 << (real_dataWidth - 1)));
            }
            ddr4_o[i][dd] = (inDataTmp);
        }
    }
#pragma HLS INTERFACE axis port = sig_o
    mm2s_wrapper(ddr4_o, sig_o);

    for (int ii = 0; ii < ((NITER * memSizeAct) / NSTREAM); ii++) {
        for (int dd = 0; dd < NSTREAM; dd++) {
            rd_data = sig_o[dd].read();
            for (int dsamp = 0; dsamp < numRealSamp; dsamp++) {
                printf("%ld  \t", (rd_data >> (dsamp * real_dataWidth)) % (1 << (real_dataWidth - 1)));
            }
        }
        printf("\n");
    }
}

int main() {
    dut();
    return 0;
}
