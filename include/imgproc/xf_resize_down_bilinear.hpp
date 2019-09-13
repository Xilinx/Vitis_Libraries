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

#ifndef _XF_RESIZE_DOWN_BILINEAR_
#define _XF_RESIZE_DOWN_BILINEAR_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
//#define POW32	4294967296   // 2^32

/*
 *	Coreprocessing Processing Block
 *
 *	PixelValue = A0*(1-Wx)*(1-Wy) + B0*(Wx)*(1-Wy) + A1*(1-Wx)*(Wy) + B1*(Wx)*(Wy)
 *			   = Wx*Wy*(A0+B1-B0-A1) + Wx*(B0-A0) + Wy*(A1-A0) + A0
 */
template<int DEPTH>
static void CoreProcessDownBilinear(XF_PTUNAME(DEPTH) A0,XF_PTUNAME(DEPTH) B0,XF_PTUNAME(DEPTH) A1,XF_PTUNAME(DEPTH) B1,ap_ufixed<12,2> Wx,ap_ufixed<12,2> Wy,XF_PTUNAME(DEPTH) *pixel)
{
#pragma HLS inline off

	ap_ufixed<12,2> Wxy;
	ap_int<16> val0,val1,val2;
	ap_fixed<28,18> P1,P2,P3,P4;
	ap_ufixed<28,18> one_num = 1.0;
	
	Wxy = (Wx*Wy);    // Wx - 0.32, Wy-0.32  (Wx*Wy-0.64)  Wxy - 0.32
	val0 = (A0+B1-(B0+A1));
	val1 = (B0-A0);
	val2 = (A1-A0);

	P1 = (val0*Wxy);		// val0(16.0) * Wxy(0.32) = P1(16.32)
	P2 = (val1*Wx);		// val1(16.0) * Wx(0.32) = P2(16.32)
	P3 = (val2*Wy);		// val1(16.0) * Wy(0.32) = P3(16.32)
	P4 = (A0);					// A0(8.0) P4(8.32)

	*pixel = (uchar_t)((P1  + P2 + P3 + P4));
	// to get only integer part from sum of 8.32's , right shift by 32
}

/*
 * Processes the 8 pixel block
 * outputs 8 pixels packed into 64-bit
 */
template<int DEPTH,int NPC,int WORDWIDTH>
static XF_TNAME(DEPTH,NPC) ProcessBlockBilinearDown(ap_uint<13> *Offset,ap_ufixed<12,2> *Weight,ap_ufixed<12,2> Yweight,XF_TNAME(DEPTH,NPC) *D0,XF_TNAME(DEPTH,NPC) *D1,uint16_t blockstart,int offset)
{
#pragma HLS INLINE off
	XF_PTUNAME(DEPTH) line0[4<<(XF_BITSHIFT(NPC))],line1[4<<XF_BITSHIFT(NPC)];		// holds the unpacked pixel data
#pragma HLS ARRAY_PARTITION variable=line0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=line1 complete dim=1

	uchar_t i,input_read;
	uint16_t block_start_ind;
	int16_t shift;
	XF_TNAME(DEPTH,NPC) val;
	XF_PTUNAME(DEPTH) Pixels[1<<XF_BITSHIFT(NPC)];
#pragma HLS ARRAY_PARTITION variable=Pixels complete

	block_start_ind = (blockstart>>XF_BITSHIFT(NPC))<<XF_BITSHIFT(NPC);

	for(i=0;i<(4);i++){
#pragma HLS unroll
		xfExtractPixels<NPC,WORDWIDTH,DEPTH>(line0,D0[i],i<<XF_BITSHIFT(NPC));
	}

	for(i=0;i<(4);i++){
#pragma HLS unroll
		xfExtractPixels<NPC,WORDWIDTH,DEPTH>(line1,D1[i],i<<XF_BITSHIFT(NPC));
	}

	val =0;
	shift = 0;
	process_block_loop:for(i=0;i<(1<<XF_BITSHIFT(NPC));i++)
	{
#pragma HLS UNROLL
		//input_read = (NPC == XF_NPPC1) ?0:Offset[offset+i] - block_start_ind;
		if(NPC == XF_NPPC1)
		{
			input_read =0;
		}
		else
		{
			input_read = Offset[offset+i] - block_start_ind;
		}
		CoreProcessDownBilinear<DEPTH>(line0[input_read],line0[input_read+1],line1[input_read],line1[input_read+1],Weight[offset+i],Yweight,&Pixels[i]);
		shift = i<<XF_BITSHIFT(NPC);
		val.range(shift+7,shift) = Pixels[i];
	}
	return val;
}
static uint64_t xFUDivBilinearDown (uint64_t in_n, unsigned short in_d)
{
	#pragma HLS INLINE OFF
	uint32_t out_res = in_n/in_d;
	return out_res;
}
/**
 * Stream implementation of resizing the image using bilinear interpolation technique.
 */
