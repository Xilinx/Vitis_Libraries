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

#include "xf_inrange_config.h"

extern "C" {

void inrange_accel(ap_uint<PTR_IN_WIDTH>* img_in,
                   unsigned char lower_thresh,
                   unsigned char upper_thresh,
                   ap_uint<PTR_OUT_WIDTH>* img_out) {
    // clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE s_axilite  port=img_in 			          bundle=control
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem1
    #pragma HLS INTERFACE s_axilite  port=img_out 			          bundle=control
    #pragma HLS INTERFACE s_axilite  port=lower_thresh 			      bundle=control
    #pragma HLS INTERFACE s_axilite  port=upper_thresh 			      bundle=control
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput;
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput;

    const int cols = WIDTH;
    const int nppc = NPC1;

    // clang-format off
    #pragma HLS STREAM variable=imgInput.data depth=cols/nppc
    #pragma HLS STREAM variable=imgOutput.data depth=cols/nppc
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Copy threshold to local memory:
    unsigned char local_low_thresh[XF_CHANNELS(IN_TYPE, NPC1)];
    unsigned char local_high_thresh[XF_CHANNELS(IN_TYPE, NPC1)];

    for (unsigned int i = 0; i < XF_CHANNELS(IN_TYPE, NPC1); ++i) {
        local_low_thresh[i] = lower_thresh;
        local_high_thresh[i] = upper_thresh;
    }

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_IN_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPC1>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::inRange<IN_TYPE, OUT_TYPE, HEIGHT, WIDTH, NPC1>(imgInput, local_low_thresh, local_high_thresh, imgOutput);

    // Convert imgOutput xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_OUT_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPC1>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C