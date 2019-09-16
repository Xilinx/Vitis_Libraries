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
#include "xf_pyr_down_config.h"
extern "C" {
void pyr_down_accel(ap_uint<INPUT_PTR_WIDTH> *img_inp, ap_uint<OUTPUT_PTR_WIDTH> *img_out, int in_rows, int in_cols, int out_rows, int out_cols)
{	

#pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem2
#pragma HLS INTERFACE s_axilite port=in_rows     bundle=control
#pragma HLS INTERFACE s_axilite port=in_cols     bundle=control
#pragma HLS INTERFACE s_axilite port=out_rows     bundle=control
#pragma HLS INTERFACE s_axilite port=out_cols     bundle=control
#pragma HLS INTERFACE s_axilite port=return   bundle=control



	xf::Mat<TYPE, HEIGHT, WIDTH, NPC_T> in_mat(in_rows,in_cols);
#pragma HLS stream variable=in_mat.data depth=2

	xf::Mat<TYPE, HEIGHT, WIDTH, NPC_T> out_mat(out_rows,out_cols);
#pragma HLS stream variable=out_mat.data depth=2

	
	#pragma HLS DATAFLOW
	xf::Array2xfMat<INPUT_PTR_WIDTH,TYPE,HEIGHT,WIDTH,NPC_T>(img_inp,in_mat);
	xf::pyrDown <TYPE, HEIGHT, WIDTH,  NPC_T,XF_USE_URAM> (in_mat, out_mat);
	xf::xfMat2Array<OUTPUT_PTR_WIDTH,TYPE,HEIGHT,WIDTH,NPC_T>(out_mat,img_out);
	
	
}
}