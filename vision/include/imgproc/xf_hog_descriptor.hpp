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

#ifndef _XF_HOG_DESCRIPTOR_WRAPPER_HPP_
#define _XF_HOG_DESCRIPTOR_WRAPPER_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "xf_hog_descriptor_kernel.hpp"

namespace xf {
//#pragma SDS data data_mover("_in_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_desc_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_in_mat.data":SEQUENTIAL)
#pragma SDS data access_pattern("_desc_mat.data":SEQUENTIAL)
#pragma SDS data copy("_in_mat.data"[0:"_in_mat.size"])
#pragma SDS data copy("_desc_mat.data"[0:"_desc_mat.size"])
template<int WIN_HEIGHT, int WIN_WIDTH, int WIN_STRIDE, int BLOCK_HEIGHT, int BLOCK_WIDTH, int CELL_HEIGHT, int CELL_WIDTH, 
int NOB, int DESC_SIZE, int IMG_COLOR, int OUTPUT_VARIANT, int SRC_T, int DST_T, int ROWS, int COLS, int NPC = XF_NPPC1,bool USE_URAM=false>
void HOGDescriptor(xf::Mat<SRC_T, ROWS, COLS, NPC> &_in_mat, xf::Mat<DST_T, 1, DESC_SIZE, NPC> &_desc_mat)
{
	hls::stream< XF_TNAME(SRC_T,NPC) > in_strm;
	hls::stream< XF_CTUNAME(SRC_T,NPC) > in[IMG_COLOR];
	hls::stream< XF_SNAME(XF_576UW) > _block_strm;
	hls::stream< XF_TNAME(DST_T,NPC) > desc_strm;

#pragma HLS DATAFLOW

	int IN_TC=(ROWS*COLS>>XF_BITSHIFT(NPC));
	for (int i = 0; i < _in_mat.size; i++)
	{
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=IN_TC
		in_strm.write(_in_mat.read(i));
	}

	// Reads the input data from Input stream and writes the data to the output stream
	xFHOGReadFromStream < ROWS,COLS,IMG_COLOR > (in_strm,in,_in_mat.rows,_in_mat.cols);

	// Process function: performs HoG over the input stream and writes the descriptor data to the output stream
	xFDHOG < WIN_HEIGHT,WIN_WIDTH,WIN_STRIDE,BLOCK_HEIGHT,BLOCK_WIDTH,CELL_HEIGHT,CELL_WIDTH,
	NOB,ROWS,COLS,XF_8UP,XF_16UP,XF_NPPC1,XF_8UW,XF_576UW,IMG_COLOR,USE_URAM > (in,_block_strm,_in_mat.rows,_in_mat.cols);

	if (OUTPUT_VARIANT == XF_HOG_RB) {
		// writes the Descriptor data Window wise
		xFWriteHOGDescRB < WIN_HEIGHT,WIN_WIDTH,WIN_STRIDE,CELL_HEIGHT,CELL_WIDTH,NOB,
		ROWS,COLS,XF_16UP,XF_16UP,XF_NPPC1,XF_576UW,XF_32UW,USE_URAM > (_block_strm,desc_strm,_in_mat.rows,_in_mat.cols);
	}
	else if (OUTPUT_VARIANT == XF_HOG_NRB) {
		// writes the block data and the descriptors are formed on the host
		xFWriteHOGDescNRB < BLOCK_HEIGHT,BLOCK_WIDTH,CELL_HEIGHT,CELL_WIDTH,NOB,XF_DHOG,
		ROWS,COLS,XF_16UP,XF_NPPC1,XF_576UW,XF_TNAME(DST_T,NPC)> (_block_strm,desc_strm,_in_mat.rows,_in_mat.cols);
	}

	int OUT_TC=(ROWS*COLS>>XF_BITSHIFT(NPC));
	for (int i = 0; i < _desc_mat.size; i++)
	{
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=IN_TC
		_desc_mat.write(i,desc_strm.read());
	}
}
}

#endif   // _XF_HOG_DESCRIPTOR_WRAPPER_HPP_
