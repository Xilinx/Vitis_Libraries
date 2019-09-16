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

#include "xf_crop_config.h"




extern "C" {
	void crop_accel(ap_uint<INPUT_PTR_WIDTH> *img_in, ap_uint<OUTPUT_PTR_WIDTH> *_dst,ap_uint<OUTPUT_PTR_WIDTH> *_dst1,ap_uint<OUTPUT_PTR_WIDTH> *_dst2,int *roi, int height, int width)
//	void crop_accel(ap_uint<INPUT_PTR_WIDTH> *img_in, ap_uint<OUTPUT_PTR_WIDTH> *_dst,int *roi, int height, int width)
	{
	#pragma HLS INTERFACE m_axi     port=img_in  offset=slave bundle=gmem1
	#pragma HLS INTERFACE m_axi     port=_dst  offset=slave bundle=gmem2
	#pragma HLS INTERFACE m_axi     port=_dst1  offset=slave bundle=gmem3
	#pragma HLS INTERFACE m_axi     port=_dst2  offset=slave bundle=gmem4
	#pragma HLS INTERFACE m_axi     port=roi   offset=slave bundle=gmem5
	#pragma HLS INTERFACE s_axilite port=img_in  bundle=control
	#pragma HLS INTERFACE s_axilite port=_dst  bundle=control
	#pragma HLS INTERFACE s_axilite port=_dst1  bundle=control
	#pragma HLS INTERFACE s_axilite port=_dst2  bundle=control
	#pragma HLS INTERFACE s_axilite port=roi  bundle=control
	#pragma HLS INTERFACE s_axilite port=height     bundle=control
	#pragma HLS INTERFACE s_axilite port=width     bundle=control
	#pragma HLS INTERFACE s_axilite port=return   bundle=control

	  const int pROWS = HEIGHT;
	  const int pCOLS = WIDTH;
	  const int pNPC1 = NPC;
		printf("started loading rect execution\n");
	  	xf::Rect_ <unsigned int> _roi[NUM_ROI];
			for(int i=0,j=0;j<(NUM_ROI*4);i++,j+=4)
			{
	
				_roi[i].x=roi[j];
				_roi[i].y=roi[j+1];
				_roi[i].height=roi[j+2];
				_roi[i].width=roi[j+3];
			}
	  
		xf::Mat<TYPE, HEIGHT, WIDTH, NPC> in_mat(height,width,img_in);
		xf::Mat<TYPE, HEIGHT, WIDTH, NPC> out_mat(_roi[0].height,_roi[0].width,_dst);
		xf::Mat<TYPE, HEIGHT, WIDTH, NPC> out_mat1(_roi[1].height,_roi[1].width,_dst1);
		xf::Mat<TYPE, HEIGHT, WIDTH, NPC> out_mat2(_roi[2].height,_roi[2].width,_dst2);

		xf::crop<TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPC>(in_mat, out_mat,_roi[0]);
		xf::crop<TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPC>(in_mat, out_mat1,_roi[1]);
		xf::crop<TYPE, HEIGHT, WIDTH, MEMORYMAPPED_ARCH, NPC>(in_mat, out_mat2,_roi[2]);
	

	}
}

