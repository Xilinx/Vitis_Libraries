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

#ifndef _XF_RESIZE_UP_AREA_
#define _XF_RESIZE_UP_AREA_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "core/xf_math.h"
#include "common/xf_utility.h"
//#define POW32	4294967296   // 2^32

/*
 *	Coreprocessing Processing Block
 *
 *	PixelValue = A0*(1-Wx)*(1-Wy) + B0*(Wx)*(1-Wy) + A1*(1-Wx)*(Wy) + B1*(Wx)*(Wy)
 *			   = Wx*Wy*(A0+B1-B0-A1) + Wx*(B0-A0) + Wy*(A1-A0) + A0
 */
static void CoreProcessUpArea(uchar_t A0,uchar_t B0,uchar_t A1,uchar_t B1,uint32_t Wx,uint32_t Wy,uchar_t *pixel)
{
#pragma HLS PIPELINE
	uint32_t Wxy;
	int16_t val0,val1,val2;
	int64_t P1,P2,P3,P4;

	Wxy = ((uint64_t)Wx*Wy)>>32;   // Wx - 0.32, Wy-0.32  (Wx*Wy-0.64)  Wxy - 0.32
	val0 = (A0+B1-(B0+A1));
	val1 = (B0-A0);
	val2 = (A1-A0);

	P1 = ((int64_t)val0*Wxy);		// val0(16.0) * Wxy(0.32) = P1(16.32)
	P2 = ((int64_t)val1*Wx);		// val1(16.0) * Wx(0.32) = P2(16.32)
	P3 = ((int64_t)val2*Wy);		// val1(16.0) * Wy(0.32) = P3(16.32)
	P4 = ((int64_t)A0<<32);			// A0(8.0) P4(8.32)

	*pixel = (uchar_t)((P1  + P2 + P3 + P4)>>32);	// to get only integer part from sum of 8.32's , right shift by 32
}

/*
 * Processes the 8 pixel block
 * outputs 8 pixles packed into 64-bit
 */
template<int DEPTH,int NPC,int WORDWIDTH,int PLANES>
static XF_TNAME(DEPTH,NPC) ProcessBlockAreaUp(ap_uint<13> Offset[],uint32_t Weight[],uint32_t Yweight,XF_TNAME(DEPTH,NPC) *D0,XF_TNAME(DEPTH,NPC) *D1,ap_uint<13> blockstart,ap_uint<13> indoffset)
{
#pragma HLS INLINE
	XF_PTUNAME(DEPTH) line0[4<<XF_BITSHIFT(NPC)],line1[4<<XF_BITSHIFT(NPC)];		// holds the unpacked pixeldata
#pragma HLS ARRAY_PARTITION variable=line0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=line1 complete dim=1

	uchar_t i,input_read,Pixels;
	uint16_t block_start_ind = (blockstart>>XF_BITSHIFT(NPC))<<XF_BITSHIFT(NPC);

	for(i=0;i<2;i++){
#pragma HLS unroll
		xfExtractPixels<NPC,WORDWIDTH,XF_DEPTH(DEPTH,NPC)>(line0,D0[i],i<<XF_BITSHIFT(NPC));
		xfExtractPixels<NPC,WORDWIDTH,XF_DEPTH(DEPTH,NPC)>(line1,D1[i],i<<XF_BITSHIFT(NPC));
	}

	XF_TNAME(DEPTH,NPC) val = 0;
	int shift = 0;
	process_block_loop:for(i=0;i<(1<<XF_BITSHIFT(NPC));i++)
	{
#pragma HLS UNROLL
		//input_read = (NPC == XF_NPPC1) ?0:Offset[indoffset+i] - block_start_ind;

		if(NPC == XF_NPPC1)
		{
			input_read =0;
		}
		else
		{
			input_read = Offset[indoffset+i] - block_start_ind;
		}
		for(ap_uint<5> c=0,k=0;c<3;c++,k+=8)
		{

		if(PLANES!=1)
		{
			CoreProcessUpArea(line0[input_read].range(k+7,k),line0[input_read+1].range(k+7,k),line1[input_read].range(k+7,k),line1[input_read+1].range(k+7,k),Weight[indoffset+i],Yweight,&Pixels);
			val.range(k+7,k) = Pixels;
		}
		else
		{
			CoreProcessUpArea(line0[input_read],line0[input_read+1],line1[input_read],line1[input_read+1],Weight[indoffset+i],Yweight,&Pixels);
			shift = i<<XF_BITSHIFT(NPC);
			val.range(shift+7,shift) = Pixels;
		}
		}
	}
	return val;
}
static uint64_t xFUDivAreaUp (uint64_t in_n, unsigned short in_d)
{
	#pragma HLS INLINE OFF
	uint64_t out_res = in_n/in_d;
	return out_res;
}

