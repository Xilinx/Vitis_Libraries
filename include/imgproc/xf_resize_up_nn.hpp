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

#ifndef _XF_RESIZE_UP_NN_
#define _XF_RESIZE_UP_NN_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "core/xf_math.h"
#include "common/xf_utility.h"

/*
 * Processes the 8 pixel block
 * outputs 8 pixels packed into 64-bit
 */
template<int DEPTH,int NPC,int WORDWIDTH>
static XF_TNAME(DEPTH,NPC) ProcessBlockNNUp(ap_uint<13> *Offset,XF_TNAME(DEPTH,NPC) *Data,ap_uint<13> blockstart,ap_uint<13> indoffset)
{
#pragma HLS INLINE OFF

	XF_PTUNAME(DEPTH) line0[(2<<XF_BITSHIFT(NPC))];
#pragma HLS ARRAY_PARTITION variable=line0 complete dim=1

	XF_PTUNAME(DEPTH) Pixel[(1<<XF_BITSHIFT(NPC))];
#pragma HLS ARRAY_PARTITION variable=Pixel complete

	uchar_t i,input_read;
	uint16_t block_start_ind;

	block_start_ind = (blockstart>>XF_BITSHIFT(NPC))<<XF_BITSHIFT(NPC);

	// unpack Data from Data to line0
	for(i=0;i<((XF_BITSHIFT(NPC)+2)>>1);i++){
#pragma HLS unroll
		xfExtractPixels<NPC,WORDWIDTH,DEPTH>(line0,Data[i],i<<3);
	}

	XF_TNAME(DEPTH,NPC) val =0;
	int shift = 0;
	process_block_loop:for(i=0;i<(1<<XF_BITSHIFT(NPC));i++)
	{
#pragma HLS UNROLL
		//input_read = (NPC == XF_NPPC1) ?0:Offset[indoffset+i] - block_start_ind;
		if(NPC == XF_NPPC1)
		{
			input_read = 0;
		}else
		{
			input_read = Offset[indoffset+i] - block_start_ind;
		}
		Pixel[i] = line0[input_read];
		shift = i<<3;
		val.range(shift+7,shift) = Pixel[i];
	}
	return val;
}
static uint32_t xFUDivNNUP (uint64_t in_n, unsigned short in_d)
{
	#pragma HLS INLINE OFF
	uint32_t out_res = in_n/in_d;
	return out_res;
}
/**
 * Stream implementation of resizing the image using nearest neighbourhood interpolation technique.
 */
