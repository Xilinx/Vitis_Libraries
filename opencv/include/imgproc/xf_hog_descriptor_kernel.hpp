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

#ifndef _XF_HOG_DESCRIPTOR_KERNEL_HPP_
#define _XF_HOG_DESCRIPTOR_KERNEL_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "core/xf_math.h"

#include "imgproc/xf_hog_descriptor_utility.hpp"
#include "imgproc/xf_hog_descriptor_gradients.hpp"
#include "imgproc/xf_hog_descriptor_pm.hpp"
#include "imgproc/xf_hog_descriptor_hist_norm.hpp"


/********************************************************************************************
 * 						xFDHOG function
 ********************************************************************************************
 *   This function calls the various pipelined functions for computing the HoG descriptors.
 *
 *   _in_stream: input image stream
 *   _block_stream: block stream (O) desc data written to this stream
 *
 ********************************************************************************************/
template<int WIN_HEIGHT, int WIN_WIDTH, int WIN_STRIDE, int CELL_HEIGHT,
int CELL_WIDTH, int NOB, int NOHCPB, int NOVCPB, int MAT_WW, int ROWS,
int COLS, int DEPTH, int DEPTH_BLOCK, int NPC, int WORDWIDTH, int WORDWIDTH_BLOCK,
int NOC,bool USE_URAM>
void xFDHOGKernel( hls::stream<XF_SNAME(WORDWIDTH)> _in_stream[NOC],
		hls::stream<XF_SNAME(WORDWIDTH_BLOCK)>& _block_stream,uint16_t _height,uint16_t _width)
{
	// streams for dataflow between various processes
	hls::stream<XF_SNAME(MAT_WW)>  grad_x_stream, grad_y_stream;
	hls::stream<XF_SNAME(XF_16UW)> phase_stream("phase_stream"), mag_stream("mag_stream");

#pragma HLS DATAFLOW

	//  gradient computation
	xFHOGgradients < ROWS,COLS,DEPTH,XF_9SP,XF_NPPC1,WORDWIDTH,MAT_WW,NOC,USE_URAM>
	(_in_stream,grad_x_stream,grad_y_stream,XF_BORDER_CONSTANT,_height,_width);

	// finding the magnitude and the phase for the gradient data
	xFHOGPhaseMagnitude < ROWS,COLS,XF_9SP,XF_16UP,XF_NPPC1,MAT_WW,XF_16UW >
	(grad_x_stream,grad_y_stream,phase_stream,mag_stream,_height,_width);

	// Descriptor function where the histogram is computed and the blocks are normalized
	xFDHOGDescriptor < WIN_HEIGHT,WIN_WIDTH,WIN_STRIDE,CELL_HEIGHT,CELL_WIDTH,NOB,NOHCPB,
	NOVCPB,ROWS,COLS,XF_16UP,DEPTH_BLOCK,XF_NPPC1, XF_16UW,WORDWIDTH_BLOCK,USE_URAM >
	(phase_stream,mag_stream,_block_stream,_height,_width);
}


/***********************************************************************
 * 						xFDHOG function
 ***********************************************************************
 *   This function acts as wrapper function for xFDHOGKernel
 *
 *   _in_stream: This stream contains the input image data (I)
 *   _block_stream: This stream contaisn the output descriptor data (O)
 *
 ***********************************************************************/
template<int WIN_HEIGHT, int WIN_WIDTH, int WIN_STRIDE, int BLOCK_HEIGHT, int BLOCK_WIDTH,
int CELL_HEIGHT, int CELL_WIDTH, int NOB, int ROWS, int COLS, int DEPTH, int DEPTH_BLOCK,
int NPC, int WORDWIDTH, int WORDWIDTH_BLOCK, int NOC,bool USE_URAM>
void xFDHOG (
		hls::stream<XF_SNAME(WORDWIDTH)> _in_stream[NOC],
		hls::stream<XF_SNAME(WORDWIDTH_BLOCK)>& _block_stream,
		uint16_t _height,uint16_t _width)
{
	//#pragma HLS license key=IPAUVIZ_HOG
	// Updating the _width based on NPC
	_width = _width >> XF_BITSHIFT(NPC);

	assert(((_height <= ROWS ) && (_width <= COLS)) && "ROWS and COLS should be greater than input image");
	assert((NPC == XF_NPPC1) && "The NPC value must be XF_NPPC1");

	if(NPC == XF_NPPC1)
	{
		xFDHOGKernel < WIN_HEIGHT,WIN_WIDTH,WIN_STRIDE,CELL_HEIGHT,CELL_WIDTH,
		NOB,(BLOCK_WIDTH/CELL_WIDTH),(BLOCK_HEIGHT/CELL_HEIGHT),XF_9UW,ROWS,COLS,
		DEPTH,DEPTH_BLOCK,NPC,WORDWIDTH,WORDWIDTH_BLOCK,NOC,USE_URAM >
		(_in_stream,_block_stream,_height,_width);
	}
}
#endif   // _XF_HOG_DESCRIPTOR_KERNEL_HPP_
