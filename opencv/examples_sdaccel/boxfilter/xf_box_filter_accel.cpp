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

#include "xf_box_filter_config.h"

extern "C" {
void boxfilter_accel(ap_uint<INPUT_PTR_WIDTH> *img_inp, ap_uint<OUTPUT_PTR_WIDTH> *img_out,int rows, int cols)
{
#pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem2
#pragma HLS INTERFACE s_axilite port=img_inp  bundle=control
#pragma HLS INTERFACE s_axilite port=img_out  bundle=control
#pragma HLS INTERFACE s_axilite port=rows     bundle=control
#pragma HLS INTERFACE s_axilite port=cols     bundle=control
#pragma HLS INTERFACE s_axilite port=return   bundle=control

  const int pROWS = HEIGHT;
  const int pCOLS = WIDTH;
  const int pNPC1 = NPIX;
  
   xf::Mat<IN_T, HEIGHT, WIDTH, NPIX> in_mat;
#pragma HLS stream variable=in_mat.data depth=pCOLS/pNPC1
	in_mat.rows = rows;
	in_mat.cols = cols;
	
	 xf::Mat<IN_T, HEIGHT, WIDTH, NPIX> _dst;
#pragma HLS stream variable=_dst.data depth=pCOLS/pNPC1
	_dst.rows = rows;
	_dst.cols = cols;	

	
#pragma HLS DATAFLOW

	
	xf::Array2xfMat<INPUT_PTR_WIDTH,IN_T,HEIGHT,WIDTH,NPIX>(img_inp,in_mat);
	
	xf::boxFilter<XF_BORDER_CONSTANT,FILTER_WIDTH,IN_T,HEIGHT, WIDTH,NPIX,XF_USE_URAM>(in_mat,_dst);
	
	xf::xfMat2Array<OUTPUT_PTR_WIDTH,IN_T,HEIGHT,WIDTH,NPIX>(_dst,img_out);
	
}
}