/**
 * Upscaling - Area Interpolation
 */
template<int SRC_ROWS,int SRC_COLS,int PLANES,int DEPTH,int NPC,int WORDWIDTH,int DST_ROWS,int DST_COLS,int SRC_TC,int DST_TC>
void xFResizeAreaUpScale(xf::Mat<DEPTH, SRC_ROWS, SRC_COLS, NPC> &stream_in, xf::Mat<DEPTH, DST_ROWS, DST_COLS, NPC> &resize_out, unsigned short height, unsigned short width, unsigned short out_height, unsigned short out_width)
{
	XF_TNAME(DEPTH,NPC) lbuf_in0[SRC_COLS>>XF_BITSHIFT(NPC)];		// input buffers (ping pong)
	XF_TNAME(DEPTH,NPC) lbuf_in1[SRC_COLS>>XF_BITSHIFT(NPC)];		// input buffers (ping pong)
	XF_TNAME(DEPTH,NPC) lbuf_in2[SRC_COLS>>XF_BITSHIFT(NPC)];		// input buffers (ping pong)
	ap_uint<13> Hoffset[DST_COLS],Voffset[DST_ROWS];			// offset buffers which indicate from where the data is to be read
if (NPC!=XF_NPPC1)
{
#pragma HLS ARRAY_PARTITION variable=Hoffset cyclic factor=8 dim=1
}
	uint32_t Hweight[DST_COLS],Vweight[DST_ROWS+1];		// buffers which hold the weights for each corresponding input pixels
if (NPC!=XF_NPPC1)
{
#pragma HLS ARRAY_PARTITION variable=Hweight cyclic factor=8 dim=1
}
	uchar_t idx=0,repcount=0,datacount=0;
	uint16_t Hstart[(DST_COLS>>XF_BITSHIFT(NPC)) + 1];						// Buffers holding the starting offset for each 8pixel block
	ap_uint<13> x,read_line,block_ind,block_start,bufferIndex;
	ap_uint<13> prev_y=-1,j=0,i=0,k,ii=0,Yoffset,offset_temp,prev_offset_temp=0;
	uint32_t Xscale,Yscale,Yweight;
	uint64_t inv_Xscale,inv_Yscale;
	int64_t Xtemp = 0,Ytemp = 0;
	int read_index = 0, write_index = 0;
	//float Xscale_float,Yscale_float,inv_Xscale_float,inv_Yscale_float;
	XF_TNAME(DEPTH,NPC) D0[2],D1[2];			// Holds the packed pixels required for processing

//	Xscale_float = (width<<XF_BITSHIFT(NPC))/(float)(out_width<<XF_BITSHIFT(NPC));
//	Yscale_float = height/(float)out_height;
//	inv_Xscale_float = (out_width<<XF_BITSHIFT(NPC))/(float)(width<<XF_BITSHIFT(NPC));
//	inv_Yscale_float = out_height/(float)height;

#pragma HLS ALLOCATION instances=xFUDivAreaUp limit=1 function
	Xscale = xFUDivAreaUp( (uint64_t)(width<<XF_BITSHIFT(NPC))*POW32 , (out_width<<XF_BITSHIFT(NPC)));
	Yscale = xFUDivAreaUp( (uint64_t)height*POW32 , out_height);
	inv_Xscale = xFUDivAreaUp( (uint64_t)(out_width<<XF_BITSHIFT(NPC))*POW32 , (width<<XF_BITSHIFT(NPC)));
	inv_Yscale = xFUDivAreaUp( (uint64_t)out_height*POW32 , height);

	/* Calculating required Horizontal parameters*/
	Hstart[0] = 0;
	Hoffset_loop:for(x=0,ii=0;x<(out_width<<XF_BITSHIFT(NPC));x++)
	{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_COLS
		offset_temp = ((uint64_t)x*Xscale+429496)>>32;					 		// Extracting only the integer part,x(16.0) Xscale(0.32)
		Xtemp = ((uint64_t)(x+1)<<32) - (offset_temp + 1)*inv_Xscale;			//inv_Xscale 32.32
		if(Xtemp < 0)
			Hweight[x] = 0;
		else
			Hweight[x] = (uint32_t)(Xtemp & 0xFFFFFFFF); // Extracting fractional part

		repcount++;
		if(prev_offset_temp != offset_temp){
			datacount = datacount + repcount;
			repcount = 0;
		}
		if(datacount >= (1<<XF_BITSHIFT(NPC)))
		{
			datacount = datacount - (1<<XF_BITSHIFT(NPC));
			//Hstart[ii+1] = datacount>0?prev_offset_temp:(prev_offset_temp+1);
			if(datacount > 0)
			{
				Hstart[ii+1] = prev_offset_temp;
			}
			else
			{
				Hstart[ii+1] = prev_offset_temp + 1;
			}
			ii++;
		}
		Hoffset[x] = offset_temp;
		prev_offset_temp = offset_temp;
	}

	/* Calculating required Vertical parameters*/
	Voffset_loop:for(x=0;x<out_height;x++)
	{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_ROWS

		offset_temp = ((uint64_t)x*Yscale+429496)>>32;				// Yscale(0.32)  Extracting only the integer part
		Ytemp = ((uint64_t)(x+1)<<32) - (offset_temp + 1)*inv_Yscale;
		if(Ytemp < 0)
			Vweight[x] = 0;
		else
			Vweight[x] = offset_temp<(height-1)?(uint32_t)(Ytemp & 0xFFFFFFFF):0;

		//Voffset[x] = (offset_temp+1)<(height)?(offset_temp+1):height;

		if((offset_temp)<height)
		{
			Voffset[x] = (offset_temp);
		}
		else
		{
			Voffset[x] = height-1;
		}
	}

	idx=0;

	bool read_flag=0;
	ap_uint<2> l0=0,l1=1,l2=2,read_into=2;
	ap_uint<16> lind0=0,lind1=1,lind2=65535,out_j=0;
	for (x=0;x<width;x++){
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max = SRC_TC
		XF_TNAME(DEPTH,NPC) tmp_in = stream_in.read(read_index++);
		lbuf_in0[x] = tmp_in;
	}
	out_j++;
	for (x=0;x<width;x++){
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max = SRC_TC
		XF_TNAME(DEPTH,NPC) tmp_in = stream_in.read(read_index++);
		lbuf_in1[x] = tmp_in;
	}
	out_j++;
	
	outerloop:for(j=0;j<out_height;j++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max = DST_ROWS
		if(read_flag)
		{
			if(read_into==2)
			{
				lind2 = out_j;
			}
			else if(read_into==1)
			{
				lind1 = out_j;
			}
			else
			{
				lind0 = out_j;
			}
			out_j++;
		}
		Yoffset = Voffset[j];		// row to be read
		Yweight = Vweight[j];		// weight of the next row
		
		if(Yoffset == lind0 && (Yoffset+1) == lind1)
		{	
			read_into = 2;
			l0 = 0;
			l1 = 1;
		}
		else if(Yoffset == lind1 && (Yoffset+1) == lind2)
		{	
			read_into = 0;
			l0 = 1;
			l1 = 2;
		}
		else if(Yoffset == lind2 && (Yoffset+1) == lind0)
		{	
			read_into = 1;
			l0 = 2;
			l1 = 0;
		}
		
		if(j < out_height-1)
		{
			if(Voffset[j+1] != 	Voffset[j])
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
			read_flag = 0;
		}
			
		innerloop:for(i=0;i<(out_width<<XF_BITSHIFT(NPC));i=i+(1<<XF_BITSHIFT(NPC)))    ///loop on column  --  processing 8 pixels at a time
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_TC avg=DST_TC
			block_ind = i >> XF_BITSHIFT(NPC);
			block_start = (NPC == XF_NPPC1) ? (ap_uint<13>)Hoffset[block_ind]:(ap_uint<13>)Hstart[block_ind];	// block_start is index of the input pixel to be read in image dimesions
			bufferIndex = block_start>>XF_BITSHIFT(NPC);
			if(read_flag && i<width<<XF_BITSHIFT(NPC) && out_j < height)
			{
				if(read_into==0)
				{
					lbuf_in0[i>>XF_BITSHIFT(NPC)] = stream_in.read(read_index++);
					for(k=0;k<2;k++)
					{
		#pragma HLS UNROLL
						if((k+bufferIndex) < width)
						{
							D0[k] = lbuf_in1[bufferIndex+k];
							D1[k] = lbuf_in2[bufferIndex+k];
						}
						else
						{
							D0[k] = 0;
							D1[k] = 0;
						}
					}
				}
				else if(read_into==1)
				{
					lbuf_in1[i>>XF_BITSHIFT(NPC)] = stream_in.read(read_index++);
					for(k=0;k<2;k++)
					{
		#pragma HLS UNROLL
						if((k+bufferIndex) < width)
						{
							D0[k] = lbuf_in2[bufferIndex+k];
							D1[k] = lbuf_in0[bufferIndex+k];
						}
						else
						{
							D0[k] = 0;
							D1[k] = 0;
						}
					}
				}
				else
				{
					lbuf_in2[i>>XF_BITSHIFT(NPC)] = stream_in.read(read_index++);
					for(k=0;k<2;k++)
					{
		#pragma HLS UNROLL
						if((k+bufferIndex) < width)
						{
							D0[k] = lbuf_in0[bufferIndex+k];
							D1[k] = lbuf_in1[bufferIndex+k];
						}
						else
						{
							D0[k] = 0;
							D1[k] = 0;
						}
					}
				}
			}
			else
			{
				for(k=0;k<2;k++)
				{
	#pragma HLS UNROLL
					if((k+bufferIndex) < width)
					{
						if(l0==0)
						{
							D0[k] = lbuf_in0[bufferIndex+k];
							if(l1==1)
								D1[k] = lbuf_in1[bufferIndex+k];
							else
								D1[k] = lbuf_in2[bufferIndex+k];
						}
						else if(l0==1)
						{
							D0[k] = lbuf_in1[bufferIndex+k];
							if(l1==0)
								D1[k] = lbuf_in0[bufferIndex+k];
							else
								D1[k] = lbuf_in2[bufferIndex+k];
						}
						else
						{
							D0[k] = lbuf_in2[bufferIndex+k];
							if(l1==0)
								D1[k] = lbuf_in0[bufferIndex+k];
							else
								D1[k] = lbuf_in1[bufferIndex+k];
						}
					}
					else
					{
						D0[k] = 0;
						D1[k] = 0;
					}
				}
			}
			XF_TNAME(DEPTH,NPC) out_pix = ProcessBlockAreaUp<DEPTH,NPC,WORDWIDTH,PLANES>(Hoffset,Hweight,Yweight,D0,D1,block_start,i);
			resize_out.write(write_index++,out_pix);
		}
	}
}



#endif
