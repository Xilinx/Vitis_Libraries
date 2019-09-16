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

void crop_accel(xf::Mat<TYPE, HEIGHT, WIDTH, NPC> &_src,xf::Mat<TYPE, HEIGHT, WIDTH, NPC> _dst[NUM_ROI],xf::Rect_<unsigned int> roi[NUM_ROI])
{


#if MEMORYMAPPED_ARCH

	for(int i=0; i<NUM_ROI; i++)
	{
		xf::crop<TYPE, HEIGHT, WIDTH,MEMORYMAPPED_ARCH, NPC>(_src, _dst[i],roi[i]);
	}
#else
		xf::Mat<TYPE, HEIGHT, WIDTH, NPC> _src1(_src.rows,_src.cols);
		xf::Mat<TYPE, HEIGHT, WIDTH, NPC> _src2(_src.rows,_src.cols);
		xf::Mat<TYPE, HEIGHT, WIDTH, NPC> _src3(_src.rows,_src.cols);
		xf::Mat<TYPE, HEIGHT, WIDTH, NPC> _src4(_src.rows,_src.cols);
	#pragma HLS dataflow
		xf::duplicateMat<TYPE, HEIGHT, WIDTH, NPC>(_src,_src1,_src2);
		xf::duplicateMat<TYPE, HEIGHT, WIDTH, NPC>(_src1,_src3,_src4);
		xf::crop<TYPE, HEIGHT, WIDTH,MEMORYMAPPED_ARCH, NPC>(_src1, _dst[0],roi[0]);
		xf::crop<TYPE, HEIGHT, WIDTH,MEMORYMAPPED_ARCH, NPC>(_src2, _dst[1],roi[1]);
		xf::crop<TYPE, HEIGHT, WIDTH,MEMORYMAPPED_ARCH, NPC>(_src4, _dst[2],roi[2]);


#endif



}
