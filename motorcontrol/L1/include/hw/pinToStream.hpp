/*
 * Copyright 2022 Xilinx, Inc.
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
#include <hls_stream.h>
#include "ap_int.h"

void process(ap_uint<1>* A,
             ap_uint<1>* B,
             ap_uint<1>* I,
             hls::stream<ap_uint<1> >& a,
             hls::stream<ap_uint<1> >& b,
             hls::stream<ap_uint<1> >& i) {
    static int k = 0;
#ifndef __SYNTHESIS__
LOOP:
    while (k < TESTSIZE)
#else
LOOP:
    while (1)
#endif
    {
#pragma HLS PIPELINE II = 1
        ap_uint<1> ini_A = A[k];
        ap_uint<1> ini_B = B[k];
        ap_uint<1> ini_I = I[k];
        a.write(ini_A);
        b.write(ini_B);
        i.write(ini_I);
        k++;
    }
}
void pinToStream(ap_uint<1>* A,
                 ap_uint<1>* B,
                 ap_uint<1>* I,
                 hls::stream<ap_uint<1> >& a,
                 hls::stream<ap_uint<1> >& b,
                 hls::stream<ap_uint<1> >& i) {
#pragma HLS INTERFACE ap_none port = A
#pragma HLS INTERFACE ap_none port = B
#pragma HLS INTERFACE ap_none port = I
#pragma HLS INTERFACE mode = axis register_mode = both depth = 512 port = a register
#pragma HLS INTERFACE mode = axis register_mode = both depth = 512 port = b register
#pragma HLS INTERFACE mode = axis register_mode = both depth = 512 port = i register
    process(A, B, I, a, b, i);
}
