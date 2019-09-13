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

#ifndef _XF_RESIZE_UP_BILINEAR_
#define _XF_RESIZE_UP_BILINEAR_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "core/xf_math.h"
#include "common/xf_utility.h"

#define EXTRACT2			2		//no. of 64-bit varibles to be read for processing
#define POW32		4294967296

static const float xF_inv_pyramid_scale_double = 0.5;

/*
 *	Coreprocessing Processing Block
 *
 *	PixelValue = A0*(1-Wx)*(1-Wy) + B0*(Wx)*(1-Wy) + A1*(1-Wx)*(Wy) + B1*(Wx)*(Wy)
 *			   = Wx*Wy*(A0+B1-B0-A1) + Wx*(B0-A0) + Wy*(A1-A0) + A0
 */
template<int DEPTH>
static void CoreProcessUpBilinear(uchar_t A0,uchar_t B0,uchar_t A1,uchar_t B1,ap_ufixed<12,2> Wx,ap_ufixed<12,2> Wy,XF_PTUNAME(DEPTH) *pixel)
{
#pragma HLS PIPELINE
	ap_ufixed<12,2> Wxy;
	ap_int<16> val0,val1,val2;
	ap_fixed<28,18> P1,P2,P3,P4;
	ap_ufixed<28,18> one_num = 1.0;
	
	Wxy = (Wx*Wy);    // Wx - 0.32, Wy-0.32  (Wx*Wy-0.64)  Wxy - 0.32
	val0 = (A0+B1-(B0+A1));
	val1 = (B0-A0);
	val2 = (A1-A0);

	P1 = ((ap_fixed<28,18>)val0*Wxy);		// val0(16.0) * Wxy(0.32) = P1(16.32)
	P2 = ((ap_fixed<28,18>)val1*Wx);		// val1(16.0) * Wx(0.32) = P2(16.32)
	P3 = ((ap_fixed<28,18>)val2*Wy);		// val1(16.0) * Wy(0.32) = P3(16.32)
	P4 = ((ap_fixed<28,18>)A0);					// A0(8.0) P4(8.32)

	*pixel = (uchar_t)((P1  + P2 + P3 + P4));
	// to get only integer part from sum of 8.32's , right shift by 32
}

/*
 * Processes the 8 pixel block
 * outputs 8 pixles packed into 64-bit
 */
template<int DEPTH,int NPC,int WORDWIDTH>
static XF_TNAME(DEPTH,NPC) ProcessBlockBilinearUp(ap_uint<13> *Offset,ap_ufixed<12,2> *Weight,ap_ufixed<12,2> Yweight,XF_TNAME(DEPTH,NPC) *D0,XF_TNAME(DEPTH,NPC) *D1,ap_uint<13> blockstart,ap_uint<13> indoffset)
{
#pragma HLS INLINE OFF

	XF_PTUNAME(DEPTH) line0[2<<XF_BITSHIFT(NPC)],line1[2<<XF_BITSHIFT(NPC)];	// holds the unpacked pixeldata
#pragma HLS ARRAY_PARTITION variable=line0 complete
#pragma HLS ARRAY_PARTITION variable=line1 complete

	XF_PTUNAME(DEPTH) Pixels[1<<XF_BITSHIFT(NPC)];
#pragma HLS ARRAY_PARTITION variable=Pixels complete

	uchar_t i,input_read;
	uint16_t block_start_ind;

	block_start_ind = (blockstart>>XF_BITSHIFT(NPC))<<XF_BITSHIFT(NPC);

	for(i=0;i<2;i++){
#pragma HLS unroll
		xfExtractPixels<NPC,WORDWIDTH,DEPTH>(line0,D0[i],i<<XF_BITSHIFT(NPC));
	}
	for(i=0;i<2;i++){
#pragma HLS unroll
		xfExtractPixels<NPC,WORDWIDTH,DEPTH>(line1,D1[i],i<<XF_BITSHIFT(NPC));
	}

	XF_TNAME(DEPTH,NPC) val =0;
	int shift = 0;
	process_block_loop:for(i=0;i<(1<<XF_BITSHIFT(NPC));i++)
	{
#pragma HLS UNROLL
		//input_read = (NPC == XF_NPPC1) ?0:(Offset[indoffset+i] - block_start_ind);
		if(NPC == XF_NPPC1)
		{
			input_read = 0;
		}
		else
		{
			input_read = (uchar_t) (Offset[indoffset+i] - block_start_ind);
		}
		CoreProcessUpBilinear<DEPTH>(line0[input_read],line0[input_read + 1],line1[input_read],line1[input_read + 1],Weight[indoffset+i],Yweight,&Pixels[i]);
		shift = i<<XF_BITSHIFT(NPC);
		val.range(shift+7,shift) = Pixels[i];
	}
	return val;
}
static uint32_t xFUDivBilinearUp (uint64_t in_n, unsigned short in_d)
{
	#pragma HLS INLINE OFF
	uint32_t out_res = in_n/in_d;
	return out_res;
}

