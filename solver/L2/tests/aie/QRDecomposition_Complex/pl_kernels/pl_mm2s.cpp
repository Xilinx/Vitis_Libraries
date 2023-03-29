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
#include "pragma_macro.hpp"

extern "C" {

void pl_mm2s(ap_uint<32>* mem, hls::stream<ap_axiu<32, 0, 0, 0> >& s, int size) {
    SAXI(s)
    AXIL(size)
    MAXI(mem, gmem0, 32, 32, 32, 32, 32, 4096)
    AXIL(mem)
    AXIL(return )
    for (int i = 0; i < size; i++) {
#pragma HLS pipeline II = 1
        ap_axiu<32, 0, 0, 0> x;
        x.data = mem[i];
        x.keep = -1;
        x.last = 0;
        s.write(x);
    }
}

/*
void pl_s2mm(ap_uint<32>* mem, hls::stream<ap_axiu<32, 0, 0, 0> >& s, int size) {
    SAXI(s)
    AXIL(size)
    MAXI(mem, gmem1, 32, 32, 32, 32, 32, 4096)
    AXIL(mem)
    AXIL(return )
    for (int i = 0; i < size; i++) {
#pragma HLS pipeline II = 1
        ap_axiu<32, 0, 0, 0> x = s.read();
        mem[i] = x.data;
    }
}
*/
}
