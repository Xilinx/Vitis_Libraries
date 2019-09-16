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
#include "xf_mean_shift_config.h"

extern "C" {
void mean_shift_accel(ap_uint<INPUT_PTR_WIDTH> *img_inp, uint16_t* tlx, uint16_t* tly,
		uint16_t* obj_height, uint16_t* obj_width, uint16_t* dx, uint16_t* dy, uint16_t* track,
		uint8_t frame_status, uint8_t no_objects, uint8_t no_of_iterations, int rows, int cols)
{
#pragma HLS INTERFACE m_axi     port=img_inp  depth=2073600 offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=tlx  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=tly  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=obj_height  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=obj_width  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi     port=dx  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=dy  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=track  offset=slave bundle=gmem5
#pragma HLS INTERFACE s_axilite port=frame_status     bundle=control
#pragma HLS INTERFACE s_axilite port=no_objects     bundle=control
#pragma HLS INTERFACE s_axilite port=no_of_iterations     bundle=control
#pragma HLS INTERFACE s_axilite port=rows     bundle=control
#pragma HLS INTERFACE s_axilite port=cols     bundle=control
#pragma HLS INTERFACE s_axilite port=return   bundle=control
	
	xf::Mat<XF_8UC4, XF_HEIGHT, XF_WIDTH, XF_NPPC1> inMat(rows,cols,img_inp);

	xf::MeanShift<XF_MAX_OBJECTS,XF_MAX_ITERS,XF_MAX_OBJ_HEIGHT,XF_MAX_OBJ_WIDTH,XF_8UC4,XF_HEIGHT,XF_WIDTH,XF_NPPC1>
		(inMat,tlx,tly,obj_height,obj_width,dx,dy,track,frame_status,no_objects,no_of_iterations);
}
}