template<int SRC_ROWS,int SRC_COLS,int DEPTH,int NPC,int WORDWIDTH,int DST_ROWS,int DST_COLS,int SRC_TC,int DST_TC>
void xFResizeBilinearUpscale(hls::stream <XF_TNAME(DEPTH,NPC)> &stream_in, hls::stream <XF_TNAME(DEPTH,NPC)> &resize_out, unsigned short height, unsigned short width, unsigned short out_height, unsigned short out_width)
{
	XF_TNAME(DEPTH,NPC) lbuf_in0[SRC_COLS>>XF_BITSHIFT(NPC)];		// input buffers (ping pong)
	XF_TNAME(DEPTH,NPC) lbuf_in1[SRC_COLS>>XF_BITSHIFT(NPC)];		// input buffers (ping pong)
	XF_TNAME(DEPTH,NPC) lbuf_in2[SRC_COLS>>XF_BITSHIFT(NPC)];		// input buffers (ping pong)
	ap_uint<13> Hoffset[DST_COLS],Voffset[DST_ROWS];		// offset buffers which indicate from where the data is to be read
	if (NPC!=XF_NPPC1)
	{
#pragma HLS ARRAY_PARTITION variable=Hoffset cyclic factor=8 dim=1
	}

	ap_ufixed<12,2> Hweight[DST_COLS], Vweight[DST_ROWS];		// buffers which hold the weights for each corresponding input pixels
	if (NPC!=XF_NPPC1)
	{
#pragma HLS ARRAY_PARTITION variable=Hweight cyclic factor=8 dim=1
	}

	XF_TNAME(DEPTH,NPC) D0[2],D1[2],tmp_in;					// Holds the packed pixels required for processing
#pragma HLS ARRAY_PARTITION variable=D0 complete
#pragma HLS ARRAY_PARTITION variable=D1 complete

	unsigned char repcount=0,datacount=0,idx;
	ap_uint<13> Hstart[DST_COLS + 1];			// Buffers holding the starting offset for each 8pixel block
	ap_uint<13> x,Yoffset,offset_temp,prev_offset_temp=0, prev_y=-1,j=0,k,i,ii=0,block_ind,block_start,bufferIndex;
	ap_ufixed<32,1> Xscale,Yscale;
	ap_ufixed<12,2> Yweight;
	int64_t Xtemp,Ytemp;

	uint64_t xnew,ynew;

	xnew = (width<<XF_BITSHIFT(NPC));///(float)(out_width<<XF_BITSHIFT(NPC));
	ynew = (height);//(float)(out_height);
	
	xnew = xnew << 32;
	ynew = ynew << 32;
		
#pragma HLS ALLOCATION instances=xFUDivBilinearUp limit=1 function
	uint32_t Xscale32,Yscale32;
	Xscale32 = xFUDivBilinearUp (xnew , (out_width<<XF_BITSHIFT(NPC)));
	Yscale32 = xFUDivBilinearUp (ynew , (out_height));
		
	ap_ufixed<64,32> temp_scale_conv;
	
	temp_scale_conv = Xscale32;
	temp_scale_conv = temp_scale_conv >> 32;
	Xscale = temp_scale_conv;
	
	temp_scale_conv = Yscale32;
	temp_scale_conv = temp_scale_conv >> 32;
	Yscale = temp_scale_conv;

	/* Calculating required Horizontal parameters*/

	Hoffset[0] = 0;	 Hweight[0] = 0; Hstart[0] = 0;

	Hoffset_loop:for(x=1,ii=0;x<(out_width<<XF_BITSHIFT(NPC));x++)
	{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_TC
		ap_fixed<26,16> dst_offset = (ap_fixed<36,16>)((ap_fixed<26,16>)x  + (ap_fixed<26,16>)0.5) * Xscale - (ap_fixed<26,16>)0.5;
		ap_int<16> indexx = dst_offset;
		if(indexx < 0)
		{
			Hweight[x] = 0;
			indexx = 0;
		}
		else
		{
			if(indexx < ((width<<XF_BITSHIFT(NPC)) -1))
			{
				
				Hweight[x] = dst_offset - (ap_fixed<26,16>)(indexx);
			}
			else
			{
				Hweight[x] = 0;
			}
		}
		repcount++;
		if(prev_offset_temp != (ap_uint<16>)dst_offset){
			datacount = datacount + repcount;
			repcount = 0;
		}
		if(datacount >= (1<<XF_BITSHIFT(NPC)))
		{
			datacount = datacount - (1<<XF_BITSHIFT(NPC));
			ii++;
			//Hstart[ii] = datacount>0?prev_offset_temp:(prev_offset_temp+1);		// checking the crossover pixel condition
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
	Voffset[0] = 0;  	Vweight[0] = 0;
	Voffset_loop:for(x=1;x<out_height;x++)
	{
#pragma HLS pipeline II=1
#pragma HLS LOOP_TRIPCOUNT min=1 max = DST_ROWS
		ap_fixed<26,16> dst_offset = (ap_fixed<32,16>)((ap_fixed<26,16>)x  + (ap_fixed<26,16>)0.5) * Yscale - (ap_fixed<26,16>)0.5;
		ap_uint<16> indexy = (ap_uint<16>)dst_offset;
		if(indexy < 0)
		{
			Vweight[x] = 0;
			indexy = 0;
		}
		else
		{
			if(indexy < height-1)
			{
				Vweight[x] = dst_offset - indexy;
			}
			else
			{
				Vweight[x] = 0;
			}
		}
		if((indexy+1)<height)
		{
			Voffset[x] = indexy;
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
		tmp_in = stream_in.read();
		lbuf_in0[x] = tmp_in;
	}
	out_j++;
	for (x=0;x<width;x++){
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max = SRC_TC
		tmp_in = stream_in.read();
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
			block_start = (NPC == XF_NPPC1) ? Hoffset[block_ind]:Hstart[block_ind];	// block_start is index of the input pixel to be read in image dimesions
			bufferIndex = block_start>>XF_BITSHIFT(NPC);
			if(read_flag && i<width<<XF_BITSHIFT(NPC) && out_j < height)
			{
				if(read_into==0)
				{
					lbuf_in0[i>>XF_BITSHIFT(NPC)] = stream_in.read();
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
					lbuf_in1[i>>XF_BITSHIFT(NPC)] = stream_in.read();
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
					lbuf_in2[i>>XF_BITSHIFT(NPC)] = stream_in.read();
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
			
			/*	ProcessBlock function processes 8 pixels at a time	*/
			XF_TNAME(DEPTH,NPC) out_val = ProcessBlockBilinearUp<DEPTH,NPC,WORDWIDTH>(Hoffset,Hweight,Yweight,D0,D1,block_start,i);
			resize_out.write(out_val);
		}
	}
}
#endif
