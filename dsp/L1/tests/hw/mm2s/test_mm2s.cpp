
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

void dut() {
    TT_DATA ddr4_o[NITER][memSizeAct];
    TT_STREAM sig_o[NSTREAM];
    TT_DATA rd_data;
    // Configure the same as 'host.cpp' for top level application:
    int data = 0;
    int cur_data1, cur_data2;
    int i = 0;
    // Load stream stimulus:
    for (int i = 0; i < NITER; i++) {
        for (int dd = 0; dd < memSizeAct; dd++) {
            cur_data1 = data++;
            cur_data2 = data++;
            std::cout << "input data value 0 \t" << cur_data1 << "\tdata value 1\t" << cur_data2 << "\n";
            ddr4_o[i][dd] = ((TT_SAMPLE(cur_data2), TT_SAMPLE(cur_data1)));
        }
    }
#pragma HLS INTERFACE axis port = sig_o
    mm2s_wrapper(ddr4_o, sig_o);

    for (int ii = 0; ii < ((NITER * memSizeAct) / NSTREAM); ii++) {
        for (int dd = 0; dd < NSTREAM; dd++) {
            rd_data = sig_o[dd].read();
            std::cout << "STREAM " << dd << "data value 0 \t" << ((rd_data << 0) >> 96) << "\tdata value 1\t"
                      << ((rd_data << 32) >> 96) << "\tdata value 2 \t" << ((rd_data << 64) >> 96) << "\tdata value 3\t"
                      << ((rd_data << 96) >> 96) << "\n";
        }
    }
}

int main() {
    dut();
    return 0;
}
