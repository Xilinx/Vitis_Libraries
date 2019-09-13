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
#ifndef _XF_PYR_DOWN_
#define _XF_PYR_DOWN_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "imgproc/xf_pyr_down_gaussian_blur.hpp"

namespace xf{

template <unsigned int ROWS, unsigned int COLS, unsigned int TYPE, unsigned int NPC, int PLANES, bool USE_URAM>
void xFpyrDownKernel(xf::Mat<TYPE, ROWS, COLS, NPC> & _src, xf::Mat<TYPE, ROWS, COLS, NPC> & _dst, unsigned short in_rows, unsigned short in_cols)
{
#pragma HLS DATAFLOW
	hls::stream< XF_TNAME(TYPE,NPC) > _filter_in;
	hls::stream< XF_TNAME(TYPE,NPC) > _filter_out;
	unsigned int read_pointer = 0;
	for(int i=0;i<in_rows;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0;j<in_cols;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
#pragma HLS PIPELINE II=1
			_filter_in.write(_src.read(read_pointer));
			read_pointer++;
		}
	}
	xFPyrDownGaussianBlur<ROWS,COLS,TYPE, NPC, XF_WORDWIDTH(TYPE,NPC), 0,5,25, PLANES>(_filter_in, _filter_out, 5, XF_BORDER_CONSTANT,in_rows,in_cols);

	unsigned int write_ptr = 0;
	for(int i=0;i<in_rows;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0;j<in_cols;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
#pragma HLS PIPELINE II=1
			XF_TNAME(TYPE,NPC) read_fil_out = _filter_out.read();
			if(i%2 == 0 && j%2 == 0)
			{
				_dst.write(write_ptr,read_fil_out);
				write_ptr++;
			}
		}
	}
	return;
}



#pragma SDS data mem_attribute("_src.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL, "_dst.data":SEQUENTIAL)
//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst.data":AXIDMA_SIMPLE)
#pragma SDS data copy("_src.data"[0:"_src.size"], "_dst.data"[0:"_dst.size"])
template<int TYPE, int ROWS, int COLS, int NPC, bool USE_URAM = false> 
void pyrDown (xf::Mat<TYPE, ROWS, COLS, NPC> & _src, xf::Mat<TYPE, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	unsigned short input_height = _src.rows;
	unsigned short input_width = _src.cols;
	xFpyrDownKernel<ROWS, COLS, TYPE, NPC, XF_CHANNELS(TYPE,NPC), USE_URAM>(_src, _dst, input_height, input_width);
	return;
}
}
#endif
