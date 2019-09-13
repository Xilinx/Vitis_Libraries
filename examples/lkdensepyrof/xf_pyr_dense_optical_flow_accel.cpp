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
#include "xf_pyr_dense_optical_flow_config.h"

void pyr_dense_optical_flow_pyr_down_accel(xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1> mat_imagepyr1[NUM_LEVELS], xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1> mat_imagepyr2[NUM_LEVELS])
{	
	for(int pyr_comp=0;pyr_comp<NUM_LEVELS-1; pyr_comp++)
	{
	#pragma SDS async(1)
	#pragma SDS resource(1)
		xf::pyrDown<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1,XF_USE_URAM>(mat_imagepyr1[pyr_comp], mat_imagepyr1[pyr_comp+1]);
	#pragma SDS async(2)
	#pragma SDS resource(2)
		xf::pyrDown<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1,XF_USE_URAM>(mat_imagepyr2[pyr_comp], mat_imagepyr2[pyr_comp+1]);
	#pragma SDS wait(1)
	#pragma SDS wait(2)	
	}
}

void pyr_dense_optical_flow_accel(xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1> & _current_img, xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1> & _next_image, xf::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1> & _streamFlowin, xf::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1> & _streamFlowout, const int level, const unsigned char scale_up_flag, float scale_in, ap_uint<1> init_flag)
{	
	xf::densePyrOpticalFlow<NUM_LEVELS, NUM_LINES_FINDIT, WINSIZE_OFLOW, TYPE_FLOW_WIDTH, TYPE_FLOW_INT, XF_8UC1, HEIGHT, WIDTH, XF_NPPC1, XF_USE_URAM>(_current_img, _next_image, _streamFlowin, _streamFlowout, level, scale_up_flag, scale_in, init_flag);
}

