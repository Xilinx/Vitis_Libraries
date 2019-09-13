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

#include "xf_boundingbox_config.h"

extern "C" {
	void boundingbox_accel(ap_uint<INPUT_PTR_WIDTH> *in_img, int *roi, int color_info[MAX_BOXES][4], int height, int width,int num_box)
	{
		#pragma HLS INTERFACE m_axi     port=in_img  		offset=slave bundle=gmem1
		#pragma HLS INTERFACE m_axi     port=roi  	    	offset=slave bundle=gmem2
		#pragma HLS INTERFACE m_axi     port=color_info  	offset=slave bundle=gmem3

		#pragma HLS INTERFACE s_axilite port=in_img            	 bundle=control
		#pragma HLS INTERFACE s_axilite port=roi            	 bundle=control
		#pragma HLS INTERFACE s_axilite port=color_info          bundle=control
		#pragma HLS INTERFACE s_axilite port=num_box           	 bundle=control
		#pragma HLS INTERFACE s_axilite port=height              bundle=control
		#pragma HLS INTERFACE s_axilite port=width               bundle=control
		#pragma HLS INTERFACE s_axilite port=return              bundle=control

		  const int pROWS = HEIGHT;
		  const int pCOLS = WIDTH;
		  const int pNPC1 = NPIX;

		xf::Rect_ <int> _roi[MAX_BOXES];
		xf::Scalar <4, unsigned char > color[MAX_BOXES];
		for(int i=0,j=0;i<num_box;i++,j+=4)
		{
			_roi[i].x=roi[j];
			_roi[i].y=roi[j+1];
			_roi[i].height=roi[j+2];
			_roi[i].width=roi[j+3];
			
		}
		for(int i=0;i<(num_box);i++)
		{
			for(int j=0;j<XF_CHANNELS(TYPE,NPIX);j++)
			{
				color[i].val[j]  = color_info[i][j];//(i*XF_CHANNELS(TYPE,NPIX))+j];
				

			}
		}

	
		xf::Mat<TYPE, HEIGHT, WIDTH, NPIX> in_mat(height,width,in_img);
		xf::boundingbox<TYPE, HEIGHT, WIDTH,MAX_BOXES, NPIX>(in_mat,_roi,color,num_box);


	}

}
