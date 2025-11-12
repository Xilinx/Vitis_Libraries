
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

#include "s2mm.cpp"
#include <fstream>
using namespace s2mm;

void dut() {
    TT_DATA mem[NITER][memSizeAct];
    TT_STREAM sig_i[NSTREAM];
    const int real_dataWidth = DATAWIDTH / 2;
    typedef ap_uint<real_dataWidth> real_dtype; // Equals two 'cint32' samples
    TT_DATA inDataTmp;
    constexpr int nbits = 128;
    // Configure the same as 'host.cpp' for top level application:
    int data[(nbits / DATAWIDTH) * NSTREAM];
    for (int s = 0; s < NSTREAM; s++) {
        data[s] = 0;
    }
    int stream;
    int numRealSamp = nbits / real_dataWidth;
    real_dtype cur_data[numRealSamp];
    // Load stream stimulus:
    for (int ll = 0; ll < NITER; ll++) {
        for (int s = 0; s < NSTREAM; s++) {
            data[s] = 0;
        }
        for (int dd = 0; dd < memSizeAct; dd++) {
            stream = dd % NSTREAM;
            for (int dsamp = 0; dsamp < numRealSamp; dsamp++) {
                if (dsamp == 0) {
                    inDataTmp = 0;
                }
                inDataTmp += (TT_DATA((stream * 1000) + data[stream]++) << (dsamp * real_dataWidth));
                printf("%ld check = %d \n", ((stream * 1000) + data[stream]),
                       (inDataTmp >> (dsamp * real_dataWidth)) % (1 << (real_dataWidth - 1)));
            }
            printf("\nwrote ");
            for (int dsamp = 0; dsamp < numRealSamp; dsamp++) {
                // inDataTmp += real_dtype((stream*1000) + data[stream]++) << (dsamp*real_dataWidth);
                printf("%d\t", (inDataTmp >> (dsamp * real_dataWidth)) % (1 << (real_dataWidth - 1)));
            }
            printf("\n");
            // inDataTmp = ;
            // cur_data1 = (stream*1000) + data[stream]++;
            // cur_data2 = (stream*1000) + data[stream]++;
            // cur_data3 = (stream*1000) + data[stream]++;
            // cur_data4 = (stream*1000) + data[stream]++;
            // std::cout << "data value 0 \t" << cur_data1 << "\tdata value 1\t" << cur_data2 <<  "data value 2 \t" <<
            // cur_data3 << "\tdata value 3\t" << cur_data4 <<"\n";
            sig_i[stream].write(inDataTmp);
        }
    }
    s2mm_wrapper(mem, sig_i);

    for (int ll = 0; ll < NITER; ll++) {
        for (int ii = 0; ii < (memSizeAct); ii++) {
            for (int subSamp = 0; subSamp < numRealSamp; subSamp++) {
                printf("%d\t", (mem[ll][ii] >> (subSamp * real_dataWidth)) % (1 << (real_dataWidth - 1)));
            }
            printf("\n");
            // std::cout << "ii = " << ii << " mem[ll][ii] " << (mem[ll][ii] >> 96) << "\t" << ((mem[ll][ii] >> 64) % (1
            // << 31)) << "\t" << ((mem[ll][ii] >> 32) % (1 << 31)) << "\t" << ((mem[ll][ii]) % (1 << 31)) << "\n";
        }
    }
}

int main() {
    dut();
    return 0;
}