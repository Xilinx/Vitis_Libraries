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
#include "xf_corner_tracker_config.h"
extern "C"
{
void pyr_down_accel(ap_uint<INPUT_PTR_WIDTH> *inImgPyr1,
				   ap_uint<OUTPUT_PTR_WIDTH> *outImgPyr1,
				   ap_uint<INPUT_PTR_WIDTH> *inImgPyr2,
				   ap_uint<OUTPUT_PTR_WIDTH> *outImgPyr2,
				   int pyr_h, int pyr_w,
				   int pyr_out_h, int pyr_out_w)
				   {
#pragma HLS INTERFACE m_axi     port=inImgPyr1  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=outImgPyr1  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=inImgPyr2  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=outImgPyr2  offset=slave bundle=gmem4	

#pragma HLS INTERFACE s_axilite port=inImgPyr1  bundle=control
#pragma HLS INTERFACE s_axilite port=outImgPyr1  bundle=control
#pragma HLS INTERFACE s_axilite port=inImgPyr2     bundle=control
#pragma HLS INTERFACE s_axilite port=outImgPyr2     bundle=control

#pragma HLS INTERFACE s_axilite port=pyr_h     bundle=control
#pragma HLS INTERFACE s_axilite port=pyr_w     bundle=control
#pragma HLS INTERFACE s_axilite port=pyr_out_h     bundle=control
#pragma HLS INTERFACE s_axilite port=pyr_out_w     bundle=control

#pragma HLS INTERFACE s_axilite port=return   bundle=control	   

  const int pROWS = HEIGHT;
  const int pCOLS = WIDTH;

xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> pyr1_in_mat;
#pragma HLS stream variable=pyr1_in_mat.data depth=pCOLS/XF_NPPC1
	pyr1_in_mat.rows = pyr_h;
	pyr1_in_mat.cols = pyr_w;
	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> pyr1_out_mat;
#pragma HLS stream variable=pyr1_out_mat.data depth=pCOLS/XF_NPPC1
	pyr1_out_mat.rows = pyr_out_h;
	pyr1_out_mat.cols = pyr_out_w;
	
	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> pyr2_in_mat;
#pragma HLS stream variable=pyr2_in_mat.data depth=pCOLS/XF_NPPC1
	pyr2_in_mat.rows = pyr_h;
	pyr2_in_mat.cols = pyr_w;
	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> pyr2_out_mat;
#pragma HLS stream variable=pyr2_out_mat.data depth=pCOLS/XF_NPPC1
	pyr2_out_mat.rows = pyr_out_h;
	pyr2_out_mat.cols = pyr_out_w;



//creating image pyramid
		#pragma HLS DATAFLOW
		xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(inImgPyr1,pyr1_in_mat);
		xf::pyrDown<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(pyr1_in_mat,pyr1_out_mat);
		xf::xfMat2Array<INPUT_PTR_WIDTH,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(pyr1_out_mat,outImgPyr1);
		
		xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(inImgPyr2,pyr2_in_mat);
		xf::pyrDown<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(pyr2_in_mat,pyr2_out_mat);
		xf::xfMat2Array<INPUT_PTR_WIDTH,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(pyr2_out_mat,outImgPyr2);
		








return;
					   
   }
				   
}