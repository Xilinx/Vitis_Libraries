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
#include "xf_pp_pipeline_config.h"

extern "C" {
void pp_pipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                       int rows_in,
                       int cols_in,
                       int rows_out,
                       int cols_out,
                       float params[3 * T_CHANNELS],
                       int th1,
                       int th2) {
#pragma HLS INTERFACE m_axi port = img_inp offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = img_out offset = slave bundle = gmem2
#pragma HLS INTERFACE m_axi port = params offset = slave bundle = gmem3

#pragma HLS INTERFACE s_axilite port = rows_in bundle = control
#pragma HLS INTERFACE s_axilite port = cols_in bundle = control
#pragma HLS INTERFACE s_axilite port = rows_out bundle = control
#pragma HLS INTERFACE s_axilite port = cols_out bundle = control
#pragma HLS INTERFACE s_axilite port = th1 bundle = control
#pragma HLS INTERFACE s_axilite port = th2 bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control

    xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput0(rows_in, cols_in);
#pragma HLS stream variable = imgInput0.data depth = 2

    xf::cv::Mat<TYPE, NEWHEIGHT, NEWWIDTH, NPC_T> out_mat(rows_out, cols_out);
#pragma HLS stream variable = out_mat.data depth = 2

    hls::stream<ap_uint<256> > resizeStrmout;
    int srcMat_cols_align_npc = ((out_mat.cols + (NPC_T - 1)) >> XF_BITSHIFT(NPC_T)) << XF_BITSHIFT(NPC_T);
#pragma HLS DATAFLOW

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_8UC3, HEIGHT, WIDTH, NPC1>(img_inp, imgInput0);
    xf::cv::resize<INTERPOLATION, TYPE, HEIGHT, WIDTH, NEWHEIGHT, NEWWIDTH, NPC_T, MAXDOWNSCALE>(imgInput0, out_mat);
    xf::cv::accel_utils obj;
    obj.xfMat2hlsStrm<INPUT_PTR_WIDTH, TYPE, NEWHEIGHT, NEWWIDTH, NPC_T, (NEWWIDTH * NEWHEIGHT / 8)>(
        out_mat, resizeStrmout, srcMat_cols_align_npc);
    xf::cv::preProcess<INPUT_PTR_WIDTH, OUTPUT_PTR_WIDTH, T_CHANNELS, CPW, HEIGHT, WIDTH, NPC_TEST, PACK_MODE, X_WIDTH,
                       ALPHA_WIDTH, BETA_WIDTH, GAMMA_WIDTH, OUT_WIDTH, X_IBITS, ALPHA_IBITS, BETA_IBITS, GAMMA_IBITS,
                       OUT_IBITS, SIGNED_IN, OPMODE>(resizeStrmout, img_out, params, rows_out, cols_out, th1, th2);
}
}