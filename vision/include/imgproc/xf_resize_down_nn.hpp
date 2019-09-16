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

#ifndef _XF_RESIZE_DOWN_NN_
#define _XF_RESIZE_DOWN_NN_
#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

/*
 * Processes the 8 pixel block
 * outputs 8 pixels packed into 64-bit
 */
template<int DEPTH,int NPC,int WORDWIDTH>
static XF_TNAME(DEPTH,NPC) ProcessBlockNNDown(ap_uint<13> *Offset,XF_TNAME(DEPTH,NPC) *Data,ap_uint<13> blockstart,ap_uint<13> offset)
{
#pragma HLS INLINE OFF

	XF_PTUNAME(DEPTH) line0[((1+NPC)<<XF_BITSHIFT(NPC))];		// holds the unpacked pixel data
#pragma HLS ARRAY_PARTITION variable=line0 complete dim=1

	XF_PTUNAME(DEPTH) Pixel[(1<<XF_BITSHIFT(NPC))];
#pragma HLS ARRAY_PARTITION variable=Pixel complete

	uchar_t i,input_read;
	uint16_t block_start_ind;

	block_start_ind = (blockstart>>XF_BITSHIFT(NPC))<<XF_BITSHIFT(NPC);

	// unpack Data from Data to line0
	for(i=0;i<(1+NPC);i++){
#pragma HLS UNROLL
		xfExtractPixels<NPC,WORDWIDTH,DEPTH>(line0,Data[i],i<<XF_BITSHIFT(NPC));
	}

	XF_TNAME(DEPTH,NPC) val =0;
	int shift = 0;
	process_block_loop:for(i=0;i<(1<<XF_BITSHIFT(NPC));i++)
	{
#pragma HLS UNROLL
		//input_read = (NPC == XF_NPPC1) ?0:(Offset[offset+i] - block_start_ind);
		if(NPC == XF_NPPC1)
		{
			input_read = 0;
		}
		else
		{
			input_read = Offset[offset+i] - block_start_ind;
		}
		Pixel[i] = line0[input_read];
		shift = i<<XF_BITSHIFT(NPC);
		val.range(shift+7,shift) = Pixel[i];
	}
	return val;
}
static uint64_t xFUDivNNDown (uint64_t in_n, unsigned short in_d)
{
	#pragma HLS INLINE OFF
	uint32_t out_res = in_n/in_d;
	return out_res;
}
/*
 * Stream implementation of resizing the image using nearest neighbourhood interpolation technique.
 */
