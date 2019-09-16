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

void mean_shift_accel(xf::Mat<XF_8UC4, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &inMat, uint16_t* tlx, uint16_t* tly,
		uint16_t* obj_height, uint16_t* obj_width, uint16_t* dx, uint16_t* dy, uint16_t* track,
		uint8_t frame_status, uint8_t no_objects, uint8_t no_of_iterations)
{	
#pragma HLS INTERFACE m_axi depth=10 port=tlx offset=direct bundle=in
#pragma HLS INTERFACE m_axi depth=10 port=tly offset=direct bundle=in
#pragma HLS INTERFACE m_axi depth=10 port=obj_height offset=direct bundle=in
#pragma HLS INTERFACE m_axi depth=10 port=obj_width offset=direct bundle=in
#pragma HLS INTERFACE m_axi depth=10 port=dx offset=direct bundle=out
#pragma HLS INTERFACE m_axi depth=10 port=dy offset=direct bundle=out
#pragma HLS INTERFACE m_axi depth=10 port=track offset=direct bundle=out

	xf::MeanShift<XF_MAX_OBJECTS,XF_MAX_ITERS,XF_MAX_OBJ_HEIGHT,XF_MAX_OBJ_WIDTH,XF_8UC4,XF_HEIGHT,XF_WIDTH,XF_NPPC1>
		(inMat,tlx,tly,obj_height,obj_width,dx,dy,track,frame_status,no_objects,no_of_iterations);
}

