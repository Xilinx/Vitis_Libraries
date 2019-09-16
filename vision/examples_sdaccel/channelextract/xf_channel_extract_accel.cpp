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

#include "xf_channel_extract_config.h"

/*void channel_extract_accel(xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput, xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput, unsigned short channel){

		xf::extractChannel<XF_8UC4, XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(imgInput, imgOutput, channel);
}*/
extern "C" {
	void channel_extract_accel(ap_uint<INPUT_PTR_WIDTH> *img_rgba, ap_uint<OUTPUT_PTR_WIDTH> *img_gray,uint16_t channel,int rows, int cols)
	{
		#pragma HLS INTERFACE m_axi     port=img_rgba  	offset=slave bundle=gmem1
		#pragma HLS INTERFACE m_axi     port=img_gray  	offset=slave bundle=gmem2
		#pragma HLS INTERFACE s_axilite port=img_rgba           bundle=control
		#pragma HLS INTERFACE s_axilite port=img_gray             bundle=control
		#pragma HLS INTERFACE s_axilite port=rows              	 bundle=control
		#pragma HLS INTERFACE s_axilite port=cols              	 bundle=control
		#pragma HLS INTERFACE s_axilite port=channel              	 bundle=control
		#pragma HLS INTERFACE s_axilite port=return              bundle=control
		
		  const int pROWS = HEIGHT;
		  const int pCOLS = WIDTH;
		  const int pNPC1 = XF_NPPC1;

		 xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1>   imgInput0;
	#pragma HLS stream variable=imgInput0.data depth=pCOLS/pNPC1
		imgInput0.rows=rows; imgInput0.cols=cols;
		xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>   imgOutput0;
	#pragma HLS stream variable=imgOutput0.data depth=pCOLS/pNPC1
		imgOutput0.rows=rows; imgOutput0.cols=cols;

		#pragma HLS DATAFLOW
		xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC4,HEIGHT, WIDTH, XF_NPPC1>  (img_rgba, imgInput0);
		xf::extractChannel<XF_8UC4, XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(imgInput0, imgOutput0, channel);
		xf::xfMat2Array<OUTPUT_PTR_WIDTH,XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(imgOutput0,img_gray);


	}
}