template<int SRC_ROWS,int SRC_COLS,int DEPTH,int NPC,int WORDWIDTH,int DST_ROWS,int DST_COLS,int SRC_TC,int DST_TC>
void xFResizeNNDownScale(hls::stream <XF_TNAME(DEPTH,NPC)> &stream_in, hls::stream <XF_TNAME(DEPTH,NPC)> &resize_out, unsigned short height, unsigned short width, unsigned short out_height, unsigned short out_width)
{
#pragma HLS INLINE
	XF_TNAME(DEPTH,NPC) lbuf_in[2][((SRC_COLS>>XF_BITSHIFT(NPC)))];  	// input buffer
#pragma HLS ARRAY_PARTITION variable=lbuf_in complete dim=1
#pragma HLS DEPENDENCE variable=lbuf_in inter false
#pragma HLS DEPENDENCE variable=lbuf_in intra false

	ap_uint<13> Hoffset[DST_COLS],Voffset[DST_ROWS];		// offset buffers which indicate from where the data is to be read

	if (NPC!=XF_NPPC1)
	{
// #pragma HLS ARRAY_PARTITION variable=Hoffset complete
	}
	ap_uint<13> Hstart[(DST_COLS>>XF_BITSHIFT(NPC)) + 1];

	XF_TNAME(DEPTH,NPC) Data[(NPC+1)];
#pragma HLS ARRAY_PARTITION variable=Data complete

	uchar_t count=0;
	ap_uint<16> x,j=0,i=0,k,read_line,block_ind,block_start,BufferIndex,offset_temp=0,Yoffset, out_i=0, out_j=0;
	bool process_flag=0;

	
	#pragma HLS ALLOCATION instances=xFUDivNNDown limit=1 function
	uint64_t xnew,ynew;

	xnew = (width<<XF_BITSHIFT(NPC));///(float)(out_width<<XF_BITSHIFT(NPC));
	ynew = (height);//(float)(out_height);
	
	xnew = xnew << 28;
	ynew = ynew << 28;
	ap_ufixed<36,4> Xscale,Yscale;
	uint64_t Xscale64,Yscale64;
	Xscale64 = xFUDivNNDown (xnew , (out_width<<XF_BITSHIFT(NPC)));
	Yscale64 = xFUDivNNDown (ynew , (out_height));
	ap_ufixed<64,32> temp_scale_conv;
	
	temp_scale_conv = Xscale64;
	temp_scale_conv = temp_scale_conv >> 28;
	Xscale = temp_scale_conv;
	
	temp_scale_conv = Yscale64;
	temp_scale_conv = temp_scale_conv >> 28;
	Yscale = temp_scale_conv;
		
	Hstart[0] = Hoffset[0] = 0;
	Hoffset_loop:for(x=1;x<(out_width<<XF_BITSHIFT(NPC));x++)
	{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_COLS
		ap_fixed<26,16> dst_offset = (ap_fixed<36,16>)(x) * Xscale + (ap_fixed<36,16>)0.0001;
		ap_int<16> indexx = dst_offset;

		Hoffset[x] = indexx;

		count++;
		if(count == (1<<XF_BITSHIFT(NPC)))
		{
			count = 0;
			i++;
			Hstart[i] = indexx;
		}
	}


	Voffset[0] = 0;
	Voffset_loop:for(x=1;x<out_height;x++)
	{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_ROWS
		ap_fixed<26,16> dst_offset = (ap_fixed<32,16>)(x) * Yscale + (ap_fixed<36,16>)0.0001;
		ap_uint<16> indexy = (ap_uint<16>)dst_offset;
		
		if((indexy+1)<height)
		{
			Voffset[x] = indexy;
		}
		else
		{
			Voffset[x] = height-1;
		}
	}

	for (x=0;x<width;x++){
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_TC
		lbuf_in[0][x] = stream_in.read();
	}

	outerloop:for(j=0;j<height;j++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_ROWS

		Yoffset = Voffset[out_j];

		if (Yoffset == j)
		{
			process_flag = 1;
			out_i = 0;
			out_j++;
		}
		else
		{
			process_flag = 0;
		}
		// std::cout << "width: " << width << " NPC: " << NPC << "\nwidth shift NPC: " << (width<<XF_BITSHIFT(NPC)) << " inc: " << (1<<XF_BITSHIFT(NPC)) << "\n";
		// std::cout << "in width count: " << i << " in height count: " << j << "\nout width count: " << out_i << " out height count: " << out_j << "\n";
		// std::cout << "Yoffset: " << Yoffset << std::endl << std::endl << std::endl;

		innerloop:for(i=0;i<(width<<XF_BITSHIFT(NPC));i=i+(1<<XF_BITSHIFT(NPC)))
		{
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_TC
	
			if(j<height-1)
			{
				if(j%2 == 0)
				{
					lbuf_in[1][i>>XF_BITSHIFT(NPC)] = stream_in.read();
				}
				else
				{
					lbuf_in[0][i>>XF_BITSHIFT(NPC)] = stream_in.read();
				}
			}

			block_ind = out_i >> XF_BITSHIFT(NPC);
			block_start = (NPC == XF_NPPC1) ? Hoffset[block_ind]:Hstart[block_ind];
			BufferIndex = (block_start >> XF_BITSHIFT(NPC));

			for(k=0;k<(XF_BITSHIFT(NPC)+1);k++)
			{
#pragma HLS UNROLL
				if((BufferIndex + k) < width)
				{	
					if(j%2 == 0)
					{
						Data[k] = lbuf_in[0][BufferIndex + k];
					}
					else
					{
						Data[k] = lbuf_in[1][BufferIndex + k];
					}
				}
				else
					Data[k] = 0;
			}

			XF_TNAME(DEPTH,NPC) out_pix = ProcessBlockNNDown<DEPTH,NPC,WORDWIDTH>(Hoffset,Data,block_start,out_i);
				
			if(process_flag && out_i<(out_width<<XF_BITSHIFT(NPC)))
			{
				resize_out.write(out_pix);
				out_i += 1<<XF_BITSHIFT(NPC);
			}
		}
	}
}
#endif
