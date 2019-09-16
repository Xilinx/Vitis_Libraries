/***************************************************************************
Copyright (c) 2019, Xilinx, Inc.
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

#include "xf_dense_npyr_optical_flow_config.h"

extern "C" {
void dense_non_pyr_of_accel(ap_uint<INPUT_PTR_WIDTH> *img_curr, ap_uint<INPUT_PTR_WIDTH> *img_prev, ap_uint<OUTPUT_PTR_WIDTH> *img_outx, ap_uint<OUTPUT_PTR_WIDTH> *img_outy, int rows, int cols)
{
#pragma HLS INTERFACE m_axi     port=img_curr  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_prev  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=img_outx  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=img_outy  offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=cols  bundle=control
#pragma HLS INTERFACE s_axilite port=rows  bundle=control
#pragma HLS INTERFACE s_axilite port=return   bundle=control



	xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC> in_curr_mat(rows,cols);
#pragma HLS stream variable=in_curr_mat.data depth=2


xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC> in_prev_mat(rows,cols);
#pragma HLS stream variable=in_prev_mat.data depth=2


	xf::Mat<XF_32FC1, MAX_HEIGHT, MAX_WIDTH, NPPC> outx_mat(rows,cols);
#pragma HLS stream variable=outx_mat.data depth=2


	xf::Mat<XF_32FC1, MAX_HEIGHT, MAX_WIDTH, NPPC> outy_mat(rows,cols);
#pragma HLS stream variable=outy_mat.data depth=2



#pragma HLS DATAFLOW

	xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(img_curr,in_curr_mat);
	xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(img_prev,in_prev_mat);
	

	xf::DenseNonPyrLKOpticalFlow<KMED, XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(in_curr_mat, in_prev_mat , outx_mat, outy_mat);


	xf::xfMat2Array<OUTPUT_PTR_WIDTH,XF_32FC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(outx_mat,img_outx);
	xf::xfMat2Array<OUTPUT_PTR_WIDTH,XF_32FC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(outy_mat,img_outy);


}
}
