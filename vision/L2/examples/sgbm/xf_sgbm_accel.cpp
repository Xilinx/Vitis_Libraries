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

#include "xf_sgbm_config.h"

extern "C" {

void sgbm_accel(ap_uint<PTR_IN_WIDTH>* img_in_l,
                ap_uint<PTR_IN_WIDTH>* img_in_r,
                unsigned char penalty_small,
                unsigned char penalty_large,
                ap_uint<PTR_OUT_WIDTH>* img_out) {
    // clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in_l      offset=slave  bundle=gmem0
    #pragma HLS INTERFACE s_axilite  port=img_in_l 			          bundle=control
    #pragma HLS INTERFACE m_axi      port=img_in_r      offset=slave  bundle=gmem1
    #pragma HLS INTERFACE s_axilite  port=img_in_r 			          bundle=control
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem2
    #pragma HLS INTERFACE s_axilite  port=img_out 			          bundle=control
    #pragma HLS INTERFACE s_axilite  port=penalty_small  	          bundle=control
    #pragma HLS INTERFACE s_axilite  port=penalty_large  	          bundle=control
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInputL;
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInputR;
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput;

    const int COLS = WIDTH;
    const int NPPC = NPC1;
    // clang-format off
    #pragma HLS STREAM variable=imgInputL.data depth=COLS/NPPC
    #pragma HLS STREAM variable=imgInputR.data depth=COLS/NPPC
    #pragma HLS STREAM variable=imgOutput.data depth=COLS/NPPC
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_IN_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPC1>(img_in_l, imgInputL);
    xf::cv::Array2xfMat<PTR_IN_WIDTH, IN_TYPE, HEIGHT, WIDTH, NPC1>(img_in_r, imgInputR);

    // Run xfOpenCV kernel:
    xf::cv::SemiGlobalBM<XF_BORDER_CONSTANT, WINDOW_SIZE, TOTAL_DISPARITY, PARALLEL_UNITS, NUM_DIR, IN_TYPE, OUT_TYPE,
                         HEIGHT, WIDTH, NPC1>(imgInputL, imgInputR, imgOutput, penalty_small, penalty_large);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_OUT_WIDTH, OUT_TYPE, HEIGHT, WIDTH, NPC1>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C
