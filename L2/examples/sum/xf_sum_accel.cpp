/***************************************************************************
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CXFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "xf_sum_config.h"

extern "C" {

void sum_accel(ap_uint<PTR_WIDTH>* img_in, double* sum_out) {
    // clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE s_axilite  port=img_in 			          bundle=control
    #pragma HLS INTERFACE m_axi      port=sum_out       offset=slave  bundle=gmem1
    #pragma HLS INTERFACE s_axilite  port=sum_out 			          bundle=control
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput;
    double sum_local[XF_CHANNELS(TYPE, NPC1)];

    const int COLS = WIDTH;
    const int NPPC = NPC1;
    // clang-format off
    #pragma HLS STREAM variable=imgInput.data depth=COLS/NPPC
    #pragma HLS STREAM variable=imgOutput.data depth=COLS/NPPC
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::sum<TYPE, HEIGHT, WIDTH, NPC1>(imgInput, sum_local);

    // Copy the result to output port:
    for (unsigned int i = 0; i < XF_CHANNELS(TYPE, NPC1); ++i) {
        sum_out[i] = sum_local[i];
    }

    return;
} // End of kernel

} // End of extern C