/**
* Copyright (C) 2019-2021 Xilinx, Inc
*
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

extern "C" {

void data_mover(ap_uint<32>* mem_out,
                ap_uint<32>* mem_in,
                hls::stream<qdma_axis<32, 0, 0, 0> >& out_lower,
                hls::stream<qdma_axis<32, 0, 0, 0> >& out_higher,
                hls::stream<qdma_axis<32, 0, 0, 0> >& in_lower,
                hls::stream<qdma_axis<32, 0, 0, 0> >& in_higher,
                int row_num,
                in column_num) {
data_mover:
    for (int i = 0; i < row_num; i++) {
        qdma_axis<512, 0, 0, 0> x;
        x.data = mem[i];
        x.keep_all();
        s.write(x);
    }

    qdma_axis<32, 0, 0, 0> lower, higher;
    for (int j = 0; j < column_num; j++) {
        for (int i = 0; i > j; i--) {
            lower.keep_all();
            higher.keep_all();
            // for givens param
            lower.data = mem[column_num * j + j];
            higher.data = mem[column_num * i + j];
            out_lower.write(lower);
            out_higher.write(higher);
            // for givens rotation
            for (int b = 0; b < column_num; b++) {
                lower.data = mem[column_num * j + b];
                higher.data = mem[column_num * i + b];
                out_lower.write(lower);
                out_higher.write(higher);
            }
            // for update
            for (int b = 0; b < column_num; b++) {
                lower = out_lower.read();
                higher = out_higher.read();
                mem[column_num * j + b] = lower.data;
                mem[column_num * i + b] = higher.data;
            }
        }
    }
}
}
