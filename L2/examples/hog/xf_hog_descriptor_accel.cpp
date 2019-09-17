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

#include "xf_hog_descriptor_config.h"

extern "C" {

void hog_descriptor_accel(
    ap_uint<PTR_IN_WIDTH>* img_in, ap_uint<PTR_OUT_WIDTH>* desc_out, int rows, int cols, int _desc_size) {
    //clang-format off
#pragma HLS INTERFACE m_axi port = img_in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = desc_out offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = rows bundle = control
#pragma HLS INTERFACE s_axilite port = cols bundle = control
#pragma HLS INTERFACE s_axilite port = _desc_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    //clang-format on

    //clang-format off
#pragma HLS DATAFLOW
    //clang-format on

    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, NPC> imgInput(rows, cols);
    xf::cv::Mat<OUT_TYPE, 1, XF_DESC_SIZE, NPC> descOutput(1, _desc_size);
    //clang-format off
#pragma HLS STREAM variable = imgInput.data depth = 2
#pragma HLS STREAM variable = descOutput.data depth = 2
    //clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<PTR_IN_WIDTH, IN_TYPE, XF_HEIGHT, XF_WIDTH, NPC>(img_in, imgInput);

    // Run xfOpenCV kernel:
    xf::cv::HOGDescriptor<XF_WIN_HEIGHT, XF_WIN_WIDTH, XF_WIN_STRIDE, XF_BLOCK_HEIGHT, XF_BLOCK_WIDTH, XF_CELL_HEIGHT,
                          XF_CELL_WIDTH, XF_NO_OF_BINS, XF_DESC_SIZE, XF_INPUT_COLOR, XF_OUTPUT_MODE, IN_TYPE, OUT_TYPE,
                          XF_HEIGHT, XF_WIDTH, NPC, XF_USE_URAM>(imgInput, descOutput);

    // Convert _dst xf::cv::Mat object to output array:
    xf::cv::xfMat2Array<PTR_OUT_WIDTH, OUT_TYPE, 1, XF_DESC_SIZE, NPC>(descOutput, desc_out);

    return;
} // End of kernel

} // End of extern C
