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
#ifndef _XF_PYR_UP_
#define _XF_PYR_UP_

#include "ap_int.h"
#include "hls_stream.h"
#include "imgproc/xf_pyr_up_gaussian_blur.hpp"
#include "common/xf_common.h"

namespace xf{

template <unsigned int ROWS, unsigned int COLS, unsigned int NPC, unsigned int DEPTH,int PLANES>
void xFpyrUpKernel(xf::Mat<DEPTH, ROWS, COLS, NPC> & _src, xf::Mat<DEPTH, 2*ROWS, 2*COLS, NPC> & _dst, unsigned short in_rows, unsigned short in_cols)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream< XF_TNAME(DEPTH,NPC) > _filter_in;
	hls::stream< XF_TNAME(DEPTH,NPC) > _filter_out;
	
	unsigned short output_height= in_rows << 1;
	unsigned short output_width = in_cols << 1;
	int read_pointer=0, write_pointer=0;
	for(int i=0;i<output_height;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0;j<output_width;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN OFF
			XF_TNAME(DEPTH,NPC) read_input;
			if(i%2 == 0 && j%2 == 0)
			{
				read_input = _src.read(read_pointer);//*(in_image + read_pointer);
				read_pointer++;
			}
			else
				read_input = 0;
			_filter_in.write(read_input);
		}
	}
	xFPyrUpGaussianBlur<2*ROWS,2*COLS, DEPTH, NPC, 0, 0,5,25,PLANES>(_filter_in, _filter_out, 5, XF_BORDER_REPLICATE,output_height,output_width);
	
	
	for(int i=0;i<output_height;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0;j<output_width;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN OFF
				//*(out_image + write_pointer) = _filter_out.read();
				_dst.write(write_pointer,(_filter_out.read()));
				write_pointer++;
			
		}
	}
	
	return;
}


//#pragma SDS data mem_attribute("_src.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
//#pragma SDS data mem_attribute("_dst.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL, "_dst.data":SEQUENTIAL)
//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst.data":AXIDMA_SIMPLE)
#pragma SDS data copy("_src.data"[0:"_src.size"], "_dst.data"[0:"_dst.size"])
template<int TYPE, int ROWS, int COLS, int NPC=1>
void pyrUp (xf::Mat<TYPE, ROWS, COLS, NPC> & _src, xf::Mat<TYPE, 2*ROWS, 2*COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	unsigned short input_height = _src.rows;
	unsigned short input_width = _src.cols;
	
	xFpyrUpKernel<ROWS, COLS, NPC, TYPE,XF_CHANNELS(TYPE,NPC)>(_src, _dst, input_height, input_width);
	
	return;
}
}
#endif
