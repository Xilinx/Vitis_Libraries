/***************************************************************************
Copyright (c) 2016, Xilinx, Inc.
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
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "xf_magnitude_config.h"

extern "C" {
void magnitude_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp1,
                     ap_uint<INPUT_PTR_WIDTH>* img_inp2,
                     ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                     int rows,
                     int cols) {
    // clang-format off
    #pragma HLS INTERFACE m_axi     port=img_inp1  offset=slave bundle=gmem1
    #pragma HLS INTERFACE m_axi     port=img_inp2  offset=slave bundle=gmem2
    #pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem3
    // clang-format on

    // clang-format off

    #pragma HLS INTERFACE s_axilite port=rows     bundle=control
    #pragma HLS INTERFACE s_axilite port=cols     bundle=control
    #pragma HLS INTERFACE s_axilite port=return   bundle=control
    // clang-format on

    xf::cv::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> _src1(rows, cols);
    // clang-format off
    #pragma HLS stream variable=_src1.data depth=2
    // clang-format on

    xf::cv::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> _src2(rows, cols);
    // clang-format off
    #pragma HLS stream variable=_src2.data depth=2
    // clang-format on

    xf::cv::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> _dst(rows, cols);
    // clang-format off
    #pragma HLS stream variable=_dst.data depth=2
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_16SC1, HEIGHT, WIDTH, NPC1>(img_inp1, _src1);
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_16SC1, HEIGHT, WIDTH, NPC1>(img_inp2, _src2);

    xf::cv::magnitude<NORM_TYPE, XF_16SC1, XF_16SC1, HEIGHT, WIDTH, NPC1>(_src1, _src2, _dst);

    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16SC1, HEIGHT, WIDTH, NPC1>(_dst, img_out);
}
}