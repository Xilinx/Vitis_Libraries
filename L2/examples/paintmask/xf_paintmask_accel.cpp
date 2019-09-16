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

#include "xf_paintmask_config.h"

extern "C" {

void paintmask_accel(ap_uint<PTR_WIDTH>* img_in,
                     ap_uint<PTR_M_WIDTH>* mask_in,
                     unsigned char* color,
                     ap_uint<PTR_WIDTH>* img_out) {
    // clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE s_axilite  port=img_in 			          bundle=control
    #pragma HLS INTERFACE m_axi      port=mask_in       offset=slave  bundle=gmem1
    #pragma HLS INTERFACE s_axilite  port=mask_in 			          bundle=control
    #pragma HLS INTERFACE m_axi      port=color   		offset=slave  bundle=gmem2
    #pragma HLS INTERFACE s_axilite  port=color 			 	      bundle=control
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem3
    #pragma HLS INTERFACE s_axilite  port=img_out 			          bundle=control
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput;
    xf::cv::Mat<M_TYPE, HEIGHT, WIDTH, NPC1> maskInput;
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput;

    const int cols = WIDTH;
    const int nppc = NPC1;

    // clang-format off
    #pragma HLS STREAM variable=imgInput.data depth=cols/nppc
    #pragma HLS STREAM variable=maskInput.data depth=cols/nppc
    #pragma HLS STREAM variable=imgOutput.data depth=cols/nppc
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Copy the color data to local memory:
    unsigned char color_local[XF_CHANNELS(TYPE, NPC1)];
    for (unsigned int i = 0; i < XF_CHANNELS(TYPE, NPC1); ++i) {
        color_local[i] = color[i];
    }

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(img_in, imgInput);
    xf::cv::Array2xfMat<PTR_M_WIDTH, M_TYPE, HEIGHT, WIDTH, NPC1>(mask_in, maskInput);

    // Run xfOpenCV kernel:
    xf::cv::paintmask<TYPE, M_TYPE, HEIGHT, WIDTH, NPC1>(imgInput, maskInput, imgOutput, color_local);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C