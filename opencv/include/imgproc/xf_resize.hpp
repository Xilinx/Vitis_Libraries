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

#ifndef _XF_RESIZE_
#define _XF_RESIZE_

#include "xf_resize_headers.h"

/**
 * Image resizing function.
 */
namespace xf {

#pragma SDS data mem_attribute("_src.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL, "_dst.data":SEQUENTIAL)
//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst.data":AXIDMA_SIMPLE)
#pragma SDS data copy("_src.data"[0:"_src.size"], "_dst.data"[0:"_dst.size"])
template<int INTERPOLATION_TYPE, int TYPE, int SRC_ROWS, int SRC_COLS, int DST_ROWS, int DST_COLS, int NPC, int MAX_DOWN_SCALE> 
void resize (xf::Mat<TYPE, SRC_ROWS, SRC_COLS, NPC> & _src, xf::Mat<TYPE, DST_ROWS, DST_COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF

	assert(  ((INTERPOLATION_TYPE == XF_INTERPOLATION_NN)
	        ||(INTERPOLATION_TYPE == XF_INTERPOLATION_BILINEAR)
			||(INTERPOLATION_TYPE == XF_INTERPOLATION_AREA)) && "Incorrect parameters interpolation type");
	
	if(INTERPOLATION_TYPE == XF_INTERPOLATION_AREA)
		assert( (NPC == XF_NPPC1)  && "Supported Operation Mode for Area Interpolation is XF_NPPC1. XF_NPPC2, XF_NPPC4 and XF_NPPC8 are not supported ");
	else
		assert( ((NPC == XF_NPPC8) || (NPC == XF_NPPC4) || (NPC == XF_NPPC2) || (NPC == XF_NPPC1) )  && "Supported Operation Modes XF_NPPC8, XF_NPPC4, XF_NPPC2 and XF_NPPC1");

	if(NPC == XF_NPPC2)
		assert((((_src.cols & 1) == 0) && ((_dst.cols & 1) == 0)) && "Input and ouput image widths should be multiples of 2 in NPPC2 mode");
	if(NPC == XF_NPPC4)
		assert((((_src.cols & 3) == 0) && ((_dst.cols & 3) == 0)) && "Input and ouput image widths should be multiples of 4 in NPPC4 mode");
	if(NPC == XF_NPPC8)
		assert((((_src.cols & 7) == 0) && ((_dst.cols & 7) == 0)) && "Input and ouput image widths should be multiples of 8 in NPPC8 mode");

	if(INTERPOLATION_TYPE == XF_INTERPOLATION_AREA)
	{
	
		assert(((_src.rows <= SRC_ROWS ) && (_src.cols <= SRC_COLS)) && "SRC_ROWS and SRC_COLS should be greater than input image");

		assert(((_dst.rows <= DST_ROWS ) && (_dst.cols <= DST_COLS)) && "DST_ROWS and DST_COLS should be greater than output image");
		unsigned short input_height = _src.rows;
		unsigned short input_width = _src.cols >> XF_BITSHIFT(NPC);
		unsigned short output_height = _dst.rows;
		unsigned short output_width = _dst.cols >> XF_BITSHIFT(NPC);
				
		if ((SRC_ROWS < DST_ROWS) || (SRC_COLS < DST_COLS)){
			xFResizeAreaUpScale<SRC_ROWS,SRC_COLS,XF_CHANNELS(TYPE,NPC),TYPE,NPC,XF_WORDWIDTH(TYPE,NPC),   \
			DST_ROWS,DST_COLS,(SRC_COLS>>XF_BITSHIFT(NPC)),(DST_COLS>>XF_BITSHIFT(NPC))> \
			(_src,_dst, input_height, input_width, output_height, output_width);
		}
		else if ((SRC_ROWS >= DST_ROWS) || (SRC_COLS >= DST_COLS)){
			xFResizeAreaDownScale<SRC_ROWS,SRC_COLS,XF_CHANNELS(TYPE,NPC),TYPE,NPC,XF_WORDWIDTH(TYPE,NPC), \
			DST_ROWS,DST_COLS,(SRC_COLS>>XF_BITSHIFT(NPC)),(DST_COLS>>XF_BITSHIFT(NPC))> \
			(_src,_dst, input_height, input_width, output_height, output_width);
		}

		return;
	}
	else
	{
		resizeNNBilinear<TYPE, SRC_ROWS, SRC_COLS, NPC, DST_ROWS, DST_COLS, INTERPOLATION_TYPE, MAX_DOWN_SCALE>(_src,_dst);
	}
}
}
#endif
