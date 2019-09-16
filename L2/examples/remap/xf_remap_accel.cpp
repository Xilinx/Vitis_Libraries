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

#include "xf_remap_config.h"

extern "C" {

void remap_accel(
    ap_uint<PTR_IMG_WIDTH>* img_in, float* map_x, float* map_y, ap_uint<PTR_IMG_WIDTH>* img_out, int rows, int cols) {
    // clang-format off
    #pragma HLS INTERFACE m_axi      port=img_in        offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=map_x         offset=slave  bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=map_y         offset=slave  bundle=gmem2
    #pragma HLS INTERFACE m_axi      port=img_out       offset=slave  bundle=gmem3
    #pragma HLS INTERFACE s_axilite  port=rows 	bundle=control
    #pragma HLS INTERFACE s_axilite  port=cols 	bundle=control
    #pragma HLS INTERFACE s_axilite  port=return 	bundle=control
    // clang-format on

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> imgInput(rows, cols);
    xf::cv::Mat<TYPE_XY, HEIGHT, WIDTH, NPC> mapX(rows, cols);
    xf::cv::Mat<TYPE_XY, HEIGHT, WIDTH, NPC> mapY(rows, cols);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> imgOutput(rows, cols);

    const int HEIGHT_WIDTH_LOOPCOUNT = HEIGHT * WIDTH / XF_NPIXPERCYCLE(NPC);
    for (unsigned int i = 0; i < rows * cols; ++i) {
        // clang-format off
	#pragma HLS LOOP_TRIPCOUNT min=1 max=HEIGHT_WIDTH_LOOPCOUNT
        #pragma HLS PIPELINE II=1
        // clang-format on
        float map_x_val = map_x[i];
        float map_y_val = map_y[i];
        mapX.write_float(i, map_x_val);
        mapY.write_float(i, map_y_val);
    }

    // clang-format off
    #pragma HLS STREAM variable=imgInput.data depth=2
    #pragma HLS STREAM variable=mapX.data depth=2
    #pragma HLS STREAM variable=mapY.data depth=2
    #pragma HLS STREAM variable=imgOutput.data depth=2
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_IMG_WIDTH, TYPE, HEIGHT, WIDTH, NPC>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::remap<XF_WIN_ROWS, XF_INTERPOLATION_TYPE, TYPE, TYPE_XY, TYPE, HEIGHT, WIDTH, NPC, XF_USE_URAM>(
        imgInput, imgOutput, mapX, mapY);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_IMG_WIDTH, TYPE, HEIGHT, WIDTH, NPC>(imgOutput, img_out);

    return;
} // End of kernel

} // End of extern C
