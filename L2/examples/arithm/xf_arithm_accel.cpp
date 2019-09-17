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

#include "xf_arithm_config.h"

extern "C" {

#if ARRAY
#if defined(FUNCT_BITWISENOT) || defined(FUNCT_ZERO)
void arithm_accel(
    ap_uint<PTR_WIDTH>* img_in1, ap_uint<PTR_WIDTH>* img_in2, ap_uint<PTR_WIDTH>* img_out, int height, int width) {
// clang-format off
#pragma HLS INTERFACE m_axi      port=img_in1       offset=slave  bundle=gmem0
#pragma HLS INTERFACE m_axi      port=img_in2       offset=slave  bundle=gmem1
#pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem2
#pragma HLS INTERFACE s_axilite  port=height 			          bundle=control
#pragma HLS INTERFACE s_axilite  port=width 			          bundle=control
// clang-format on
#ifdef FUNCT_MULTIPLY
// clang-format off
#pragma HLS INTERFACE s_axilite  port=scale 			          bundle=control
// clang-format on
#endif
    // clang-format off
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput1(height, width);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(height, width);

    // clang-format off
    #pragma HLS STREAM variable=imgInput1.data depth=2
    #pragma HLS STREAM variable=imgOutput.data depth=2
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(img_in1, imgInput1);

    // Run xfOpenCV kernel:
    xf::cv::FUNCT_NAME<TYPE, HEIGHT, WIDTH, NPC1>(imgInput1, imgOutput);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(imgOutput, img_out);

    return;
} // End of kernel
#else
void arithm_accel(ap_uint<PTR_WIDTH>* img_in1,
                  ap_uint<PTR_WIDTH>* img_in2,
#ifdef FUNCT_MULTIPLY
                  float scale,
#endif
                  ap_uint<PTR_WIDTH>* img_out,
                  int height,
                  int width) {
// clang-format off
#pragma HLS INTERFACE m_axi      port=img_in1       offset=slave  bundle=gmem0

#pragma HLS INTERFACE m_axi      port=img_in2       offset=slave  bundle=gmem1

#pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem2
#pragma HLS INTERFACE s_axilite  port=height 			          bundle=control
#pragma HLS INTERFACE s_axilite  port=width 			          bundle=control
// clang-format on
#ifdef FUNCT_MULTIPLY
// clang-format off
#pragma HLS INTERFACE s_axilite  port=scale 			          bundle=control
// clang-format on
#endif
    // clang-format off
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput1(height, width);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput2(height, width);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(height, width);

    // clang-format off
    #pragma HLS STREAM variable=imgInput1.data depth=2
    #pragma HLS STREAM variable=imgInput2.data depth=2
    #pragma HLS STREAM variable=imgOutput.data depth=2
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(img_in1, imgInput1);
    xf::cv::Array2xfMat<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(img_in2, imgInput2);

// Run xfOpenCV kernel:
#ifdef EXTRA_PARM
    xf::cv::FUNCT_NAME<EXTRA_PARM, TYPE, HEIGHT, WIDTH, NPC1>(imgInput1, imgInput2, imgOutput
#ifdef FUNCT_MULTIPLY
                                                              ,
                                                              scale
#endif
    );
#else
    xf::cv::FUNCT_NAME<TYPE, HEIGHT, WIDTH, NPC1>(imgInput1, imgInput2, imgOutput);
#endif

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(imgOutput, img_out);

    return;
} // End of kernel
#endif
#endif

#if SCALAR
void arithm_accel(
    ap_uint<PTR_WIDTH>* img_in1, unsigned char* scl_in, ap_uint<PTR_WIDTH>* img_out, int height, int width) {
    // clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in1       offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=scl_in        offset=slave  bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=img_out      	 offset=slave  bundle=gmem2
   
    #pragma HLS INTERFACE s_axilite  port=height 			          bundle=control
    #pragma HLS INTERFACE s_axilite  port=width 			          bundle=control
    #pragma HLS INTERFACE s_axilite  port=return 			          bundle=control
    // clang-format on

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput1(height, width);
    unsigned char scl[XF_CHANNELS(TYPE, NPC1)];
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(height, width);

    // clang-format off
    #pragma HLS STREAM variable=imgInput1.data depth=2
    #pragma HLS STREAM variable=imgOutput.data depth=2
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(img_in1, imgInput1);
    for (unsigned int i = 0; i < XF_CHANNELS(TYPE, NPC1); ++i) {
        scl[i] = scl_in[i];
    }

    // Run xfOpenCV kernel:
    xf::cv::FUNCT_NAME<
#ifdef EXTRA_PARM
        EXTRA_PARM,
#endif
        TYPE, HEIGHT, WIDTH, NPC1>(imgInput1, scl, imgOutput);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_WIDTH, TYPE, HEIGHT, WIDTH, NPC1>(imgOutput, img_out);

    return;
} // End of kernel
#endif

} // End of extern C