template<int SRC_ROWS,int SRC_COLS,int DEPTH,int NPC,int WORDWIDTH,int DST_ROWS,int DST_COLS,int SRC_TC,int DST_TC>
void xFResizeNNUpScale(hls::stream <XF_TNAME(DEPTH,NPC)> &stream_in, hls::stream <XF_TNAME(DEPTH,NPC)> &resize_out, unsigned short height, unsigned short width, unsigned short out_height, unsigned short out_width)
{
	XF_TNAME(DEPTH,NPC) lbuf_in0[(SRC_COLS>>XF_BITSHIFT(NPC))];  // input buffers
	XF_TNAME(DEPTH,NPC) lbuf_in1[(SRC_COLS>>XF_BITSHIFT(NPC))];  // input buffers

	ap_uint<13> Hoffset[DST_COLS],Voffset[DST_ROWS];		// offset buffers which indicate from where the data is to be read
	if (NPC!=XF_NPPC1)
	{
#pragma HLS ARRAY_PARTITION variable=Hoffset cyclic factor=8 dim=1
	}
	ap_uint<13> Hstart[(DST_COLS>>XF_BITSHIFT(NPC)) + 1];

	XF_TNAME(DEPTH,NPC) Data[(XF_BITSHIFT(NPC)+2)>>1];
#pragma HLS ARRAY_PARTITION variable=Data complete

	ap_uint<13> x,j=0,i=0,prev_y=-1,ii=0;
	ap_uint<13> block_ind,block_start,BufferIndex,offset_temp=0,prev_offset_temp=0,Yoffset,in_j=0,read_flag=0;
	uint64_t Xtemp=0,Ytemp=0;
	uchar_t repcount = 1,datacount=0;
	
	#pragma HLS ALLOCATION instances=xFUDivNNUP limit=1 function
	uint64_t xnew,ynew;

	xnew = (width<<XF_BITSHIFT(NPC));///(float)(out_width<<XF_BITSHIFT(NPC));
	ynew = (height);//(float)(out_height);
	
	xnew = xnew << 32;
	ynew = ynew << 32;
	ap_ufixed<32,1> Xscale,Yscale;
	uint32_t Xscale32,Yscale32;
	Xscale32 = xFUDivNNUP (xnew , (out_width<<XF_BITSHIFT(NPC)));
	Yscale32 = xFUDivNNUP (ynew , (out_height));
		
	ap_ufixed<64,32> temp_scale_conv;
	
	temp_scale_conv = Xscale32;
	temp_scale_conv = temp_scale_conv >> 32;
	Xscale = temp_scale_conv;
	
	temp_scale_conv = Yscale32;
	temp_scale_conv = temp_scale_conv >> 32;
	Yscale = temp_scale_conv;

	/* Calculating required Horizontal parameters*/

	Hoffset[0] = 0;	 Hstart[0] = 0;

	Hoffset_loop:for(x=1,ii=0;x<(out_width<<XF_BITSHIFT(NPC));x++)
	{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_TC
		ap_fixed<26,16> dst_offset = (ap_fixed<36,16>)(x) * Xscale + (ap_fixed<36,16>)0.0001;
		ap_int<16> indexx = dst_offset;
		
		repcount++;
		if(prev_offset_temp != indexx){
			datacount = datacount + repcount;
			repcount = 0;
		}
		if(datacount >= (1<<XF_BITSHIFT(NPC)))
		{
			datacount = datacount - (1<<XF_BITSHIFT(NPC));
			ii++;
			if(datacount > 0)
			{
				Hstart[ii] = prev_offset_temp;
			}
			else
			{
				Hstart[ii] = prev_offset_temp + 1;
			}
		}

		Hoffset[x] = indexx;
		prev_offset_temp = (ap_uint<16>)dst_offset;
	}
	/* Calculating required Vertical parameters*/
	Voffset[0] = 0;
	Voffset_loop:for(x=1;x<out_height;x++)
	{
#pragma HLS pipeline II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max = DST_ROWS
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

	
	for(i=0;i<(width);i++){
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_TC
		lbuf_in0[i] = stream_in.read();
	}
	in_j = 0;
	outerloop:for(j=0;j<out_height;j++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_ROWS
		if(read_flag)
			in_j++;
		
		Yoffset = Voffset[j];
		
		if(in_j != Yoffset)
			std::cout << "Error: not matching" <<std::endl;
		
		if(j<out_height-1)
		{
			if(Voffset[j+1] == in_j+1)
			{ 
				read_flag = 1;
			}
			else
			{
				read_flag = 0;
			}
		}
		else
		{
			if(in_j < height-1)
				read_flag = 1;
			else
				read_flag = 0;
		}
		
		innerloop:for(i=0;i<(out_width<<XF_BITSHIFT(NPC));i=i+(1<<XF_BITSHIFT(NPC)))
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_TC
			block_ind = i >> XF_BITSHIFT(NPC);
			block_start = (NPC == XF_NPPC1) ? Hoffset[block_ind]:Hstart[block_ind];
			BufferIndex = (block_start >> XF_BITSHIFT(NPC));
			if(in_j<height-1 && i < width<<XF_BITSHIFT(NPC) && read_flag)
			{
				if(in_j%2==0)
				{
					lbuf_in1[i>>XF_BITSHIFT(NPC)] = stream_in.read();
					for(int8_t k=0;k<((2+XF_BITSHIFT(NPC))>>1);k++)
					{
		#pragma HLS UNROLL
						if((k+BufferIndex) < width)
						{
							Data[k] = lbuf_in0[BufferIndex+k];
						}
						else
						{
							Data[k] = 0;
						}
					}
				}
				else
				{
					lbuf_in0[i>>XF_BITSHIFT(NPC)] = stream_in.read();
					for(int8_t k=0;k<((2+XF_BITSHIFT(NPC))>>1);k++)
					{
		#pragma HLS UNROLL
						if((k+BufferIndex) < width)
						{
							Data[k] = lbuf_in1[BufferIndex+k];
						}
						else
						{
							Data[k] = 0;
						}
					}
				}
			}
			else
			{
				for(int8_t k=0;k<((2+XF_BITSHIFT(NPC))>>1);k++)
				{
	#pragma HLS UNROLL
					if((k+BufferIndex) < width)
					{
						if(in_j%2==0)
							Data[k] = lbuf_in0[BufferIndex+k];
						else
							Data[k] = lbuf_in1[BufferIndex+k];
					}
					else
					{
						Data[k] = 0;
					}
				}
			}
			
			XF_TNAME(DEPTH,NPC) out_pix = ProcessBlockNNUp<DEPTH,NPC,WORDWIDTH>(Hoffset,Data,block_start,i);
			resize_out.write(out_pix);
		}
	}
}
#endif