template<int SRC_ROWS,int SRC_COLS,int DEPTH,int NPC,int WORDWIDTH,int DST_ROWS,int DST_COLS,int SRC_TC,int DST_TC>
void xFResizeBilinearDownScale(hls::stream <XF_TNAME(DEPTH,NPC)> &stream_in, hls::stream <XF_TNAME(DEPTH,NPC)> &resize_out, unsigned short height, unsigned short width, unsigned short out_height, unsigned short out_width)
{
	XF_TNAME(DEPTH,NPC) lbuf_in0[(SRC_COLS>>XF_BITSHIFT(NPC))];  // input buffers of size width by 8 or 16
	XF_TNAME(DEPTH,NPC) lbuf_in1[(SRC_COLS>>XF_BITSHIFT(NPC))];  // input buffers of size width by 8 or 16
	XF_TNAME(DEPTH,NPC) lbuf_in2[(SRC_COLS>>XF_BITSHIFT(NPC))];  // input buffers of size width by 8 or 16
#pragma HLS RESOURCE variable=lbuf_in0 core=RAM_T2P_BRAM
#pragma HLS DEPENDENCE variable=lbuf_in0 inter false
#pragma HLS DEPENDENCE variable=lbuf_in0 intra false

#pragma HLS RESOURCE variable=lbuf_in1 core=RAM_T2P_BRAM
#pragma HLS DEPENDENCE variable=lbuf_in1 inter false
#pragma HLS DEPENDENCE variable=lbuf_in1 intra false

#pragma HLS RESOURCE variable=lbuf_in2 core=RAM_T2P_BRAM
#pragma HLS DEPENDENCE variable=lbuf_in2 inter false
#pragma HLS DEPENDENCE variable=lbuf_in2 intra false

	const int NPX_PER_CYC = (NPC == XF_NPPC1)? 2 : 4;

	ap_uint<13> Hoffset[DST_COLS],Voffset[DST_ROWS];		// offset buffers which indicate from where the data is to be read
	if (NPC!=XF_NPPC1)
	{
#pragma HLS ARRAY_PARTITION variable=Hoffset cyclic factor=8 dim=1
	}

	ap_ufixed<12,2> Hweight[DST_COLS],Vweight[DST_ROWS];		// buffers which hold the weights for each corresponding input pixels
	if (NPC!=XF_NPPC1)
	{
#pragma HLS ARRAY_PARTITION variable=Hweight cyclic factor=8 dim=1
	}
	ap_uint<13> Hstart[(DST_COLS>>XF_BITSHIFT(NPC)) + 1];

	XF_TNAME(DEPTH,NPC) D0[5],D1[5];
	XF_TNAME(DEPTH,NPC) D0T[5],D1T[5];
#pragma HLS ARRAY_PARTITION variable=D0 complete
#pragma HLS ARRAY_PARTITION variable=D1 complete

	uchar_t count=0,first,second,third,read_into;
	ap_uint<13> x,j=0,i=0,k,block_ind,block_start,offset_temp=0,Yoffset,read_offset,line_idx0,line_idx1,line_idx2,out_j, out_i;
	bool flag_resize = 0;

	line_idx0 = 0; 	line_idx1 = 0; 	line_idx2 = 0;
	first = 0; 		second = 1; third = 2;

	
	#pragma HLS ALLOCATION instances=xFUDivBilinearDown limit=1 function
	uint64_t xnew,ynew;

	xnew = (width<<XF_BITSHIFT(NPC));///(float)(out_width<<XF_BITSHIFT(NPC));
	ynew = (height);//(float)(out_height);
	
	xnew = xnew << 28;
	ynew = ynew << 28;
	ap_ufixed<36,4> Xscale,Yscale;
	ap_ufixed<12,2> Yweight;
	uint64_t Xscale64,Yscale64;
	Xscale64 = xFUDivBilinearDown (xnew , (out_width<<XF_BITSHIFT(NPC)));
	Yscale64 = xFUDivBilinearDown (ynew , (out_height));
	ap_ufixed<64,32> temp_scale_conv;
	
	temp_scale_conv = Xscale64;
	temp_scale_conv = temp_scale_conv >> 28;
	Xscale = temp_scale_conv;
	
	temp_scale_conv = Yscale64;
	temp_scale_conv = temp_scale_conv >> 28;
	Yscale = temp_scale_conv;
			/* Calculating required Horizontal parameters*/
    Hstart[0] = 0;
	Hoffset_loop:for(x=0;x<(out_width<<XF_BITSHIFT(NPC));x++)
	{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_COLS
		ap_fixed<26,16> dst_offset = (ap_fixed<36,16>)((ap_fixed<26,16>)x  + (ap_fixed<26,16>)0.5) * Xscale - (ap_fixed<26,16>)0.5;
		ap_int<16> indexx = dst_offset;
		Hweight[x] = dst_offset-indexx; 	// Extracting fractional part
		Hoffset[x] = indexx;
		count++;
		if(count == (1<<XF_BITSHIFT(NPC))+1)
		{
			count = 1;
			i++;
			Hstart[i] = indexx;
		}
	}
	
	Voffset_loop:for(x=0;x<out_height;x++)
	{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_ROWS
		ap_fixed<26,16> dst_offset = (ap_fixed<32,16>)((ap_fixed<26,16>)x  + (ap_fixed<26,16>)0.5) * Yscale - (ap_fixed<26,16>)0.5;
		ap_uint<16> indexy = (ap_uint<16>)dst_offset;
		Voffset[x] = indexy;
		Vweight[x] = dst_offset - indexy; // Extracting fractional part
	}


	first = 0;
	second = 1;
	third = 2;
	read_into = 0;
	out_i = 0;
	out_j = 0;
	line_idx0 = 0;
	line_idx1 = 1;
	line_idx2 = -1;
	for (x=0;x<width;x++){
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_TC
		lbuf_in0[x] = stream_in.read();
	}
	for (x=0;x<width;x++){
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_TC
		lbuf_in1[x] = stream_in.read();
	}
	
	outerloop:for(j=0;j<height;j++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_ROWS
		Yoffset = Voffset[out_j];
		Yweight = Vweight[out_j];

		if(j == 0)
			read_into = 2;
		else
		{
			if(read_into<2)
			{
				read_into++;
			}
			else
			{
				read_into = 0;
			}
		}
		if (Yoffset < height-1){
			if (Yoffset == line_idx0 && Yoffset+1 == line_idx1 && read_into == 2){
				first = 0;
				second = 1;
				flag_resize = 1;
				out_i = 0;
				out_j++;
			}
			else if (Yoffset == line_idx1 && Yoffset+1 == line_idx2 && read_into == 0){
				first = 1;
				second = 2;
				flag_resize = 1;
				out_i = 0;
				out_j++;
			}
			else if (Yoffset == line_idx2 && Yoffset+1 == line_idx0 && read_into == 1){
				first = 2;
				second = 0;
				flag_resize = 1;
				out_i = 0;
				out_j++;
			}
			else
			{
				flag_resize = 0;
			}
		}
		else
			Yweight = 0;
		// std::cout << "width: " << width << " NPC: " << NPC << "\nwidth shift NPC: " << (width<<XF_BITSHIFT(NPC)) << " inc: " << (1<<XF_BITSHIFT(NPC)) << "\n";
		// std::cout << "in width count: " << i << " in height count: " << j << "\nout width count: " << out_i << " out height count: " << out_j << "\n";
		// std::cout << "line_idx0: " << line_idx0 << " line_idx1: " << line_idx1 <<  " line_idx2: " << line_idx2 << "\nYoffset: " << Yoffset << std::endl << std::endl << std::endl;
		block_start = Hstart[0];			// block_start is index of the input pixel to be read in image dimensions
		read_offset = (block_start >> 3);	// buffer_index gives the 64-bit data to be read to get the block_start pixel

		innerloop:for(i=0;i<(width<<XF_BITSHIFT(NPC));i=i+(1<<XF_BITSHIFT(NPC)))    /// processing 8 pixels at a time
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_TC

			block_ind = out_i >> XF_BITSHIFT(NPC);
			//block_start = (NPC == XF_NPPC1) ? Hoffset[block_ind]:Hstart[block_ind];
			if(NPC == XF_NPPC1)
			{
				block_start =  Hoffset[block_ind];
			}
			else
			{
				block_start =  Hstart[block_ind];
			}
			read_offset = (block_start >> XF_BITSHIFT(NPC));
				
			if(j<height-2){
				if(read_into == 0)
				{
					lbuf_in0[i>>(XF_BITSHIFT(NPC))] = stream_in.read();
					if(first == 2)
					{
						for(k=0;k<NPX_PER_CYC;k++)
						{
						#pragma HLS UNROLL
							if((k+read_offset) < width )
							{
								D0T[k] = lbuf_in2[read_offset+k];
								D1T[k] = lbuf_in1[read_offset+k];
							}
						}
					}
					else
					{
						for(k=0;k<NPX_PER_CYC;k++)
						{
						#pragma HLS UNROLL
							if((k+read_offset) < width )
							{
								D0T[k] = lbuf_in1[read_offset+k];
								D1T[k] = lbuf_in2[read_offset+k];
							}
						}
					}
				}
				else if(read_into == 1)
				{
					lbuf_in1[i>>(XF_BITSHIFT(NPC))] = stream_in.read();
					if(first == 2)
					{
						for(k=0;k<NPX_PER_CYC;k++)
						{
						#pragma HLS UNROLL
							if((k+read_offset) < width )
							{
								D0T[k] = lbuf_in2[read_offset+k];
								D1T[k] = lbuf_in0[read_offset+k];
							}
						}
					}
					else
					{
						for(k=0;k<NPX_PER_CYC;k++)
						{
						#pragma HLS UNROLL
							if((k+read_offset) < width )
							{
								D0T[k] = lbuf_in0[read_offset+k];
								D1T[k] = lbuf_in2[read_offset+k];
							}
						}
					}
				}
				else if(read_into == 2)
				{
					lbuf_in2[i>>(XF_BITSHIFT(NPC))] = stream_in.read();
					if(first == 1)
					{
						for(k=0;k<NPX_PER_CYC;k++)
						{
						#pragma HLS UNROLL
							if((k+read_offset) < width )
							{
								D0T[k] = lbuf_in1[read_offset+k];
								D1T[k] = lbuf_in0[read_offset+k];
							}
						}
					}
					else
					{
						for(k=0;k<NPX_PER_CYC;k++)
						{
						#pragma HLS UNROLL
							if((k+read_offset) < width )
							{
								D0T[k] = lbuf_in0[read_offset+k];
								D1T[k] = lbuf_in1[read_offset+k];
							}
						}
					}
				}
			}
			else
			{
				for(k=0;k<NPX_PER_CYC;k++)
				{
				#pragma HLS UNROLL
					if((k+read_offset) < width )
					{
						if(first == 0)
						{
							D0T[k] = lbuf_in0[read_offset+k];
							if(second == 1)
								D1T[k] = lbuf_in1[read_offset+k];
							else
								D1T[k] = lbuf_in2[read_offset+k];
						}
						else if (first == 1)
						{
							D0T[k] = lbuf_in1[read_offset+k];
							if(second == 2)
								D1T[k] = lbuf_in2[read_offset+k];
							else
								D1T[k] = lbuf_in0[read_offset+k];
						}
						else
						{
							D0T[k] = lbuf_in2[read_offset+k];
							if(second == 1)
								D1T[k] = lbuf_in1[read_offset+k];
							else
								D1T[k] = lbuf_in0[read_offset+k];
						}						
					}
				}
			}
			if(flag_resize && out_i<(out_width<<XF_BITSHIFT(NPC)) && read_into != first && read_into != second)
			{				
				for(k=0;k<NPX_PER_CYC;k++)
				{
	#pragma HLS UNROLL
					if((k+read_offset) < width )
					{
						// D0[k] = lbuf_in[first][read_offset + k];
						// D1[k] = lbuf_in[second][read_offset + k];
						D0[k] = D0T[k];
						D1[k] = D1T[k];
					}
					else
					{
						D0[k] = 0;
						D1[k] = 0;
					}
				}
				XF_TNAME(DEPTH,NPC) out_pix = ProcessBlockBilinearDown<DEPTH,NPC,WORDWIDTH>(Hoffset,Hweight,Yweight,D0,D1,block_start,out_i);
				resize_out.write(out_pix);
				out_i=out_i+(1<<XF_BITSHIFT(NPC));
			}
		}
		if(read_into == 0)
		{
			line_idx0 = j + 2;
		}
		else if(read_into == 1)
		{
			line_idx1 = j + 2;
		}
		else if(read_into == 2)
		{
			line_idx2 = j + 2;
		}
	}
}
#endif
