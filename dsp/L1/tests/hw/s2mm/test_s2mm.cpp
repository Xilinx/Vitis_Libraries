
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
    typedef ap_uint<32> real_32; // Equals two 'cint32' samples

    // Configure the same as 'host.cpp' for top level application:
    int NIER = 4;
    int data[NSTREAM];
    for (int s = 0; s < NSTREAM; s++) {
        data[s] = 0;
    }
    int stream;
    int cur_data1, cur_data2, cur_data3, cur_data4;
    // Load stream stimulus:
    for (int ll = 0; ll < NITER; ll++) {
        for (int s = 0; s < NSTREAM; s++) {
            data[s] = 0;
        }
        for (int dd = 0; dd < memSizeAct; dd++) {
            stream = dd % NSTREAM;
            cur_data1 = (stream * 1000) + data[stream]++;
            cur_data2 = (stream * 1000) + data[stream]++;
            cur_data3 = (stream * 1000) + data[stream]++;
            cur_data4 = (stream * 1000) + data[stream]++;
            std::cout << "data value 0 \t" << cur_data1 << "\tdata value 1\t" << cur_data2 << "data value 2 \t"
                      << cur_data3 << "\tdata value 3\t" << cur_data4 << "\n";
            sig_i[stream].write((real_32(cur_data4), real_32(cur_data3), real_32(cur_data2), real_32(cur_data1)));
        }
    }
    s2mm_wrapper(mem, sig_i);

    for (int ll = 0; ll < NITER; ll++) {
        for (int ii = 0; ii < (memSizeAct); ii++) {
            std::cout << "ii = " << ii << " mem[ll][ii] " << (mem[ll][ii] >> 96) << "\t"
                      << ((mem[ll][ii] >> 64) % (1 << 31)) << "\t" << ((mem[ll][ii] >> 32) % (1 << 31)) << "\t"
                      << ((mem[ll][ii]) % (1 << 31)) << "\n";
        }
    }
}

int main() {
    dut();
    return 0;
}