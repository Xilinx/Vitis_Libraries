
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
#ifndef _XF_CVT_COLOR_HPP_
#define _XF_CVT_COLOR_HPP_

#include "hls_stream.h"
#include "common/xf_common.h"
#include "xf_cvt_color_1.hpp"
#include "xf_cvt_color_utils.hpp"
#include <assert.h>

namespace xf {
template<int SRC_T,int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC,int TC>
void write_y_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<DST_T, ROWS, COLS, NPC> & out_y,uint16_t height,uint16_t width)
{
XF_SNAME(WORDWIDTH_SRC) tmp;
unsigned long long int idx=0;
for(int i=0;i<height;i++)
{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
	for(int j=0;j<(width>>XF_BITSHIFT(NPC));j++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
		tmp=src_y.read(i*(width>>XF_BITSHIFT(NPC))+j);
		out_y.write(idx++,tmp);
	}
}
}

//KernRgba2Yuv4
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int PLANES, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernRgba2Yuv4_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst1, xf::Mat<DST_T, ROWS, COLS, NPC> & dst2, xf::Mat<DST_T, ROWS, COLS, NPC> & dst3,uint16_t height,uint16_t width)
{
//	width=width>>NPC;
	XF_PTNAME(XF_8UP) Y0[16], U[16], V[16];
	uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0 complete
#pragma HLS ARRAY_PARTITION variable=U complete
#pragma HLS ARRAY_PARTITION variable=V complete
#pragma HLS ARRAY_PARTITION variable=RGB complete
unsigned long long int y_idx=0,u_idx=0,v_idx=0;
	XF_SNAME(WORDWIDTH_SRC)  PackedPixels;
	XF_SNAME(WORDWIDTH_DST) YPacked, UPacked, VPacked;
	uint8_t offset;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = src.read(i*width+j);
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			//	Converting from RGBA to YUV4
			//		Y =  (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
			//		U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128
			//		V =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
			for(int l = 0; l < (1<<XF_BITSHIFT(NPC))>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
				//#pragma HLS unroll
				if(PLANES==4)
				{
				offset = l << 3;
				Y0[(l<<1)]   = CalculateY(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
				Y0[(l<<1)+1] = CalculateY(RGB[offset + 4], RGB[offset + 5], RGB[offset + 6]);

				U[(l<<1)]   = CalculateU(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
				U[(l<<1)+1] = CalculateU(RGB[offset + 4], RGB[offset + 5], RGB[offset + 6]);

				V[(l<<1)]   = CalculateV(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
				V[(l<<1)+1] = CalculateV(RGB[offset + 4], RGB[offset + 5], RGB[offset + 6]);
				}
				else
				{
					offset = l * 6;
					Y0[(l<<1)]   = CalculateY(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
					Y0[(l<<1)+1] = CalculateY(RGB[offset + 3],RGB[offset + 4], RGB[offset + 5]);

					U[(l<<1)]   = CalculateU(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
					U[(l<<1)+1] = CalculateU(RGB[offset + 3], RGB[offset + 4], RGB[offset + 5]);

					V[(l<<1)]   = CalculateV(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
					V[(l<<1)+1] = CalculateV(RGB[offset + 3], RGB[offset + 4], RGB[offset + 5]);

				}
			}
			YPacked = PackPixels<WORDWIDTH_DST>(Y0);
			UPacked = PackPixels<WORDWIDTH_DST>(U);
			VPacked = PackPixels<WORDWIDTH_DST>(V);

			dst1.write(y_idx++,YPacked);
			dst2.write(u_idx++,UPacked);
			dst3.write(v_idx++,VPacked);
		}
	}
}

//KernRgba2Iyuv
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int PLANES,int WORDWIDTH_SRC, int WORDWIDTH_DST,int ROWS_U,int ROWS_V,int TC,int iTC>
void KernRgba2Iyuv_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & rgba, xf::Mat<DST_T, ROWS, COLS, NPC> & y_plane, xf::Mat<DST_T, ROWS/4, COLS, NPC> & u_plane, xf::Mat<DST_T, ROWS/4, COLS, NPC> & v_plane,uint16_t height,uint16_t width)
{

	ap_uint8_t Y0[16], U[16], V[16]; uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0  complete
#pragma HLS ARRAY_PARTITION variable=U   complete
#pragma HLS ARRAY_PARTITION variable=V   complete
#pragma HLS ARRAY_PARTITION variable=RGB complete
unsigned long long int y_idx=0,out_idx=0,out_idx1=0;
	XF_SNAME(WORDWIDTH_SRC)  PackedPixels;
	XF_SNAME(WORDWIDTH_DST) YPacked, UPacked, VPacked;

	uint8_t Ycount = 0, UVcount = 0;
	int  offset;
	uchar_t UVoffset_ind,l;
    ap_uint<13> i,j;
	UVoffset_ind = (1<<XF_BITSHIFT(NPC))>>1;

	bool evenRow = true, evenBlock = true;
	rowloop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = rgba.read(i*width+j);
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			for( l = 0; l < (1<<XF_BITSHIFT(NPC))>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				if(PLANES==4)
				{
				offset = l << 3;
					Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					Y0[(l<<1)+1] = CalculateY(RGB[offset+4], RGB[offset+5], RGB[offset+6]);
				}
				else
				{
					offset = l * 6;
					Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					Y0[(l<<1)+1] = CalculateY(RGB[offset+3], RGB[offset+4], RGB[offset+5]);

				}
				if(evenRow)	//As Sampling rate is 2, Calculating U and V components only for even rows
				{
					/* 128 is added to U and V values to make them always positive and in studio range 16-240 */
					if(evenBlock)
					{
						U[l] = CalculateU(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
						V[l] = CalculateV(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					}
					else
					{
						U[UVoffset_ind + l] = CalculateU(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
						V[UVoffset_ind + l] = CalculateV(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					}
				}
			}
			YPacked = PackPixels<WORDWIDTH_DST>(Y0);
			y_plane.write(y_idx++,YPacked);
			if(evenRow & !evenBlock)
			{
				UPacked = PackPixels<WORDWIDTH_DST>(U);
				VPacked = PackPixels<WORDWIDTH_DST>(V);
				u_plane.write(out_idx++,UPacked);
				v_plane.write(out_idx1++,VPacked);
			}
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
//	if(((ROWS+1)>>1) & 0x1)
//	{	// Filling the empty region with zeros, when the height is multiple of 2 but not a multiple of 4
//		for( i = 0; i < width; i++)
//		{
//#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
//			u_plane.write(0);
//			v_plane.write(0);
//		}
//	}
}

//KernRgba2Nv12
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC,int NPC_UV,int PLANES, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void KernRgba2Nv12_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & rgba, xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & uv_plane,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	XF_PTNAME(XF_8UP) Y0[16], UV[16];
	uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0  complete
#pragma HLS ARRAY_PARTITION variable=UV  complete
#pragma HLS ARRAY_PARTITION variable=RGB complete
	XF_SNAME(WORDWIDTH_SRC)  PackedPixels;
	XF_SNAME(WORDWIDTH_Y) YPacked, UVPacked;
	unsigned long long int idx=0,idx1=0;
	uint8_t offset;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = rgba.read(i*width+j);
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			for(int l = 0; l < (1<<XF_BITSHIFT(NPC))>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				if(PLANES==4)
				{
				offset = l << 3;
				Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				Y0[(l<<1)+1] = CalculateY(RGB[offset+4], RGB[offset+5], RGB[offset+6]);
				}
				else
				{
					offset = l * 6;
					Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					Y0[(l<<1)+1] = CalculateY(RGB[offset+3], RGB[offset+4], RGB[offset+5]);

				}
				if(evenRow)
				{
					UV[l<<1]     = CalculateU(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					UV[(l<<1)+1] = CalculateV(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Y0);
			y_plane.write(idx++,YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_UV>(UV);
				uv_plane.write(idx1++,UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}
//KernRgba2Nv12
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC,int NPC_UV,int PLANES, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void Kernbgr2Nv12_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & rgba, xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & uv_plane,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	XF_PTNAME(XF_8UP) Y0[16], UV[16];
	uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0  complete
#pragma HLS ARRAY_PARTITION variable=UV  complete
#pragma HLS ARRAY_PARTITION variable=RGB complete
	XF_SNAME(WORDWIDTH_SRC)  PackedPixels;
	XF_SNAME(WORDWIDTH_Y) YPacked, UVPacked;
	unsigned long long int idx=0,idx1=0;
	uint8_t offset;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = rgba.read(i*width+j);
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			for(int l = 0; l < (1<<XF_BITSHIFT(NPC))>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				if(PLANES==4)
				{
//				offset = l << 3;
//				Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
//				Y0[(l<<1)+1] = CalculateY(RGB[offset+4], RGB[offset+5], RGB[offset+6]);
				}
				else
				{
					offset = l * 6;
					Y0[(l<<1)]   = CalculateY( RGB[offset+2], RGB[offset+1],RGB[offset+0]);
					Y0[(l<<1)+1] = CalculateY(RGB[offset+5], RGB[offset+4],RGB[offset+3] );

				}
				if(evenRow)
				{
					UV[l<<1]     = CalculateU(RGB[offset+2],RGB[offset+1], RGB[offset+0] );
					UV[(l<<1)+1] = CalculateV( RGB[offset+2],RGB[offset+1], RGB[offset+0]);
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Y0);
			y_plane.write(idx++,YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_UV>(UV);
				uv_plane.write(idx1++,UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

//KernRgba2Nv21
template<int SRC_T,int Y_T,int UV_T, int ROWS, int COLS, int NPC,int NPC_UV,int PLANES, int WORDWIDTH_SRC, int WORDWIDTH_Y,int WORDWIDTH_VU, int TC, int iTC>
void KernRgba2Nv21_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & rgba, xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & vu_plane,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	uint16_t i, j, k, l;
	ap_uint8_t Y0[16], VU[16];
	uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0 complete
#pragma HLS ARRAY_PARTITION variable=VU complete
#pragma HLS ARRAY_PARTITION variable=RGB complete
	XF_SNAME(WORDWIDTH_SRC) PackedPixels;
	XF_SNAME(WORDWIDTH_Y) YPacked, VUPacked;
	uint8_t offset;
	unsigned long long int idx=0,idx1=0;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = (XF_SNAME(WORDWIDTH_SRC))rgba.read(i*width+j);
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			for(int l = 0; l < (1<<XF_BITSHIFT(NPC))>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				if(PLANES==4)
				{
				offset = l << 3;
				Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				Y0[(l<<1)+1] = CalculateY(RGB[offset+4], RGB[offset+5], RGB[offset+6]);
				}
				else
				{
					offset = l *6;
					Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					Y0[(l<<1)+1] = CalculateY(RGB[offset+3], RGB[offset+4], RGB[offset+5]);

				}
				if(evenRow)
				{
					VU[(l<<1)]   = CalculateV(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					VU[(l<<1)+1] = CalculateU(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Y0);
			y_plane.write(idx++,YPacked);
			if(evenRow)
			{
				VUPacked = PackPixels<WORDWIDTH_Y>(VU);
				vu_plane.write(idx1++,VUPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

template<int SRC_T,int Y_T,int UV_T, int ROWS, int COLS, int NPC,int NPC_UV,int PLANES, int WORDWIDTH_SRC, int WORDWIDTH_Y,int WORDWIDTH_VU, int TC, int iTC>
void Kernbgr2Nv21_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & rgba, xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & vu_plane,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	uint16_t i, j, k, l;
	ap_uint8_t Y0[16], VU[16];
	uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0 complete
#pragma HLS ARRAY_PARTITION variable=VU complete
#pragma HLS ARRAY_PARTITION variable=RGB complete
	XF_SNAME(WORDWIDTH_SRC) PackedPixels;
	XF_SNAME(WORDWIDTH_Y) YPacked, VUPacked;
	uint8_t offset;
	unsigned long long int idx=0,idx1=0;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = (XF_SNAME(WORDWIDTH_SRC))rgba.read(i*width+j);
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			for(int l = 0; l < (1<<XF_BITSHIFT(NPC))>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				if(PLANES==4)
				{
//				offset = l << 3;
//				Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
//				Y0[(l<<1)+1] = CalculateY(RGB[offset+4], RGB[offset+5], RGB[offset+6]);
				}
				else
				{
					offset = l *6;
					Y0[(l<<1)]   = CalculateY( RGB[offset+2], RGB[offset+1],RGB[offset+0]);
					Y0[(l<<1)+1] = CalculateY(RGB[offset+5], RGB[offset+4],RGB[offset+3] );

				}
				if(evenRow)
				{
					VU[(l<<1)]   = CalculateV(RGB[offset+2], RGB[offset+1], RGB[offset+0]);
					VU[(l<<1)+1] = CalculateU( RGB[offset+2], RGB[offset+1],RGB[offset+0]);
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Y0);
			y_plane.write(idx++,YPacked);
			if(evenRow)
			{
				VUPacked = PackPixels<WORDWIDTH_Y>(VU);
				vu_plane.write(idx1++,VUPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}



//KernIyuv2Rgba
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernIyuv2Rgba_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & in_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & in_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & in_v,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
//	ap_uint<13> i,j,k;
//	uchar_t k;
	XF_PTNAME(XF_8UP) RGB[64], Ybuf[16], Ubuf[16], Vbuf[16];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=Ubuf complete
#pragma HLS ARRAY_PARTITION variable=Vbuf complete

	hls::stream<XF_SNAME(WORDWIDTH_SRC)> UStream, VStream;
#pragma HLS STREAM variable=&UStream  depth=COLS
#pragma HLS STREAM variable=&VStream  depth=COLS

	XF_SNAME(WORDWIDTH_SRC)  YPacked, UPacked, VPacked;
	XF_SNAME(WORDWIDTH_DST) PackedPixels;
unsigned long long int idx=0,out_idx=0;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	uint8_t offset;
	bool evenRow = true, evenBlock = true;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YPacked = in_y.read(i*width+j);

			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Ybuf, YPacked, 0);
			if(evenBlock)
			{
				if(evenRow)
				{
					UPacked = in_u.read(idx);
					UStream.write(UPacked);
					VPacked = in_v.read(idx++);
					VStream.write(VPacked);
				}
				else
				{
					/* Copy of the U and V values are pushed into stream to be used for next row */
					UPacked = UStream.read();
					VPacked = VStream.read();
				}
				xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Ubuf, UPacked, 0);
				xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Vbuf, VPacked, 0);
				offset = 0;
			}
			else
			{
				offset = (1 << XF_BITSHIFT(NPC)) >> 1;
			}
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{	// Y00 and Y01 have a U and V values in common
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
				//Y01 = (Ybuf[(k<<1) + 1] > 16) ? (Ybuf[(k<<1)+1]-16) : 0;

				if((Ybuf[k<<1] > 16))
				{
					Y00 = (Ybuf[k<<1]-16);
				}
				else
				{
					Y00 = 0;
				}

				if((Ybuf[(k<<1) + 1] > 16))
				{
					Y01 = (Ybuf[(k<<1)+1]-16);
				}
				else
				{
					Y01 = 0;
				}

				U = Ubuf[k + offset] - 128;
				V = Vbuf[k + offset] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
				RGB[(k<<3)]     = CalculateR(Y00,V2Rtemp,V);		//R0
				RGB[(k<<3) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
				RGB[(k<<3) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
				RGB[(k<<3) + 3] = 255;					//A
				RGB[(k<<3) + 4] = CalculateR(Y01,V2Rtemp,V);		//R1
				RGB[(k<<3) + 5] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
				RGB[(k<<3) + 6] = CalculateB(Y01,U2Btemp,U);		//B1
				RGB[(k<<3) + 7] = 255;					//A
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			_rgba.write(out_idx++,PackedPixels);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false: true;
	}
}

//KernIyuv2Nv12
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_UV, int rTC, int cTC, int iTC>
void KernIyuv2Nv12_ro(xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,uint16_t height,uint16_t width)
{
	ap_uint<13> i,j;
	XF_PTNAME(XF_8UP) U[16], V[16];
#pragma HLS ARRAY_PARTITION variable=U complete
#pragma HLS ARRAY_PARTITION variable=V complete
unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_SRC) UVPacked0, UVPacked1, UPacked, VPacked;
	rowloop:
	for( i = 0; i < height>>1; i++)
	{
		/*
		 * Reading the plane interleaved U and V data from streams and packing them in pixel interleaved
		 * and writing out to UV stream
		 */
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=rTC max=rTC
		columnloop:
		for( j = 0; j < (width>>(1+XF_BITSHIFT(NPC))); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=cTC max=cTC
			UPacked = _u.read(idx);
			VPacked = _v.read(idx++);

			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(U, UPacked, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(V, VPacked, 0);
			// Packing with alternative U and V values for Pixel interleaving
#define AU_CVT_STEP 16
			ap_uint<4> off = (1 << XF_BITSHIFT(NPC)) >> 1;
			ap_uint<4> k;
			int l;
			for( k = 0,  l = 0; k < ((1<<XF_BITSHIFT(NPC))>>1); k++, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				UVPacked0.range(l+AU_CVT_STEP-1, l) = (U[k]) | ((ap_uint<16>)V[k] << (8));
				UVPacked1.range(l+AU_CVT_STEP-1, l) = (U[k+off]) | ((ap_uint<16>)V[k+off] << (8));
			}
			_uv.write(idx1++,UVPacked0);
			_uv.write(idx1++,UVPacked1);

		}
	}
}

//KernIyuv2Yuv4
template<int SRC_T,int ROWS, int COLS, int NPC, int WORDWIDTH, int rTC, int cTC, int iTC>
void KernIyuv2Yuv4_ro(xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _in_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _in_v, xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{
	XF_TNAME(SRC_T, NPC) arr[COLS >>XF_BITSHIFT(NPC)];
	XF_TNAME(SRC_T, NPC) arr1[COLS >>XF_BITSHIFT(NPC)];


	hls::stream<XF_TNAME(SRC_T, NPC)> inter_u,inter_v;
#pragma HLS stream variable=&inter_u depth=COLS/2
#pragma HLS stream variable=&inter_v depth=COLS/2
unsigned long long int idx=0,idx1=0;
	XF_PTNAME(XF_8UP) U[16], V[16];
#pragma HLS ARRAY_PARTITION variable=U complete
#pragma HLS ARRAY_PARTITION variable=V complete

	XF_SNAME(WORDWIDTH) IUPacked, IVPacked, UPacked0,
			VPacked0, UPacked1, VPacked1;
	rowloop:
	for(int i = 0; i < ((height>>2)<<1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=rTC max=rTC
		columnloop:
		for(int j = 0,k=0; j < ((width>>XF_BITSHIFT(NPC))>>1); j++,k+=2)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=cTC max=cTC
			IUPacked = _in_u.read(idx);
			IVPacked = _in_v.read(idx++);

			xfExtractPixels<NPC, WORDWIDTH, XF_8UP>(U, IUPacked, 0);
			xfExtractPixels<NPC, WORDWIDTH, XF_8UP>(V, IVPacked, 0);
#define AU_CVT_STEP 16
			int off = 1 << (2); // (1 << NPC) >> 1;
			for(int k = 0, l = 0; k <(1<<(2)); k++, l += AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				UPacked0.range(l+AU_CVT_STEP-1, l) = (U[k]) | ((ap_uint<16>)U[k] << (8));
				VPacked0.range(l+AU_CVT_STEP-1, l) = (V[k]) | ((ap_uint<16>)V[k] << (8));
				UPacked1.range(l+AU_CVT_STEP-1, l) = (U[k+off]) | ((ap_uint<16>)U[k+off] << (8));
				VPacked1.range(l+AU_CVT_STEP-1, l) = (V[k+off]) | ((ap_uint<16>)V[k+off] << (8));
			}
			_u_image.write((((i*2))*(_u_image.cols>>XF_BITSHIFT(NPC)))+k, UPacked0);
			_v_image.write((((i*2))*(_v_image.cols>>XF_BITSHIFT(NPC)))+k, VPacked0);
			_u_image.write((((i*2))*(_u_image.cols>>XF_BITSHIFT(NPC)))+k+1, UPacked1);
			_v_image.write((((i*2))*(_v_image.cols>>XF_BITSHIFT(NPC)))+k+1, VPacked1);

			inter_u.write(UPacked0);
			inter_v.write(VPacked0);
			inter_u.write(UPacked1);
			inter_v.write(VPacked1);
		}
		for(int j = 0; j < (_u_image.cols>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
			_u_image.write((((i*2)+1)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j, inter_u.read());
			_v_image.write((((i*2)+1)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j, inter_v.read());
		}
	}


}


//KernNv122Iyuv
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernNv122Iyuv_ro(xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) UV0[16], UV1[16];
#pragma HLS ARRAY_PARTITION variable=UV0 complete
#pragma HLS ARRAY_PARTITION variable=UV1 complete
unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_DST) UPacked, VPacked;
	XF_SNAME(WORDWIDTH_SRC) UVPacked0, UVPacked1;
ap_uint<13> i,j;
	rowloop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < ((width>>XF_BITSHIFT(NPC))>>1); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			UVPacked0 = _uv.read(idx++);
			UVPacked1 = _uv.read(idx++);

			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UV0, UVPacked0, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UV1, UVPacked1, 0);
			// Packing the U and V by picking even indeces for U and odd indeces for V
#define AU_CVT_STEP 16
			int sft = 1 << (XF_BITSHIFT(NPC)+2);
			int l;
			ap_uint<9> k;
			for(int k = 0, l = 0; k < (1<<(XF_BITSHIFT(NPC))); k+=4, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				VPacked.range(l+AU_CVT_STEP-1, l) = (UV0[k+1]) | ((ap_uint<16>)UV0[k+3] << (8));
				UPacked.range(l+AU_CVT_STEP-1, l) =  (UV0[k])  | ((ap_uint<16>)UV0[k+2] << (8));

				VPacked.range(l+sft+AU_CVT_STEP-1, l+sft) = (UV1[k+1]) | ((ap_uint<16>)UV1[k+3] << (8));
				UPacked.range(l+sft+AU_CVT_STEP-1, l+sft) = (UV1[k])   | ((ap_uint<16>)UV1[k+2] << (8));
			}
			_u.write(idx1,UPacked);
			_v.write(idx1++,VPacked);
		}
	}
}

//KernNv122Rgba
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV,int PLANES, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
void KernNv122Rgba_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & in_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & in_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	XF_PTNAME(XF_8UP) RGB[64], Ybuf[16], UVbuf[16];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete

	hls::stream<XF_SNAME(WORDWIDTH_UV)> UVStream;
#pragma HLS STREAM variable=&UVStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) YPacked; XF_SNAME(WORDWIDTH_UV) UVPacked;
	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	unsigned long long int uv_idx=0,out_idx=0;
	int8_t U, V;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC

			YPacked = in_y.read(i*width+j);
			xfExtractPixels<NPC, WORDWIDTH_Y, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				UVPacked = in_uv.read(uv_idx++);
				UVStream.write(UVPacked);
			}
			else // Keep a copy of UV row data in stream to use for oddrow
				UVPacked = UVStream.read();

			xfExtractPixels<NPC, WORDWIDTH_UV, XF_8UP>(UVbuf, UVPacked, 0);
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
				//Y01 = (Ybuf[(k<<1)+1] > 16) ? (Ybuf[(k<<1)+1] - 16) : 0;

				if((Ybuf[k<<1] > 16))
				{
					Y00 = (Ybuf[k<<1]-16);
				}
				else
				{
					Y00 = 0;
				}

				if((Ybuf[(k<<1)+1] > 16))
				{
					Y01 = (Ybuf[(k<<1)+1] - 16);
				}
				else
				{
					Y01 = 0;
				}

				U = UVbuf[k<<1] - 128;
				V = UVbuf[(k<<1)+1] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
				if(PLANES==4)
				{
				RGB[(k<<3) + 0] = CalculateR(Y00, V2Rtemp, V);	     //R0
				RGB[(k<<3) + 1] = CalculateG(Y00, U2Gtemp, V2Gtemp); //G0
				RGB[(k<<3) + 2] = CalculateB(Y00, U2Btemp, U);	     //B0
				RGB[(k<<3) + 3] = 255;				     //A
				RGB[(k<<3) + 4] = CalculateR(Y01, V2Rtemp, V);	     //R1
				RGB[(k<<3) + 5] = CalculateG(Y01, U2Gtemp, V2Gtemp); //G1
				RGB[(k<<3) + 6] = CalculateB(Y01, U2Btemp, U);	     //B0
				RGB[(k<<3) + 7] = 255;				     //A
				}
				else
				{
					RGB[(k*6) + 0] = CalculateR(Y00, V2Rtemp, V);	     //R0
					RGB[(k*6) + 1] = CalculateG(Y00, U2Gtemp, V2Gtemp); //G0
					RGB[(k*6) + 2] = CalculateB(Y00, U2Btemp, U);	     //B0
					RGB[(k*6) + 3] = CalculateR(Y01, V2Rtemp, V);	     //R1
					RGB[(k*6) + 4] = CalculateG(Y01, U2Gtemp, V2Gtemp); //G1
					RGB[(k*6) + 5] = CalculateB(Y01, U2Btemp, U);	     //B0

				}
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(out_idx++,PackedPixels);
		}
		evenRow = evenRow ? false : true;
	}
//	if(height & 1)
//	{
//		for(int i = 0; i < (width>>NPC); i++)
//		{
//#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
//			UVStream.read();
//		}
//	}
}
//KernNv122Rgba
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV,int PLANES, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
void KernNv122bgr_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & in_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & in_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	XF_PTNAME(XF_8UP) RGB[64], Ybuf[16], UVbuf[16];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete

	hls::stream<XF_SNAME(WORDWIDTH_UV)> UVStream;
#pragma HLS STREAM variable=&UVStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) YPacked; XF_SNAME(WORDWIDTH_UV) UVPacked;
	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	unsigned long long int uv_idx=0,out_idx=0;
	int8_t U, V;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC

			YPacked = in_y.read(i*width+j);
			xfExtractPixels<NPC, WORDWIDTH_Y, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				UVPacked = in_uv.read(uv_idx++);
				UVStream.write(UVPacked);
			}
			else // Keep a copy of UV row data in stream to use for oddrow
				UVPacked = UVStream.read();

			xfExtractPixels<NPC, WORDWIDTH_UV, XF_8UP>(UVbuf, UVPacked, 0);
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
				//Y01 = (Ybuf[(k<<1)+1] > 16) ? (Ybuf[(k<<1)+1] - 16) : 0;

				if((Ybuf[k<<1] > 16))
				{
					Y00 = (Ybuf[k<<1]-16);
				}
				else
				{
					Y00 = 0;
				}

				if((Ybuf[(k<<1)+1] > 16))
				{
					Y01 = (Ybuf[(k<<1)+1] - 16);
				}
				else
				{
					Y01 = 0;
				}

				U = UVbuf[k<<1] - 128;
				V = UVbuf[(k<<1)+1] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
				if(PLANES==4)
				{
				RGB[(k<<3) + 0] = CalculateR(Y00, V2Rtemp, V);	     //R0
				RGB[(k<<3) + 1] = CalculateG(Y00, U2Gtemp, V2Gtemp); //G0
				RGB[(k<<3) + 2] = CalculateB(Y00, U2Btemp, U);	     //B0
				RGB[(k<<3) + 3] = 255;				     //A
				RGB[(k<<3) + 4] = CalculateR(Y01, V2Rtemp, V);	     //R1
				RGB[(k<<3) + 5] = CalculateG(Y01, U2Gtemp, V2Gtemp); //G1
				RGB[(k<<3) + 6] = CalculateB(Y01, U2Btemp, U);	     //B0
				RGB[(k<<3) + 7] = 255;				     //A
				}
				else
				{
					RGB[(k*6) + 0] = CalculateB(Y00, U2Btemp, U);	     //B0
					RGB[(k*6) + 1] = CalculateG(Y00, U2Gtemp, V2Gtemp); //G0
					RGB[(k*6) + 2] = CalculateR(Y00, V2Rtemp, V);	     //R0
					RGB[(k*6) + 3] = CalculateB(Y01, U2Btemp, U);	     //B0
					RGB[(k*6) + 4] = CalculateG(Y01, U2Gtemp, V2Gtemp); //G1
					RGB[(k*6) + 5] = CalculateR(Y01, V2Rtemp, V);	     //R1

				}
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(out_idx++,PackedPixels);
		}
		evenRow = evenRow ? false : true;
	}
//	if(height & 1)
//	{
//		for(int i = 0; i < (width>>NPC); i++)
//		{
//#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
//			UVStream.read();
//		}
//	}
}

//KernNv122Yuv4
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
void KernNv122Yuv4_ro(xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _u,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) UV[16];
#pragma HLS ARRAY_PARTITION variable=UV complete
ap_uint<13> i,j;
	XF_SNAME(WORDWIDTH_UV) UPacked;
	XF_SNAME(WORDWIDTH_DST) VPacked, UVPacked;
		XF_SNAME(WORDWIDTH_DST) arr_UPacked[COLS>>(XF_BITSHIFT(NPC))],arr_VPacked[COLS>>(XF_BITSHIFT(NPC))];

	unsigned long long int idx=0,idx1=0;
	rowloop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			UVPacked = _uv.read(idx1++);
			xfExtractPixels<NPC, WORDWIDTH_DST, XF_8UP>(UV, UVPacked, 0);
#define AU_CVT_STEP 16
			for(int k = 0, l = 0; k < (1<<(XF_BITSHIFT(NPC))); k+=2, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				VPacked.range(l+AU_CVT_STEP-1, l) = (UV[k+1]) | ((ap_uint<16>)UV[k+1] << (8));
				UPacked.range(l+AU_CVT_STEP-1, l) = (UV[k])   | ((ap_uint<16>)UV[k] << (8));
			}
			_u.write(((i*2)*(_u.cols>>XF_BITSHIFT(NPC)))+j, UPacked);
			_v.write(((i*2)*(_v.cols>>XF_BITSHIFT(NPC)))+j, VPacked);
			arr_UPacked[j]=UPacked;
			arr_VPacked[j]=VPacked;
		}
		for( j = 0;j < (width>>XF_BITSHIFT(NPC)); j++)
		{
		_u.write((((i*2)+1)*(_u.cols>>XF_BITSHIFT(NPC)))+j, arr_UPacked[j]);
		_v.write((((i*2)+1)*(_v.cols>>XF_BITSHIFT(NPC)))+j, arr_VPacked[j]);
		}
	}
}

//KernNv212Iyuv
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernNv212Iyuv_ro(xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & in_uv,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & u_out,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & v_out,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) VU0[16], VU1[16];
#pragma HLS ARRAY_PARTITION variable=VU0 complete
#pragma HLS ARRAY_PARTITION variable=VU1 complete
ap_uint<13> i,j;
	XF_SNAME(WORDWIDTH_DST) UPacked, VPacked;
	XF_SNAME(WORDWIDTH_SRC) VUPacked0, VUPacked1;
	unsigned long long int idx=0,idx1=0;
int l;
ap_uint<4> k;
	rowloop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < ((width>>XF_BITSHIFT(NPC))>>1); j++)
		{	// reading UV pixel interleaved data and writing them into UStream and VStream
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			VUPacked0 = in_uv.read(idx++);
			VUPacked1 = in_uv.read(idx++);

			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(VU0, VUPacked0, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(VU1, VUPacked1, 0);

#define AU_CVT_STEP 16
			int sft = 1 << (XF_BITSHIFT(NPC)+2);
			for( k = 0, l = 0; k < (1<<(XF_BITSHIFT(NPC))); k+=4, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				UPacked.range(l+AU_CVT_STEP-1, l) = (VU0[k+1]) | ((ap_uint<16>)VU0[k+3] << (8));
				VPacked.range(l+AU_CVT_STEP-1, l) = (VU0[k])   | ((ap_uint<16>)VU0[k+2] << (8));

				UPacked.range(l+sft+AU_CVT_STEP-1, l+sft) = (VU1[k+1]) | ((ap_uint<16>)VU1[k+3] << (8));
				VPacked.range(l+sft+AU_CVT_STEP-1, l+sft) = (VU1[k])   | ((ap_uint<16>)VU1[k+2] << (8));
			}
			u_out.write(idx1,UPacked);
			v_out.write(idx1,VPacked);
			idx1++;
		}
	}
/*	if((height>>1)& 0x1)
	{
		// Writing 0's to fill the stream if the UV plane width is odd
		for(int i = 0; i < ((width>>XF_BITSHIFT(NPC))>>1); i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			u_out.write(idx1,0);
			v_out.write(idx1++,0);
		}
	}*/
}
//template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC, int NPC_UV,int PLANES,int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
//void KernNv212bgr_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & in_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & in_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height,uint16_t width)
//{
//	XF_PTNAME(XF_8UP) RGB[64],Ybuf[16],UVbuf[16];
//#pragma HLS ARRAY_PARTITION variable=RGB complete
//#pragma HLS ARRAY_PARTITION variable=Ybuf complete
//#pragma HLS ARRAY_PARTITION variable=UVbuf complete
//ap_uint<13> i,j;
//unsigned long long int in_idx=0,out_idx=0;
//int k;
//	hls::stream<XF_SNAME(WORDWIDTH_UV)> UVStream;
//#pragma HLS STREAM variable=&UVStream  depth=COLS
//	XF_SNAME(WORDWIDTH_Y) YPacked; XF_SNAME(WORDWIDTH_UV) UVPacked;
//	XF_SNAME(WORDWIDTH_DST) PackedPixels;
//	uint8_t Y00, Y01;
//	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
//	int8_t U, V;
//	bool evenRow = true;
//	rowloop:
//	for( i = 0; i < height; i++)
//	{
//#pragma HLS LOOP_FLATTEN off
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		columnloop:
//		for( j = 0; j < width; j++)
//		{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
//			YPacked = in_y.read(i*width+j);
//			xfExtractPixels<NPC, WORDWIDTH_Y, XF_8UP>(Ybuf, YPacked, 0);
//			if(evenRow)
//			{
//				UVPacked = in_uv.read(in_idx++);
//				UVStream.write(UVPacked);
//			}
//			else // Keep a copy of UV row data in stream to use for oddrow
//			{
//				UVPacked = UVStream.read();
//			}
//
//			xfExtractPixels<NPC, WORDWIDTH_UV, XF_8UP>(UVbuf, UVPacked, 0);
//			for( k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
//			{
//#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
//#pragma HLS unroll
//				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
//				//Y01 = (Ybuf[(k<<1)+1] > 16) ? (Ybuf[(k<<1)+1]-16) : 0;
//
//				if((Ybuf[k<<1] > 16))
//				{
//					Y00 = (Ybuf[k<<1]-16);
//				}
//				else
//				{
//					Y00 = 0;
//				}
//
//				if((Ybuf[(k<<1)+1] > 16))
//				{
//					Y01 = (Ybuf[(k<<1)+1]-16);
//				}
//				else
//				{
//					Y01 = 0;
//				}
//
//				V = UVbuf[k<<1] - 128;
//				U = UVbuf[(k<<1)+1] - 128;
//
//				V2Rtemp = V * (short int)V2R;
//				U2Gtemp = (short int)U2G * U;
//				V2Gtemp = (short int)V2G * V;
//				U2Btemp = U * (short int)U2B;
//
//				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
//				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
//				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
//
//					RGB[(k*6) + 0] = CalculateB(Y00,U2Btemp,U);		 	//B0
//					RGB[(k*6) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
//					RGB[(k*6) + 2] =  CalculateR(Y00,V2Rtemp,V);		//R0
//					RGB[(k*6) + 3] = CalculateB(Y01,U2Btemp,U);		    //B1
//					RGB[(k*6) + 4] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
//					RGB[(k*6) + 5] = CalculateR(Y01,V2Rtemp,V);		    //R1
//
//			}
//
//			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
//			rgba.write(out_idx++,PackedPixels);
//		}
//		evenRow = evenRow ? false : true;
//	}
////	if(height & 1)
////	{
////		for( i = 0; i < (width>>XF_BITSHIFT(NPC)); i++)
////		{
////#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
////			UVStream.read();
////		}
////	}
//}
////KernNv212Rgba
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC, int NPC_UV,int PLANES,int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
void KernNv212Rgba_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & in_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & in_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) RGB[64],Ybuf[16],UVbuf[16];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
ap_uint<13> i,j;
unsigned long long int in_idx=0,out_idx=0;
int k;
	hls::stream<XF_SNAME(WORDWIDTH_UV)> UVStream;
#pragma HLS STREAM variable=&UVStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) YPacked; XF_SNAME(WORDWIDTH_UV) UVPacked;
	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	bool evenRow = true;
	rowloop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YPacked = in_y.read(i*width+j);
			xfExtractPixels<NPC, WORDWIDTH_Y, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				UVPacked = in_uv.read(in_idx++);
				UVStream.write(UVPacked);
			}
			else // Keep a copy of UV row data in stream to use for oddrow
				UVPacked = UVStream.read();

			xfExtractPixels<NPC, WORDWIDTH_UV, XF_8UP>(UVbuf, UVPacked, 0);
			for( k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
				//Y01 = (Ybuf[(k<<1)+1] > 16) ? (Ybuf[(k<<1)+1]-16) : 0;

				if((Ybuf[k<<1] > 16))
				{
					Y00 = (Ybuf[k<<1]-16);
				}
				else
				{
					Y00 = 0;
				}

				if((Ybuf[(k<<1)+1] > 16))
				{
					Y01 = (Ybuf[(k<<1)+1]-16);
				}
				else
				{
					Y01 = 0;
				}

				V = UVbuf[k<<1] - 128;
				U = UVbuf[(k<<1)+1] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
				if(PLANES==4)
				{
				RGB[(k<<3) + 0] = CalculateR(Y00,V2Rtemp,V);		//R0
				RGB[(k<<3) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
				RGB[(k<<3) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
				RGB[(k<<3) + 3] = 255;					//A
				RGB[(k<<3) + 4] = CalculateR(Y01,V2Rtemp,V);		//R1
				RGB[(k<<3) + 5] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
				RGB[(k<<3) + 6] = CalculateB(Y01,U2Btemp,U);		//B0
				RGB[(k<<3) + 7] = 255;					//A
				}
				else
				{
					RGB[(k*6) + 0] = CalculateR(Y00,V2Rtemp,V);		//R0
					RGB[(k*6) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
					RGB[(k*6) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
					RGB[(k*6) + 3] = CalculateR(Y01,V2Rtemp,V);		//R1
					RGB[(k*6) + 4] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
					RGB[(k*6) + 5] = CalculateB(Y01,U2Btemp,U);		//B0

				}
			}

			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(out_idx++,PackedPixels);
		}
		evenRow = evenRow ? false : true;
	}
//	if(height & 1)
//	{
//		for( i = 0; i < (width>>XF_BITSHIFT(NPC)); i++)
//		{
//#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
//			UVStream.read();
//		}
//	}
}


////KernNv212bgr
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC, int NPC_UV,int PLANES,int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
void KernNv212bgr_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & in_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & in_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) RGB[64],Ybuf[16],UVbuf[16];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
ap_uint<13> i,j;
unsigned long long int in_idx=0,out_idx=0;
int k;
	hls::stream<XF_SNAME(WORDWIDTH_UV)> UVStream;
#pragma HLS STREAM variable=&UVStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) YPacked; XF_SNAME(WORDWIDTH_UV) UVPacked;
	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	bool evenRow = true;
	rowloop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YPacked = in_y.read(i*width+j);
			xfExtractPixels<NPC, WORDWIDTH_Y, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				UVPacked = in_uv.read(in_idx++);
				UVStream.write(UVPacked);
			}
			else // Keep a copy of UV row data in stream to use for oddrow
				UVPacked = UVStream.read();

			xfExtractPixels<NPC, WORDWIDTH_UV, XF_8UP>(UVbuf, UVPacked, 0);
			for( k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
				//Y01 = (Ybuf[(k<<1)+1] > 16) ? (Ybuf[(k<<1)+1]-16) : 0;

				if((Ybuf[k<<1] > 16))
				{
					Y00 = (Ybuf[k<<1]-16);
				}
				else
				{
					Y00 = 0;
				}

				if((Ybuf[(k<<1)+1] > 16))
				{
					Y01 = (Ybuf[(k<<1)+1]-16);
				}
				else
				{
					Y01 = 0;
				}

				V = UVbuf[k<<1] - 128;
				U = UVbuf[(k<<1)+1] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
//				if(PLANES==4)
//				{
//				RGB[(k<<3) + 0] = CalculateR(Y00,V2Rtemp,V);		//R0
//				RGB[(k<<3) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
//				RGB[(k<<3) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
//				RGB[(k<<3) + 3] = 255;					//A
//				RGB[(k<<3) + 4] = CalculateR(Y01,V2Rtemp,V);		//R1
//				RGB[(k<<3) + 5] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
//				RGB[(k<<3) + 6] = CalculateB(Y01,U2Btemp,U);		//B0
//				RGB[(k<<3) + 7] = 255;					//A
//				}
//				else
//				{
					RGB[(k*6) + 0] = CalculateB(Y00,U2Btemp,U);		//B0
					RGB[(k*6) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
					RGB[(k*6) + 2] = CalculateR(Y00,V2Rtemp,V);		//R0
					RGB[(k*6) + 3] = CalculateB(Y01,U2Btemp,U);		//B0
					RGB[(k*6) + 4] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
					RGB[(k*6) + 5] = CalculateR(Y01,V2Rtemp,V);		//R1

	//			}
			}

			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(out_idx++,PackedPixels);
		}
		evenRow = evenRow ? false : true;
	}
//	if(height & 1)
//	{
//		for( i = 0; i < (width>>XF_BITSHIFT(NPC)); i++)
//		{
//#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
//			UVStream.read();
//		}
//	}
}
//KernNv212Yuv4
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_VU, int WORDWIDTH_DST, int TC, int iTC>
void KernNv212Yuv4_ro(xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _vu,xf::Mat<SRC_T, ROWS, COLS, NPC> & _u,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) VUbuf[16];
#pragma HLS ARRAY_PARTITION variable=VUbuf complete
	XF_SNAME(WORDWIDTH_DST) UPacked,VPacked; XF_SNAME(WORDWIDTH_VU) VUPacked;
	XF_SNAME(WORDWIDTH_DST) arr_UPacked[COLS>>(XF_BITSHIFT(NPC))],arr_VPacked[COLS>>(XF_BITSHIFT(NPC))];
	ap_uint<13> i,j;
	ap_uint<4> k;
	unsigned long long int idx=0,idx1=0;
	int l;
	rowloop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0;j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			VUPacked = _vu.read(idx1++);
			xfExtractPixels<NPC, WORDWIDTH_VU, XF_8UP>(VUbuf, VUPacked, 0);
#define AU_CVT_STEP 16
			for( k = 0, l = 0; k < (1<<(XF_BITSHIFT(NPC))); k+=2, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				UPacked.range(l+AU_CVT_STEP-1, l) = (VUbuf[k+1]) | ((ap_uint<16>)VUbuf[k+1] << (8));
				VPacked.range(l+AU_CVT_STEP-1, l) = (VUbuf[k]) | ((ap_uint<16>)VUbuf[k] << (8));
			}
			//_u.write(idx,UPacked);
			//_v.write(idx++,VPacked);
			_u.write(((i*2)*(_u.cols>>XF_BITSHIFT(NPC)))+j, UPacked);
			_v.write(((i*2)*(_v.cols>>XF_BITSHIFT(NPC)))+j, VPacked);
			arr_UPacked[j]=UPacked;
			arr_VPacked[j]=VPacked;
		}
		for( j = 0;j < (width>>XF_BITSHIFT(NPC)); j++)
		{
		_u.write((((i*2)+1)*(_u.cols>>XF_BITSHIFT(NPC)))+j, arr_UPacked[j]);
		_v.write((((i*2)+1)*(_v.cols>>XF_BITSHIFT(NPC)))+j, arr_VPacked[j]);
		}
	}

}

//KernYuyv2Rgba
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC,int PLANES, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernYuyv2Rgba_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & yuyv,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height, uint16_t width)
{
	ap_uint8_t RGB[64];
	XF_PTNAME(XF_8UP) YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	unsigned long long int idx=0;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = yuyv.read(i*width+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);
			for(int k = 0; k < (XF_NPIXPERCYCLE(NPC)>>1); k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
				//Y00 = (YUVbuf[(k<<2)] > 16) ? (YUVbuf[(k<<2)]-16) : 0;
				if(YUVbuf[(k<<2)] > 16)
				{
					Y00 = (YUVbuf[(k<<2)]-16);
				}
				else
				{
					Y00 = 0;
				}
				U =  YUVbuf[(k<<2)+1] - 128;

				//Y01 = (YUVbuf[(k<<2)+2] > 16) ? (YUVbuf[(k<<2)+2]-16) : 0;
				if(YUVbuf[(k<<2)+2] > 16)
				{
					Y01 = YUVbuf[(k<<2)+2]-16;
				}
				else
				{
					Y01 = 0;
				}
				V =  YUVbuf[(k<<2)+3] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;
				if(PLANES==4)
				{
				RGB[(k<<3)]   = CalculateR(Y00,V2Rtemp,V);		 //R0
				RGB[(k<<3)+1] = CalculateG(Y00,U2Gtemp,V2Gtemp); //G0
				RGB[(k<<3)+2] = CalculateB(Y00,U2Btemp,U);		 //B0
				RGB[(k<<3)+3] = 255;							 //A
				RGB[(k<<3)+4] = CalculateR(Y01,V2Rtemp,V);		 //R1
				RGB[(k<<3)+5] = CalculateG(Y01,U2Gtemp,V2Gtemp); //G1
				RGB[(k<<3)+6] = CalculateB(Y01,U2Btemp,U);		 //B0
				RGB[(k<<3)+7] = 255;	//A
				}
				else
				{
					RGB[(k*6)]   = CalculateR(Y00,V2Rtemp,V);		 //R0
					RGB[(k*6)+1] = CalculateG(Y00,U2Gtemp,V2Gtemp); //G0
					RGB[(k*6)+2] = CalculateB(Y00,U2Btemp,U);		 //B0
					RGB[(k*6)+3] = CalculateR(Y01,V2Rtemp,V);		 //R1
					RGB[(k*6)+4] = CalculateG(Y01,U2Gtemp,V2Gtemp); //G1
					RGB[(k*6)+5] = CalculateB(Y01,U2Btemp,U);		 //B0

				}
			}

			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(idx++,PackedPixels);
		}
	}
}

template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC,int PLANES, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernYuyv2bgr_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & yuyv,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height, uint16_t width)
{
	ap_uint8_t RGB[64];
	XF_PTNAME(XF_8UP) YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	unsigned long long int idx=0;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = yuyv.read(i*width+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);
			for(int k = 0; k < (XF_NPIXPERCYCLE(NPC)>>1); k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
				//Y00 = (YUVbuf[(k<<2)] > 16) ? (YUVbuf[(k<<2)]-16) : 0;
				if(YUVbuf[(k<<2)] > 16)
				{
					Y00 = (YUVbuf[(k<<2)]-16);
				}
				else
				{
					Y00 = 0;
				}
				U =  YUVbuf[(k<<2)+1] - 128;

				//Y01 = (YUVbuf[(k<<2)+2] > 16) ? (YUVbuf[(k<<2)+2]-16) : 0;
				if(YUVbuf[(k<<2)+2] > 16)
				{
					Y01 = YUVbuf[(k<<2)+2]-16;
				}
				else
				{
					Y01 = 0;
				}
				V =  YUVbuf[(k<<2)+3] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

					RGB[(k*6)]   = CalculateB(Y00,U2Btemp,U);		 //B0
					RGB[(k*6)+1] = CalculateG(Y00,U2Gtemp,V2Gtemp); //G0
					RGB[(k*6)+2] = CalculateR(Y00,V2Rtemp,V);		 //R0
					RGB[(k*6)+3] = CalculateB(Y01,U2Btemp,U);		 //B0
					RGB[(k*6)+4] = CalculateG(Y01,U2Gtemp,V2Gtemp); //G1
					RGB[(k*6)+5] = CalculateR(Y01,V2Rtemp,V);		 //R1
			}

			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(idx++,PackedPixels);
		}
	}
}


////KernYuyvNv12
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void KernYuyv2Nv12_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & _yuyv,xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & uv_plane, uint16_t height, uint16_t width)
{
	XF_PTNAME(XF_8UP) Ybuf[16], UVbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete
	XF_SNAME(WORDWIDTH_SRC) YUVPacked;
	XF_SNAME(WORDWIDTH_Y)  YPacked, UVPacked;
	unsigned long long idx=0,idx1=0;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = _yuyv.read(i*width+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);

			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{	// filling the Ybuf and UVbuf in the format required for NV12
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2)];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2)+2];
				if(evenRow)
				{
					UVbuf[(k<<1)] = YUVbuf[(k<<2)+1];
					UVbuf[(k<<1)+1] = YUVbuf[(k<<2)+3];
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Ybuf);
			y_plane.write(idx++,YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_UV>(UVbuf);
				uv_plane.write(idx1++,UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}
////KernYuyvNv21
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void KernYuyv2Nv21_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & _yuyv,xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & uv_plane, uint16_t height, uint16_t width)
{
	XF_PTNAME(XF_8UP) Ybuf[16], UVbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete
	XF_SNAME(WORDWIDTH_SRC) YUVPacked;
	XF_SNAME(WORDWIDTH_Y)  YPacked, UVPacked;
	unsigned long long idx=0,idx1=0;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = _yuyv.read(i*width+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);

			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{	// filling the Ybuf and UVbuf in the format required for NV12
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2)];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2)+2];
				if(evenRow)
				{
					UVbuf[(k<<1)+1] = YUVbuf[(k<<2)+1];
					UVbuf[(k<<1)] = YUVbuf[(k<<2)+3];
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Ybuf);
			y_plane.write(idx++,YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_UV>(UVbuf);
				uv_plane.write(idx1++,UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}
////KernYuyv2Iyuv
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernYuyv2Iyuv_ro(	xf::Mat<SRC_T, ROWS, COLS, NPC> & _yuyv, xf::Mat<DST_T, ROWS, COLS, NPC> & _y, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v, uint16_t height, uint16_t width)
{
	uint16_t i, j, k, l;
	ap_uint8_t Ybuf[16], Ubuf[16], Vbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf   complete
#pragma HLS ARRAY_PARTITION variable=Ubuf   complete
#pragma HLS ARRAY_PARTITION variable=Vbuf   complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete
unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	XF_SNAME(WORDWIDTH_DST) YPacked0, UPacked, VPacked;
	uint8_t offset;
	bool evenRow = true, evenBlock = true;
	offset = (1 << XF_BITSHIFT(NPC)) >> 1;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = _yuyv.read(i*width+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2)];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2)+2];
				if(evenRow)
				{
					if(evenBlock)
					{
						Ubuf[k] = YUVbuf[(k<<2)+1];
						Vbuf[k] = YUVbuf[(k<<2)+3];
					}
					else
					{
						Ubuf[k+offset] = YUVbuf[(k<<2)+1];
						Vbuf[k+offset] = YUVbuf[(k<<2)+3];
					}
				}
			}
			YPacked0 = PackPixels<WORDWIDTH_DST>(Ybuf);
			_y.write(idx++,YPacked0);
			if(evenRow & !evenBlock)
			{
				UPacked = PackPixels<WORDWIDTH_DST>(Ubuf);
				VPacked = PackPixels<WORDWIDTH_DST>(Vbuf);
				_u.write(idx1,UPacked);
				_v.write(idx1++,VPacked);
			}
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
}

//KernUyvy2Iyuv
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernUyvy2Iyuv_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & _uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & y_plane,xf::Mat<DST_T, ROWS/4, COLS, NPC> & u_plane,xf::Mat<DST_T, ROWS/4, COLS, NPC> & v_plane, uint16_t height, uint16_t width)
{

	ap_uint8_t Ybuf[16], Ubuf[16], Vbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=Ubuf complete
#pragma HLS ARRAY_PARTITION variable=Vbuf complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	XF_SNAME(WORDWIDTH_DST) YPacked0, UPacked, VPacked;
	uint8_t offset;
	unsigned long long int idx=0,idx1=0;
	bool evenRow = true, evenBlock = true;

	offset = (1<<XF_BITSHIFT(NPC))>>1;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = _uyvy.read(i*width+j);

			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2) + 1];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2) + 3];
				if(evenRow)
				{
					if(evenBlock)
					{
						Ubuf[k] = YUVbuf[(k<<2)];
						Vbuf[k] = YUVbuf[(k<<2)+2];
					}
					else
					{
						Ubuf[k + offset] = YUVbuf[(k<<2)];
						Vbuf[k + offset] = YUVbuf[(k<<2)+2];
					}
				}
			}
			YPacked0 = PackPixels<WORDWIDTH_DST>(Ybuf);
			y_plane.write(idx1++,YPacked0);
			if(evenRow & !evenBlock)
			{
				UPacked = PackPixels<WORDWIDTH_DST>(Ubuf);
				VPacked = PackPixels<WORDWIDTH_DST>(Vbuf);
				u_plane.write(idx,UPacked);
				v_plane.write(idx++,VPacked);
			}
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
}

////KernUyvy2Nv12
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void KernUyvy2Nv12_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & _uyvy,xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & uv_plane, uint16_t height, uint16_t width)
{
	ap_uint8_t Ybuf[16], UVbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete
	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	XF_SNAME(WORDWIDTH_Y) YPacked, UVPacked;
	unsigned long long int idx=0,idx1=0;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = _uyvy.read(i*width+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked,YUVbuf);
			// filling the Ybuf and UVbuf in the format required for NV12
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2)+1];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2)+3];
				if(evenRow)
				{
					UVbuf[(k<<1)] = YUVbuf[(k<<2)];
					UVbuf[(k<<1)+1] = YUVbuf[(k<<2)+2];
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Ybuf);
			y_plane.write(idx++,YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_Y>(UVbuf);
				uv_plane.write(idx1++,UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}
//KernUyvy2Nv21
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void KernUyvy2Nv21_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & _uyvy,xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & uv_plane, uint16_t height, uint16_t width)
{
	ap_uint8_t Ybuf[16], UVbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete
	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	XF_SNAME(WORDWIDTH_Y) YPacked, UVPacked;
	unsigned long long int idx=0,idx1=0;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = _uyvy.read(i*width+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked,YUVbuf);
			// filling the Ybuf and UVbuf in the format required for NV12
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2)+1];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2)+3];
				if(evenRow)
				{
					UVbuf[(k<<1)] = YUVbuf[(k<<2)+2];
					UVbuf[(k<<1)+1] = YUVbuf[(k<<2)];
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Ybuf);
			y_plane.write(idx++,YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_Y>(UVbuf);
				uv_plane.write(idx1++,UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}
////KernUyvy2Rgb_ro
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernUyvy2Rgb_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height,uint16_t width)
{
	uint16_t i,j,k;
	XF_PTNAME(XF_8UP) RGB[64],YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	XF_SNAME(WORDWIDTH_SRC) YUVPacked;
	uint8_t Y00, Y01;
	int32_t V2Rtemp,U2Gtemp,V2Gtemp,U2Btemp;
	int8_t U,V;
unsigned long long int idx=0,out_idx=0;
	rowloop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		columnloop:
		for(j = 0;j < width;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			YUVPacked = uyvy.read(idx++);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked,YUVbuf);
			for(k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				U = YUVbuf[(k<<2)] - 128;
				//Y00 = (YUVbuf[(k<<2) + 1] > 16) ? (YUVbuf[(k<<2) + 1] - 16):0;
				if(YUVbuf[(k<<2) + 1] > 16)
				{
					Y00 = (YUVbuf[(k<<2) + 1] - 16);
				}
				else
				{
					Y00 = 0;
				}
				V = YUVbuf[(k<<2) + 2] - 128;
				//Y01 = (YUVbuf[(k<<2) + 3] > 16) ? (YUVbuf[(k<<2) + 3] - 16):0;
				if((YUVbuf[(k<<2) + 3] > 16))
				{
					Y01 = (YUVbuf[(k<<2) + 3] - 16);
				}
				else
				{
					Y01 = 0;
				}

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				RGB[(k*6) ] = CalculateR(Y00,V2Rtemp,V);	//G0
				RGB[(k*6) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
				RGB[(k*6) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
				RGB[(k*6) + 3] = CalculateR(Y01,V2Rtemp,V);		//R1
				RGB[(k*6) + 4] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
				RGB[(k*6) + 5] = CalculateB(Y01,U2Btemp,U);		//B0
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(out_idx++,PackedPixels);
		}
	}
}
////KernUyvy2Rgb_ro
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernUyvy2bgr_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height,uint16_t width)
{
	uint16_t i,j,k;
	XF_PTNAME(XF_8UP) RGB[64],YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	XF_SNAME(WORDWIDTH_SRC) YUVPacked;
	uint8_t Y00, Y01;
	int32_t V2Rtemp,U2Gtemp,V2Gtemp,U2Btemp;
	int8_t U,V;
unsigned long long int idx=0,out_idx=0;
	rowloop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		columnloop:
		for(j = 0;j < width;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			YUVPacked = uyvy.read(idx++);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked,YUVbuf);
			for(k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				U = YUVbuf[(k<<2)] - 128;
				//Y00 = (YUVbuf[(k<<2) + 1] > 16) ? (YUVbuf[(k<<2) + 1] - 16):0;
				if(YUVbuf[(k<<2) + 1] > 16)
				{
					Y00 = (YUVbuf[(k<<2) + 1] - 16);
				}
				else
				{
					Y00 = 0;
				}
				V = YUVbuf[(k<<2) + 2] - 128;
				//Y01 = (YUVbuf[(k<<2) + 3] > 16) ? (YUVbuf[(k<<2) + 3] - 16):0;
				if((YUVbuf[(k<<2) + 3] > 16))
				{
					Y01 = (YUVbuf[(k<<2) + 3] - 16);
				}
				else
				{
					Y01 = 0;
				}

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				RGB[(k*6) ] = CalculateB(Y00,U2Btemp,U);		//B0
				RGB[(k*6) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
				RGB[(k*6) + 2] = CalculateR(Y00,V2Rtemp,V);	//G0
				RGB[(k*6) + 3] = CalculateB(Y01,U2Btemp,U);		//B0
				RGB[(k*6) + 4] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
				RGB[(k*6) + 5] = CalculateR(Y01,V2Rtemp,V);		//R1
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(out_idx++,PackedPixels);
		}
	}
}
////KernUyvy2Rgba_ro
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernUyvy2Rgba_ro(xf::Mat<SRC_T, ROWS, COLS, NPC> & uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & rgba,uint16_t height,uint16_t width)
{
	uint16_t i,j,k;
	XF_PTNAME(XF_8UP) RGB[64],YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	XF_SNAME(WORDWIDTH_SRC) YUVPacked;
	uint8_t Y00, Y01;
	int32_t V2Rtemp,U2Gtemp,V2Gtemp,U2Btemp;
	int8_t U,V;
unsigned long long int idx=0,out_idx=0;
	rowloop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		columnloop:
		for(j = 0;j < width;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			YUVPacked = uyvy.read(idx++);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked,YUVbuf);
			for(k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				U = YUVbuf[(k<<2)] - 128;
				//Y00 = (YUVbuf[(k<<2) + 1] > 16) ? (YUVbuf[(k<<2) + 1] - 16):0;
				if(YUVbuf[(k<<2) + 1] > 16)
				{
					Y00 = (YUVbuf[(k<<2) + 1] - 16);
				}
				else
				{
					Y00 = 0;
				}
				V = YUVbuf[(k<<2) + 2] - 128;
				//Y01 = (YUVbuf[(k<<2) + 3] > 16) ? (YUVbuf[(k<<2) + 3] - 16):0;
				if((YUVbuf[(k<<2) + 3] > 16))
				{
					Y01 = (YUVbuf[(k<<2) + 3] - 16);
				}
				else
				{
					Y01 = 0;
				}

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				RGB[(k<<3) ] = CalculateR(Y00,V2Rtemp,V);	//G0
				RGB[(k<<3) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
				RGB[(k<<3) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
				RGB[(k<<3) + 3] = 255;
				RGB[(k<<3) + 4] = CalculateR(Y01,V2Rtemp,V);		//R1
				RGB[(k<<3) + 5] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
				RGB[(k<<3) + 6] = CalculateB(Y01,U2Btemp,U);		//B0
				RGB[(k<<3) + 7] =255;
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(out_idx++,PackedPixels);
		}
	}
}

/********************************************************************************
 * Color Conversion APIs
 *******************************************************************************/

template<int SRC_T, int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFRgba2Yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{
		width=width>>XF_BITSHIFT(NPC);

		if(NPC==1)
		{
		KernRgba2Yuv4<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST>(_src, _y_image, _u_image, _v_image,height,width);
		}
		else
		{
		KernRgba2Yuv4_ro<SRC_T,DST_T,ROWS,COLS,NPC,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(_src, _y_image, _u_image, _v_image,height,width);
		}
}

#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
#pragma SDS data copy("_src.data"[0:"_src.size"])
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void rgba2yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC4)  && " RGBA image Type must be XF_8UC4");
	assert((DST_T  == XF_8UC1)  && " Y, U, V image Type must be XF_8UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGBA image rows and cols should be less than ROWS, COLS");
	assert(((_src.cols == _y_image.cols) && (_src.rows == _y_image.rows)) && "RGBA and Y plane dimensions mismatch");
	assert(((_src.cols == _u_image.cols) && (_src.rows == _u_image.rows)) && "RGBA and U plane dimensions mismatch");
	assert(((_src.cols == _v_image.cols) && (_src.rows == _v_image.rows)) && "RGBA and V plane dimensions mismatch");
	assert( ( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");

	xFRgba2Yuv4< SRC_T,DST_T,ROWS, COLS, NPC, XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src, _y_image, _u_image, _v_image, _src.rows,_src.cols);


}
////auRgba2Yuv4


/////////////////////////////////////////////////////////RGB2IYUV////////////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int ROWS_U,int ROWS_V>
void KernRgb2Iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _rgba, xf::Mat<DST_T, ROWS, COLS, NPC> & _y, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v,uint16_t height,uint16_t width)
{
	ap_uint<24> rgba;
	uint8_t y, u, v;
	bool evenRow = true, evenBlock = true;
unsigned long long int idx=0,idx1=0;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			rgba = _rgba.read(i*width+j);
			uint8_t r = rgba.range(7,0);
			uint8_t g = rgba.range(15,8);
			uint8_t b = rgba.range(23,16);

			y = CalculateY(r, g, b);
			if(evenRow)
			{
				if(evenBlock)
				{
					u = CalculateU(r, g, b);
					v = CalculateV(r, g, b);
				}
			}
			_y.write(idx++,y);
			if(evenRow & !evenBlock)
			{
				_u.write(idx1,u);
				_v.write(idx1++,v);
			}
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
}

template<int SRC_T, int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int ROWS_U,int ROWS_V>
void xFRgb2Iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
	if(NPC == XF_NPPC1)
	{

		KernRgb2Iyuv<SRC_T,DST_T,ROWS, COLS,  NPC, WORDWIDTH_SRC,  WORDWIDTH_DST, ROWS_U,ROWS_V>(_src, _y_image, _u_image, _v_image,height,width);

	}
	else
	{
		KernRgba2Iyuv_ro<SRC_T, DST_T,ROWS,COLS,NPC,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_DST,ROWS_U,ROWS_V,(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(_src, _y_image, _u_image, _v_image,height,width);

	}
}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC=0>
void rgb2iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC1)  && " Y, U, V image Type must be XF_8UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGB image rows and cols should be less than ROWS, COLS");
	assert(((_src.cols == _y_image.cols) && (_src.rows == _y_image.rows)) && "RGB and Y plane dimensions mismatch");
	assert(((_src.cols == _u_image.cols) && (_src.rows == (_u_image.rows<<2))) && "RGB and U plane dimensions mismatch");
	assert(((_src.cols == _v_image.cols) && (_src.rows == (_v_image.rows<<2))) && "RGB and V plane dimensions mismatch");
	assert( ( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");

	xFRgb2Iyuv< SRC_T,DST_T,ROWS, COLS, NPC, XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),  ROWS/4, ROWS/4>(_src,_y_image, _u_image, _v_image, _src.rows,_src.cols);
}

/////////////////////////////////////////////////////////end of RGB2IYUV/////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int ROWS_U,int ROWS_V>
void xFRgba2Iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{
width=width>>XF_BITSHIFT(NPC);

	if(NPC == XF_NPPC1)
	{
		KernRgba2Iyuv<SRC_T, DST_T,ROWS, COLS,  NPC, WORDWIDTH_SRC,  WORDWIDTH_DST, ROWS_U,ROWS_V>(_src, _y_image, _u_image, _v_image,height,width);


	}
	else
	{
		KernRgba2Iyuv_ro<SRC_T, DST_T,ROWS,COLS,NPC,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_DST,ROWS_U,ROWS_V,(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(_src, _y_image, _u_image, _v_image,height,width);

	}
}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC=0>
void rgba2iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC4)  && " RGBA image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC1)  && " Y, U, V image Type must be XF_8UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGBA image rows and cols should be less than ROWS, COLS");
	assert(((_src.cols == _y_image.cols) && (_src.rows == _y_image.rows)) && "RGBA and Y plane dimensions mismatch");
	assert(((_src.cols == _u_image.cols) && (_src.rows == (_u_image.rows<<2))) && "RGBA and U plane dimensions mismatch");
	assert(((_src.cols == _v_image.cols) && (_src.rows == (_v_image.rows<<2))) && "RGBA and V plane dimensions mismatch");
	assert( ( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");

	xFRgba2Iyuv<SRC_T,DST_T, ROWS, COLS, NPC, XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),  ROWS/4, ROWS/4>(_src, _y_image, _u_image, _v_image, _src.rows,_src.cols);

}
//auRgba2Iyuv


////auRgba2Nv12
//
template<int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFRgba2Nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,uint16_t height ,uint16_t width)
{


	width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
			KernRgba2Nv21<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV>(_src, _y, _uv,height,width);

		}
		else
		{
			KernRgba2Nv21_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,(COLS>>XF_BITSHIFT(NPC)),(1<<(XF_BITSHIFT(NPC)+1))>(_src, _y, _uv,height,width);

		}


}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
template <int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC=1,int NPC_UV=1>
void rgba2nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC4)  && " RGBA image Type must be XF_8UC3");
	assert((Y_T == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " VU image Type must be XF_8UC2");

	assert(((_src.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( _src.cols == _y.cols) && (_src.rows == _y.rows)) && "Y and RGBA plane dimensions mismatch");
	assert(((_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and VU planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the VU plane for multipixel parallelism  ");
		assert(( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}
	xFRgba2Nv21<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y,_uv,_src.rows,_src.cols);

}
template<int SRC_T, int Y_T, int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFRgba2Nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,uint16_t height,uint16_t width)
{

	width=width>>XF_BITSHIFT(NPC);
	if(NPC==1)
	{
		KernRgba2Nv12<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV>(_src, _y, _uv,height,width);
	}
	else
	{
		KernRgba2Nv12_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,(COLS>>XF_BITSHIFT(NPC)),(1<<(XF_BITSHIFT(NPC)+1))>(_src, _y, _uv,height,width);

	}
}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
template <int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC=1, int NPC_UV=1>
void rgba2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC4)  && " RGBA image Type must be XF_8UC3");
	assert((Y_T == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " UV image Type must be XF_8UC2");

	assert(((_src.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( _src.cols == _y.cols) && (_src.rows == _y.rows)) && "Y and RGBA plane dimensions mismatch");
	assert(((_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert(( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}
	xFRgba2Nv12<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y,_uv,_src.rows,_src.cols);

}
//auRgba2Nv21


template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFIyuv2Rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
	if((NPC == XF_NPPC8))
	{
		KernIyuv2Rgba_ro<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),(1<<(XF_BITSHIFT(NPC)+1))>(src_y, src_u, src_v, _dst0,height,width);
	}
	else
	{
		KernIyuv2Rgba<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src_y, src_u, src_v, _dst0,height,width);

	}

}

#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_u.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_v.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_u.data"[0:"src_u.size"])
#pragma SDS data copy("src_v.data"[0:"src_v.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC=1>
void iyuv2rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF

	assert((SRC_T == XF_8UC1)  && " Y, U, V images Type must be XF_8UC1");
	assert((DST_T == XF_8UC4)  && " RGBA image Type must be XF_8UC4");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert(( (src_y.cols == (_dst0.cols)) && (src_y.rows == _dst0.rows))      && "Y plane and RGBA dimensions mismatch");
	assert(( (src_u.cols == (_dst0.cols)) && (src_u.rows == (_dst0.rows>>2))) && "U plane and RGBA dimensions mismatch");
	assert(( (src_v.cols == (_dst0.cols)) && (src_v.rows == (_dst0.rows>>2))) && "V plane and RGBA dimensions mismatch");
	assert(( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");

	xFIyuv2Rgba<SRC_T,DST_T,ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src_y,src_u,src_v,_dst0,src_y.rows,src_y.cols);
}
//Iyuv2Rgba



template<int SRC_T,int ROWS, int COLS, int NPC, int WORDWIDTH>
void xFIyuv2Yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image, xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{


	if(NPC == XF_NPPC8)
	{
#pragma HLS DATAFLOW
		KernIyuv2Yuv4_ro<SRC_T,ROWS,COLS,NPC,WORDWIDTH,(ROWS<<1),((COLS>>XF_BITSHIFT(NPC))>>1),((1<<XF_BITSHIFT(NPC))>>1)>(src_u, src_v, _u_image, _v_image,height,width);
		write_y_ro<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH,(COLS>>XF_BITSHIFT(NPC))>(src_y,_y_image,height,width);
	}
	else if (NPC == XF_NPPC1)
	{
#pragma HLS DATAFLOW
		KernIyuv2Yuv4<SRC_T,ROWS,COLS,NPC,WORDWIDTH,(ROWS>>1),((COLS>>XF_BITSHIFT(NPC))>>1)>(src_u, src_v, _u_image, _v_image,height,width);
		write_y<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH,(COLS>>XF_BITSHIFT(NPC)),ROWS>(src_y,_y_image,height,width);
	}
}

#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_u.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_v.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_u.data"[0:"src_u.size"])
#pragma SDS data copy("src_v.data"[0:"src_v.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int ROWS, int COLS, int NPC=1 >
void iyuv2yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image, xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T == XF_8UC1)  && " Y, U, V images Type must be XF_8UC1");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert(( (src_y.cols == (_y_image.cols)) && (src_y.rows == _y_image.rows))      && "input and ouput Y planes dimensions mismatch");
	assert(( (src_u.cols == (_u_image.cols)) && (src_u.rows == (_u_image.rows>>2))) && "input and ouput U dimensions mismatch");
	assert(( (src_v.cols == (_v_image.cols)) && (src_v.rows == (_v_image.rows>>2))) && "input and ouput V dimensions mismatch");
	assert(( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");

	xFIyuv2Yuv4<SRC_T,ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC)>(src_y,src_u,src_v,_y_image,_u_image,_v_image,src_y.rows,src_y.cols);
}
////////////////////////////////////////////////IYUV2NV12////////////////////////////////////////////////////
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_UV>
void xFIyuv2Nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image,uint16_t height,uint16_t width)
{

if(NPC==XF_NPPC8)
{
#pragma HLS DATAFLOW
	KernIyuv2Nv12_ro<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_UV,(ROWS>>1),((COLS>>XF_BITSHIFT(NPC))>>1),((1<<XF_BITSHIFT(NPC))>>1)>
		(src_u,src_v,_uv_image,height,width);
	write_y_ro<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_SRC,(COLS>>XF_BITSHIFT(NPC))>(src_y,_y_image,height,width);
}
else
{
#pragma HLS DATAFLOW
	KernIyuv2Nv12<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_UV,(ROWS>>1),((COLS>>XF_BITSHIFT(NPC))>>1)>
		(src_u,src_v,_uv_image,height,width);

	write_y<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_SRC,(COLS>>XF_BITSHIFT(NPC)),(ROWS>>1)>(src_y,_y_image,height,width);
}
}

#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_u.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_v.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_u.data"[0:"src_u.size"])
#pragma SDS data copy("src_v.data"[0:"src_v.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_uv_image.data"[0:"_uv_image.size"])
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC=1 ,int NPC_UV=1>
void iyuv2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T == XF_8UC1)  && " Y, U, V images Type must be XF_8UC1");
	assert((UV_T  == XF_8UC2)  && " UV image Type must be XF_8UC2");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert(( (src_y.cols == (_y_image.cols)) && (src_y.rows == _y_image.rows))&& "input and ouput Y planes dimensions mismatch");
	assert(( (src_y.cols == (src_u.cols)) && (src_y.rows == (src_u.rows<<2))) && "Y and  U dimensions mismatch");
	assert(( (src_y.cols == (src_v.cols)) && (src_y.rows == (src_v.rows<<2))) && "Y and  V dimensions mismatch");
	assert(( (src_y.cols == (_uv_image.cols<<1)) && (src_y.rows == (_uv_image.rows<<1)))&& "input and ouput Y planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert( ( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}
	xFIyuv2Nv12<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(src_y,src_u,src_v,_y_image,_uv_image,src_y.rows,src_y.cols);
}
/////////////////////////////////////////////Iyuv2Nv12//////////////////////////////////////////////////////////////////
////auIyuv2Yuv4

template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFNv122Iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{
	if(NPC == XF_NPPC8)
	{
#pragma HLS DATAFLOW
		KernNv122Iyuv_ro<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>XF_BITSHIFT(NPC))>>1),((1<<XF_BITSHIFT(NPC))>>2)>(src_uv, _u_image, _v_image,height,width);
		write_y_ro<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src_y,_y_image,height,width);

	}
	else
	{
#pragma HLS DATAFLOW
		KernNv122Iyuv<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>XF_BITSHIFT(NPC))>>1)>(src_uv, _u_image, _v_image,height,width);
		write_y<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),(ROWS>>XF_BITSHIFT(NPC))>(src_y,_y_image,height,width);
	}
}
//Nv122Iyuv

#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF

	assert((SRC_T  == XF_8UC1)  && " Y,U,V image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " UV image Type must be XF_8UC2");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert(( (src_y.cols == (src_uv.cols<<1)) && (src_y.rows == (src_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");
	assert((( src_y.cols == _y_image.cols) && (src_y.rows == _y_image.rows)) && "Input and Outut Y planes dimensions mismatch");
	assert((( src_y.cols == _u_image.cols) && (src_y.rows == (_u_image.rows<<2))) && "U, Y planes dimensions mismatch");
	assert((( src_y.cols == _v_image.cols) && (src_y.rows == (_v_image.rows<<2))) && "V, Y planes dimensions mismatch");
	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert( ( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}
		xFNv122Iyuv<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(SRC_T,NPC)>(src_y,src_uv,_y_image,_u_image,_v_image,src_y.rows,src_y.cols);


}
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv122Rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
	if(NPC==1)
	{
		KernNv122Rgba<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_Y,WORDWIDTH_UV,WORDWIDTH_DST>(src_y, src_uv, _dst0,height,width);
	}
	else
	{
		KernNv122Rgba_ro<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(DST_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(src_y,src_uv,_dst0,height,width);

	}

}
//Nv122Rgba
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int UV_T,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " UV image Type must be XF_8UC2");
	assert((DST_T  == XF_8UC4) && " RGBA image Type must be XF_8UC4");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( src_y.cols == _dst0.cols) && (src_y.rows == _dst0.rows)) && "Y and RGBA Aplane dimensions mismatch");
	assert(( (src_y.cols == (src_uv.cols<<1)) && (src_y.rows == (src_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC values must be same  ");
	}
	xFNv122Rgba<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC)>(src_y,src_uv,_dst0,src_y.rows,src_y.cols);


}
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv122Yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{

//	assert(( (in_uv.cols == (u_out.cols)) && (in_uv.rows == (u_out.rows>>1)))
//			&& "UV plane and U plane dimensions mismatch");
//	assert(( (in_uv.cols == (v_out.cols)) && (in_uv.rows == (v_out.rows>>1)))
//			&& "UV plane and V plane dimensions mismatch");
	if(NPC == XF_NPPC8 )
	{
#pragma HLS DATAFLOW
		KernNv122Yuv4_ro<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),((1<<(XF_BITSHIFT(NPC)))>>1)>
		(src_uv, _u_image, _v_image,height,width);
		write_y_ro<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src_y,_y_image,height,width);
	}
	else
	{
#pragma HLS DATAFLOW
		KernNv122Yuv4<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>
		(src_uv, _u_image, _v_image,height,width);
		write_y<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),ROWS>(src_y,_y_image,height,width);


	}
}
//auNv122Yuv4


#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	xFNv122Yuv4<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(SRC_T,NPC)>(src_y,src_uv,_y_image,_u_image,_v_image,src_y.rows,src_y.cols);
}
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFNv212Iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{

	if(NPC == XF_NPPC8)
	{
#pragma HLS DATAFLOW
		KernNv212Iyuv_ro<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>XF_BITSHIFT(NPC))>>1),((1<<XF_BITSHIFT(NPC))>>2)>
		(src_uv,_u_image,_v_image,height,width);
		write_y_ro<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src_y,_y_image,height,width);

	}
	else
	{
#pragma HLS DATAFLOW
		KernNv212Iyuv<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>XF_BITSHIFT(NPC))>>1)>(src_uv,_u_image,_v_image,height,width);
		write_y<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),ROWS>(src_y,_y_image,height,width);

	}

}



//Nv212Iyuv

#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " Y,U,V image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " VU image Type must be XF_8UC2");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert(( (src_y.cols == (src_uv.cols<<1)) && (src_y.rows == (src_uv.rows<<1)))&& "Y and VU planes dimensions mismatch");
	assert((( src_y.cols == _y_image.cols) && (src_y.rows == _y_image.rows)) && "Input and Outut Y planes dimensions mismatch");
	assert((( src_y.cols == _u_image.cols) && (src_y.rows == (_u_image.rows<<2))) && "U, Y planes dimensions mismatch");
	assert((( src_y.cols == _v_image.cols) && (src_y.rows == (_v_image.rows<<2))) && "V, Y planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the VU plane for multipixel parallelism  ");
		assert( ( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}

		xFNv212Iyuv<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(SRC_T,NPC)>(src_y,src_uv,_y_image,_u_image,_v_image,src_y.rows,src_y.cols);
}

template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv212Rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
		KernNv212Rgba<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_Y,WORDWIDTH_UV,WORDWIDTH_DST>(src_y,src_uv,_dst0,height,width);
		}
		else
		{
		KernNv212Rgba_ro<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(DST_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(src_y, src_uv, _dst0,height,width);

		}

}
//Nv212Rgba

#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)

#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int UV_T,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " VU image Type must be XF_8UC2");
	assert((DST_T  == XF_8UC4) && " RGBA image Type must be XF_8UC4");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( src_y.cols == _dst0.cols) && (src_y.rows == _dst0.rows)) && "Y and RGBA Aplane dimensions mismatch");
	assert(( (src_y.cols == (src_uv.cols<<1)) && (src_y.rows == (src_uv.rows<<1)))&& "Y and VU planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the VU plane for multipixel parallelism  ");
		assert( ( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}
	xFNv212Rgba<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(DST_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC)>(src_y,src_uv,_dst0,src_y.rows,src_y.cols);
}

template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv212Yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{
	if(NPC == XF_NPPC8)
	{
#pragma HLS DATAFLOW
		KernNv212Yuv4_ro<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(src_uv, _u_image, _v_image,height,width);
		write_y_ro<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src_y,_y_image,height,width);

	}
	else
	{
#pragma HLS DATAFLOW
		KernNv212Yuv4<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src_uv, _u_image, _v_image,height,width);
		write_y<SRC_T,SRC_T,ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),ROWS>(src_y,_y_image,height,width);
	}

}
//auNv212Yuv4

#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  &&   "Y plane Type must be XF_8UC1");
	assert((UV_T   == XF_8UC2)  &&   "UV plane Type must be XF_8UC2");
	assert(( (src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(( (_y_image.cols == src_y.cols) && (_y_image.rows == src_y.rows))&& "Y  planes dimensions mismatch");
	assert(( (_u_image.cols == src_y.cols) && (_u_image.rows == src_y.rows))&& "Y and U planes dimensions mismatch");
	assert(( (_v_image.cols == src_y.cols) && (_v_image.rows == src_y.rows))&& "Y and V planes dimensions mismatch");
	assert(( ((src_uv.cols<<1) == src_y.cols) && ((src_uv.rows<<1) == src_y.rows))&& "Y and V planes dimensions mismatch");
	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert( ( (NPC==XF_NPPC1)||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC, NPC_UV values must be same  ");
	}

	xFNv212Yuv4<SRC_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(SRC_T,NPC)>(src_y,src_uv,_y_image,_u_image,_v_image,src_y.rows,src_y.cols);

}
template<int SRC_T,int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFUyvy2Iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & y_plane,xf::Mat<DST_T, ROWS/4, COLS, NPC> & u_plane,xf::Mat<DST_T, ROWS/4, COLS, NPC> & v_plane,uint16_t height, uint16_t width)
{
	width = width >> XF_BITSHIFT(NPC);
	if(NPC == XF_NPPC8)
	{

		KernUyvy2Iyuv_ro<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(uyvy, y_plane, u_plane, v_plane, height, width);
	}
	else
	{
		KernUyvy2Iyuv<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(uyvy, y_plane, u_plane, v_plane, height, width);
	}

}


#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void uyvy2iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image,xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image,xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_16UC1) && " UYVY plane Type must be XF_16UC1");
	assert((DST_T  == XF_8UC1)  &&  " Y, U, V planes Type must be XF_8UC1");
	assert(( (_src.rows <= ROWS ) && (_src.cols <= COLS)) && " UYVY image rows and cols should be less than ROWS, COLS");
	assert(( (_y_image.cols == _src.cols) && (_y_image.rows == _src.rows))&& "Y and UYVY planes dimensions mismatch");
	assert(( (_u_image.cols == _src.cols) && ((_u_image.rows<<2)== _src.rows))&& "U and UYVY planes dimensions mismatch");
	assert(( (_v_image.cols == _src.cols) && ((_v_image.rows<<2)== _src.rows))&& "U and UYVY planes dimensions mismatch");
	assert(((NPC==XF_NPPC1) ||(NPC==XF_NPPC8))&& " 1, 8 pixel parallelism is supported  ");

	xFUyvy2Iyuv<SRC_T,DST_T,ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_y_image,_u_image,_v_image,_src.rows,_src.cols);
}
//Uyvy2Nv12
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFUyvy2Nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & uyvy,xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & uv_plane,uint16_t height, uint16_t width)
{

/*	assert(( (uyvy.cols == (y_plane.cols<<1)) && (uyvy.rows == y_plane.rows))
			&& "UYVY and Y plane dimensions mismatch");
	assert(( (uyvy.cols == (uv_plane.cols<<1)) && (uyvy.rows == (uv_plane.rows<<1)))
			&& "UYVY and UV plane dimensions mismatch");*/

	width=width>>XF_BITSHIFT(NPC);


	if(NPC == XF_NPPC1)
	{
#pragma HLS DATAFLOW
		KernUyvy2Nv12<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC))> (uyvy, y_plane, uv_plane, height, width);
	}
	else
	{
		KernUyvy2Nv12_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<NPC)>>1)>(uyvy, y_plane, uv_plane, height, width);

	}
}

	#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
	#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
	#pragma SDS data access_pattern("_uv_image.data":SEQUENTIAL)
	#pragma SDS data copy("_src.data"[0:"_src.size"])
	#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
	#pragma SDS data copy("_uv_image.data"[0:"_uv_image.size"])
template<int SRC_T,int Y_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void uyvy2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<Y_T, ROWS, COLS, NPC> & _y_image,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_16UC1) && " UYVY plane Type must be XF_16UC1");
	assert((Y_T    == XF_8UC1)  &&  " Y plane Type must be XF_8UC1");
	assert((UV_T   == XF_8UC2)  &&  " UV image Type must be XF_8UC2");

	assert(( (_y_image.rows <= ROWS ) && (_y_image.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(( (_y_image.cols == (_uv_image.cols<<1)) && (_y_image.rows == (_uv_image.rows<<1)))&& "Y and UV planes dimensions mismatch");
	assert(( (_y_image.cols == _src.cols) && (_y_image.rows == _src.rows))&& "Y and UYVY planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");

	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC, NPC_UV values must be same  ");
	}
	xFUyvy2Nv12<SRC_T,Y_T,UV_T,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y_image,_uv_image,_src.rows,_src.cols);
}

///////////////////////////////////////////////////////Uyvy2Rgba///////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFUyvy2Rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst,uint16_t height, uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);

		if(NPC==1)
		{
		KernUyvy2Rgba<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);
		}
		else
		{
		KernUyvy2Rgba_ro<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),(1<<XF_BITSHIFT(NPC)>>1)>(_src, _dst, height, width);

		}

}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void uyvy2rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_16UC1) && " UYVY plane Type must be XF_16UC1");
	assert((DST_T  == XF_8UC4)  &&  " RGBA plane Type must be XF_8UC4");
	assert(( (_src.rows <= ROWS ) && (_src.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(( (_dst.cols == _src.cols) && (_dst.rows == _src.rows))&& "RGBA and UYVY planes dimensions mismatch");
	assert( ( (NPC==XF_NPPC1) ||(NPC==XF_NPPC8) ) && " 1,8 pixel parallelism is supported  ");

	xFUyvy2Rgba<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_dst,_src.rows,_src.cols);


}
//Yuyv2Iyuv
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFYuyv2Iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image, uint16_t height, uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);

	if(NPC == XF_NPPC8 )
	{

		KernYuyv2Iyuv_ro<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(_src, _y_image, _u_image, _v_image, height, width);
	}
	else
	{
		KernYuyv2Iyuv<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _y_image, _u_image, _v_image, height, width);
	}
}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])

template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC=1>
void yuyv2iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_16UC1) && " YUYV plane Type must be XF_16UC1");
	assert((DST_T  == XF_8UC1)  &&  " Y, U, V planes Type must be XF_8UC1");
	assert(( (_src.rows <= ROWS ) && (_src.cols <= COLS)) && " YUYV image rows and cols should be less than ROWS, COLS");
	assert(( (_y_image.cols == _src.cols) && (_y_image.rows == _src.rows))&& "Y and UYVY planes dimensions mismatch");
	assert(( (_u_image.cols == _src.cols) && ((_u_image.rows<<2)== _src.rows))&& "U and UYVY planes dimensions mismatch");
	assert(((NPC==XF_NPPC1) ||(NPC==XF_NPPC8))&& " 1, 8 pixel parallelism is supported  ");

	xFYuyv2Iyuv<SRC_T,DST_T,ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_y_image,_u_image,_v_image,_src.rows,_src.cols);
}

//Yuyv2Nv12
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFYuyv2Nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<Y_T, ROWS, COLS, NPC> & _y_image,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image,uint16_t height, uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
	if(NPC == XF_NPPC1)
	{

		KernYuyv2Nv12<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _y_image, _uv_image, height, width);
	}
	else
	{
		KernYuyv2Nv12_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(_src, _y_image, _uv_image, height, width);
	}
}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_uv_image.data"[0:"_uv_image.size"])
template<int SRC_T,int Y_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void yuyv2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<Y_T, ROWS, COLS, NPC> & _y_image,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_16UC1) && " YUYV plane Type must be XF_16UC1");
	assert((Y_T    == XF_8UC1)  &&  " Y plane Type must be XF_8UC1");
	assert((UV_T   == XF_8UC2)  &&  " UV image Type must be XF_8UC2");

	assert(( (_y_image.rows <= ROWS ) && (_y_image.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(( (_y_image.cols == (_uv_image.cols<<1)) && (_y_image.rows == (_uv_image.rows<<1)))&& "Y and UV planes dimensions mismatch");
	assert(( (_y_image.cols == _src.cols) && (_y_image.rows == _src.rows))&& "Y and YUYV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC, NPC_UV values must be same  ");
	}
	xFYuyv2Nv12<SRC_T,Y_T,UV_T,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y_image,_uv_image,_src.rows,_src.cols);
}
//Yuyv2Rgba
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFYuyv2Rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst,uint16_t height, uint16_t width)
{

	width=width>>XF_BITSHIFT(NPC);
	if(NPC==1)
	{
	KernYuyv2Rgba<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);
	}
	else
	{
	KernYuyv2Rgba_ro<SRC_T,DST_T,ROWS,COLS,NPC,XF_CHANNELS(DST_T,NPC),WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);

	}

}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void yuyv2rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  	 == XF_16UC1) &&  " YUYV plane Type must be XF_16UC1");
	assert((DST_T    == XF_8UC4)  &&  " RGBA plane Type must be XF_8UC4");

	assert(( (_src.rows <= ROWS ) && (_src.cols <= COLS)) && " YUYV image rows and cols should be less than ROWS, COLS");
	assert(( (_dst.cols == _src.cols) && (_dst.rows == _src.rows))&& "YUYV and RGBA planes dimensions mismatch");

	assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xFYuyv2Rgba<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_dst,_src.rows,_src.cols);

}
/////////////////////////////////////////////////////////RGB2INV12////////////////////////////////////////////////////////////////////////////
template<int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFRgb2Nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,uint16_t height,uint16_t width)
{

//	assert(( (rgba.cols == y_plane.cols) && (rgba.rows == y_plane.rows))
//			&& "RGBA and Y plane dimensions mismatch");
//	assert(( (rgba.cols == uv_plane.cols) && (rgba.rows == (uv_plane.rows<<1)))
//			&& "RGBA and UV plane dimensions mismatch");
	width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
			KernRgba2Nv12<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV>(_src, _y, _uv,height,width);

		}else
		{
			KernRgba2Nv12_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,(COLS>>XF_BITSHIFT(NPC)),(1<<(XF_BITSHIFT(NPC)+1))>(_src, _y, _uv,height,width);

		}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
template <int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC=1,int NPC_UV=1>
void rgb2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((Y_T == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " UV image Type must be XF_8UC2");

	assert(((_src.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( _src.cols == _y.cols) && (_src.rows == _y.rows)) && "Y and RGB plane dimensions mismatch");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}
	xFRgb2Nv12<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y,_uv,_src.rows,_src.cols);
}
/////////////////////////////////////////////////////////end of RGB2NV12/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////RGB2NV21//////////////////////////////////////////////////////////////////////
//template<int SRC_T, int Y_T, int UV_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_VU>
//void KernRgb2Nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _rgba, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC> & _vu,uint16_t height,uint16_t width)
//{
//	width=width>>XF_BITSHIFT(NPC);
//	XF_SNAME(XF_32UW) rgba;
//	unsigned long long int idx=0,idx1=0;
//	uint8_t y, u, v;
//	bool evenRow = true, evenBlock = true;
//
//	RowLoop:
//	for(int i = 0; i < height; i++)
//	{
//#pragma HLS LOOP_FLATTEN off
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		ColLoop:
//		for(int j = 0; j < width; j++)
//		{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
//			rgba = _rgba.read(i*width+j);
//			uint8_t r = rgba.range(7,0);
//			uint8_t g = rgba.range(15,8);
//			uint8_t b = rgba.range(23,16);
//
//			y = CalculateY(r, g, b);
//			if(evenRow)
//			{
//				u = CalculateU(r, g, b);
//				v = CalculateV(r, g, b);
//			}
//			_y.write(idx++,y);
//			if(evenRow)
//			{
//				if((j & 0x01)==0)
//					_vu.write(idx1++,v | ((uint16_t)u << 8));
//			}
//		}
//		evenRow = evenRow ? false : true;
//	}
//}
template<int SRC_T, int Y_T, int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFRgb2Nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,uint16_t height ,uint16_t width)
{


	width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
			KernRgba2Nv21<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV>(_src, _y, _uv,height,width);
		}
		else
		{
			KernRgba2Nv21_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,(COLS>>XF_BITSHIFT(NPC)),(1<<(XF_BITSHIFT(NPC)+1))>(_src, _y, _uv,height,width);

		}


}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
template <int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC=1,int NPC_UV=1>
void rgb2nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((Y_T == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " UV image Type must be XF_8UC2");

	assert(((_src.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( _src.cols == _y.cols) && (_src.rows == _y.rows)) && "Y and RGB plane dimensions mismatch");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC values must be same  ");
	}
	xFRgb2Nv21<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y,_uv,_src.rows,_src.cols);
}
////////////////////////////////////////////////////////end of RGB2NV21////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////RGB2YUV4///////////////////////////////////////////////////////////////////////
template<int SRC_T, int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void KernRgb2Yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & _rgba, xf::Mat<DST_T, ROWS, COLS, NPC> & _y, xf::Mat<DST_T, ROWS, COLS, NPC> & _u, xf::Mat<DST_T, ROWS, COLS, NPC> & _v,uint16_t height,uint16_t width )
{
	XF_SNAME(XF_32UW) rgba;
	uint8_t y, u, v;
unsigned long long int idx=0;
	RowLoop:
	for(int i = 0; i < height; ++i)
	{
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; ++j)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
#pragma HLS PIPELINE
			rgba = _rgba.read(i*width+j);

			y = CalculateY(rgba.range(7,0),rgba.range(15,8), rgba.range(23,16));
			u = CalculateU(rgba.range(7,0),rgba.range(15,8), rgba.range(23,16));
			v = CalculateV(rgba.range(7,0),rgba.range(15,8), rgba.range(23,16));

			_y.write(idx,y);
			_u.write(idx,u);
			_v.write(idx++,v);
		}
	}
}
template<int SRC_T, int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFRgb2Yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _v_image,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (rgba.cols == y_plane.cols) && (rgba.rows == y_plane.rows))
//			&& "RGBA and Y plane dimensions mismatch");
//	assert(( (rgba.cols == u_plane.cols) && (rgba.rows == u_plane.rows))
//			&& "RGBA and U plane dimensions mismatch");
//	assert(( (rgba.cols == v_plane.cols) && (rgba.rows == v_plane.rows))
//			&& "RGBA and V plane dimensions mismatch");

		width=width>>(XF_BITSHIFT(NPC));
		if(NPC==1)
		{
		KernRgb2Yuv4< SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST>(_src, _y_image, _u_image, _v_image,height,width);
		}
		else
		{
		KernRgba2Yuv4_ro<SRC_T,DST_T,ROWS,COLS,NPC,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(_src, _y_image, _u_image, _v_image,height,width);

		}
}
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
#pragma SDS data copy("_src.data"[0:"_src.size"])
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void rgb2yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	xFRgb2Yuv4< SRC_T,DST_T,ROWS, COLS, NPC, XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src, _y_image, _u_image, _v_image, _src.rows,_src.cols);
}
////////////////////////////////////////////////////////end of RGB2YUV4////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////UYVY2RGB///////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernUyvy2Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba, uint16_t height, uint16_t width)
{
	XF_SNAME(WORDWIDTH_DST) rgba;

	XF_SNAME(WORDWIDTH_SRC) uyvy;

	XF_SNAME(WORDWIDTH_SRC) uy;
	XF_SNAME(WORDWIDTH_SRC) vy;

	unsigned long long int idx=0;
	XF_PTNAME(XF_8UP)  r, g, b;
	int8_t y1, y2, u, v;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ColLoop:
		for(int j = 0; j < width; j+=2)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

			//uyvy = _uyvy.read();

			uy = _uyvy.read(i*width+j);
			vy = _uyvy.read(i*width+j+1);

			u = (uint8_t)uy.range(7,0) - 128;

/*			if(uyvy.range(15,8) > 16)
				y1 = (uint8_t)uyvy.range(15,8) - 16;
			else
				y1 = 0;*/

			y1 = (uy.range(15,8) > 16) ? ((uint8_t)uy.range(15,8) - 16) : 0;

			v = (uint8_t)vy.range(7,0) - 128;

/*			if(uyvy.range(31,24) > 16)
				y2 = ((uint8_t)uyvy.range(31,24) - 16);
			else
				y2 = 0;*/
			y2 = (vy.range(15,8) > 16) ? ((uint8_t)vy.range(15,8) - 16) : 0;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			r = CalculateR(y1, V2Rtemp, v);
			g = CalculateG(y1, U2Gtemp, V2Gtemp);
			b = CalculateB(y1, U2Btemp, u);

			rgba = ((ap_uint24_t)r) | ((ap_uint24_t)g << 8) | ((ap_uint24_t)b << 16) ;
			_rgba.write(idx,rgba);
			idx++;
			r = CalculateR(y2, V2Rtemp, v);
			g = CalculateG(y2, U2Gtemp, V2Gtemp);
			b = CalculateB(y2, U2Btemp, u);

			rgba = ((ap_uint24_t)r) | ((ap_uint24_t)g << 8) | ((ap_uint24_t)b << 16) ;
			_rgba.write(idx,rgba);
			idx++;
		}
	}
}

template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFUyvy2Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst,uint16_t height, uint16_t width)
{
/*	assert(( (uyvy.cols == (rgba.cols<<1)) && (uyvy.rows == rgba.rows))
			&& "UYVY and RGBA plane dimensions mismatch");*/
	width=width>>XF_BITSHIFT(NPC);

		if(NPC==1)
		{
		KernUyvy2Rgb<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);
		}
		else
		{
		KernUyvy2Rgb_ro<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);

		}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void uyvy2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  	 == XF_16UC1) &&  " UYVY plane Type must be XF_16UC1");
	assert((DST_T    == XF_8UC3)  &&  " RGB plane Type must be XF_8UC3");

	assert(( (_src.rows <= ROWS ) && (_src.cols <= COLS)) && " UYVY image rows and cols should be less than ROWS, COLS");
	assert(( (_dst.cols == _src.cols) && (_dst.rows == _src.rows))&& "UYVY and RGB planes dimensions mismatch");

	assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");


	xFUyvy2Rgb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_dst,_src.rows,_src.cols);


}
////////////////////////////////////////////////////////end of UYVY2RGB////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////YUYV2RGB//////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernYuyv2Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _yuyv,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba,uint16_t height, uint16_t width)
{
	XF_SNAME(WORDWIDTH_DST) rgba;
	XF_SNAME(WORDWIDTH_SRC) yu,yv;
	XF_PTNAME(XF_8UP)  r, g, b;
	int8_t y1, y2, u, v;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	unsigned long long int idx=0;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ColLoop:
		for(int j = 0; j < width ; j += 2)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

			yu = _yuyv.read(i*width+j);
			yv = _yuyv.read(i*width+j+1);
			u = (uint8_t)yu.range(15,8) - 128;
			y1 = (yu.range(7,0) > 16) ? ((uint8_t)yu.range(7,0) - 16) : 0;

			v = (uint8_t)yv.range(15,8) - 128;
			y2 = (yv.range(7,0) > 16) ? ((uint8_t)yv.range(7,0) - 16) : 0;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			r = CalculateR(y1, V2Rtemp, v);
			g = CalculateG(y1, U2Gtemp, V2Gtemp);
			b = CalculateB(y1, U2Btemp, u);

			rgba = ((ap_uint24_t)r) | ((ap_uint24_t)g << 8) | ((ap_uint24_t)b << 16 );
			_rgba.write(idx++,rgba);

			r = CalculateR(y2, V2Rtemp, v);
			g = CalculateG(y2, U2Gtemp, V2Gtemp);
			b = CalculateB(y2, U2Btemp, u);

			rgba = ((ap_uint24_t)r) | ((ap_uint24_t)g << 8) | ((ap_uint24_t)b << 16);
			_rgba.write(idx++,rgba);
		}
	}
}

//Yuyv2Rgba
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFYuyv2Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst,uint16_t height, uint16_t width)
{

	width=width>>XF_BITSHIFT(NPC);
	if(NPC==1)
	{
		KernYuyv2Rgb<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);
	}
	else
	{
		KernYuyv2Rgba_ro<SRC_T,DST_T,ROWS,COLS,NPC,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);

	}

}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void yuyv2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  	 == XF_16UC1) &&  " YUYV plane Type must be XF_16UC1");
	assert((DST_T    == XF_8UC3)  &&  " RGB plane Type must be XF_8UC3");

	assert(( (_src.rows <= ROWS ) && (_src.cols <= COLS)) && " YUYV image rows and cols should be less than ROWS, COLS");
	assert(( (_dst.cols == _src.cols) && (_dst.rows == _src.rows))&& "YUYV and RGB planes dimensions mismatch");
	assert( ( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8) ) && " 1,2,4,8 pixel parallelism is supported  ");

	xFYuyv2Rgb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_dst,_src.rows,_src.cols);
}

////////////////////////////////////////////////////////end of YUYV2RGB////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////IYUV2RGB///////////////////////////////////////////////////////////////////////
template< int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernIyuv2Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba,uint16_t height,uint16_t width)
{
	ap_uint<13> i,j;
	hls::stream<XF_SNAME(WORDWIDTH_SRC)> uStream, vStream;
#pragma HLS STREAM variable=&uStream  depth=COLS
#pragma HLS STREAM variable=&vStream  depth=COLS

	XF_SNAME(WORDWIDTH_SRC)  yPacked, uPacked, vPacked;
	XF_SNAME(WORDWIDTH_DST) rgba;
unsigned long long int idx=0,idx1=0;

	uint8_t y1, y2;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t u, v;
	bool evenRow = true, evenBlock = true;
	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			yPacked = _y.read(i*width+j);
			//dummy1 =  dst1.read();
			//dummy2 = dst2.read();

			if(evenBlock)
			{
				if(evenRow)
				{
					uPacked = _u.read(idx);
					uStream.write(uPacked);
					vPacked = _v.read(idx++);
					vStream.write(vPacked);
				}
				else
				{
					/* Copy of the U and V values are pushed into stream to be used for next row */
					uPacked = uStream.read();
					vPacked = vStream.read();
				}
			}

			y1 = (uint8_t)yPacked.range(7,0) > 16 ? (uint8_t)yPacked.range(7,0)-16 : 0;

			u = (uint8_t)uPacked.range(7,0) - 128;
			v = (uint8_t)vPacked.range(7,0) - 128;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
			// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
			// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
			rgba.range(7,0)   = CalculateR(y1, V2Rtemp, v);		  //R
			rgba.range(15,8)  = CalculateG(y1, U2Gtemp, V2Gtemp); //G
			rgba.range(23,16) = CalculateB(y1, U2Btemp, u);		  //B

			_rgba.write(idx1++,rgba);
			evenBlock = evenBlock ? false : true;
		}

		evenRow = evenRow ? false: true;
	}
}
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFIyuv2Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (in_y.cols == (rgba.cols)) && (in_y.rows == rgba.rows))
//			&& "Y plane and RGBA dimensions mismatch");
//	assert(( (in_u.cols == (rgba.cols)) && (in_u.rows == (rgba.rows>>2)))
//			&& "U plane and RGBA dimensions mismatch");
//	assert(( (in_v.cols == (rgba.cols)) && (in_v.rows == (rgba.rows>>2)))
//			&& "V plane and RGBA dimensions mismatch");
	width=width>>XF_BITSHIFT(NPC);
//if((NPC == XF_NPPC8))// && (FLAG == AU_STANDALONE))
//	{
////#pragma HLS INLINE
////		hls::stream< ap_uint<256> > tmp;
////#pragma HLS DATAFLOW
////		KernIyuv2Rgba<ROWS,COLS,NPC,WORDWIDTH_SRC,AU_256UW,(COLS>>NPC),(1<<(NPC+1))>
////		(in_y, in_u, in_v, tmp,height,width);
//
//		Convert256To64<ROWS,COLS,NPC,AU_256UW,WORDWIDTH_DST,(COLS>>NPC)>(tmp, rgba,height,width);
//
//	}
//else
//{
	KernIyuv2Rgb<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src_y, src_u, src_v, _dst0,height,width);

//}

}
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_u.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_v.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_u.data"[0:"src_u.size"])
#pragma SDS data copy("src_v.data"[0:"src_v.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC=1>
void iyuv2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " Y,U,V planes Type must be XF_8UC1");
	assert((DST_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(((src_y.cols == _dst0.cols) && (src_y.rows == _dst0.rows)) && "Y and RGB plane dimensions mismatch");
	assert(((src_y.cols == src_u.cols) && (src_y.rows == (src_u.rows<<2)))&& "Y and U planes dimensions mismatch");
	assert(((src_y.cols == src_v.cols) && (src_y.rows == (src_v.rows<<2)))&& "Y and U planes dimensions mismatch");

	assert(((NPC==XF_NPPC1) ||(NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xFIyuv2Rgb<SRC_T,DST_T,ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src_y,src_u,src_v,_dst0,src_y.rows,src_y.cols);

}
////////////////////////////////////////////////////////end of IYUV2RGB////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// NV122bgr////////////////////////////////////////////////////////////////
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void KernNv122bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba,uint16_t height,uint16_t width)
{
	unsigned long long int idx=0,idx1=0;
	hls::stream<XF_SNAME(WORDWIDTH_UV)> uvStream;
#pragma HLS STREAM variable=&uvStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_UV) uvPacked;
	XF_SNAME(WORDWIDTH_DST) rgba;
	uint8_t y1, y2;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t u, v;
	bool evenRow = true, evenBlock = true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			yPacked = _y.read(i*width+j);
			if(evenRow)
			{
				if(evenBlock)
				{
					uvPacked = _uv.read(idx++);
					uvStream.write(uvPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					uvPacked = uvStream.read();
				}
			}
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UVbuf, UVPacked, 0);
			uint8_t t = yPacked.range(7,0);
			y1 = t > 16 ? t - 16 : 0;
			v = (uint8_t)uvPacked.range(15,8) - 128;
			u = (uint8_t)uvPacked.range(7,0) - 128;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
			// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
			// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
			rgba.range(23,16)   = CalculateR(y1, V2Rtemp, v);		  //R
			rgba.range(15,8)  = CalculateG(y1, U2Gtemp, V2Gtemp); //G
			rgba.range(7,0) = CalculateB(y1, U2Btemp, u);		  //B

			//			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			_rgba.write(idx1++,rgba);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for(int i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			uvStream.read();
		}
	}
}
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv122bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
	if(NPC==1)
	{
		KernNv122bgr<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_Y,WORDWIDTH_UV,WORDWIDTH_DST>(src_y, src_uv, _dst0,height,width);
	}
	else
	{
		KernNv122bgr_ro<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(DST_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(src_y, src_uv, _dst0,height,width);
	}

}
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)

#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int UV_T,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " VU image Type must be XF_8UC2");
	assert((DST_T  == XF_8UC3) && " BGR image Type must be XF_8UC3");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert((( src_y.cols == _dst0.cols) && (src_y.rows == _dst0.rows)) && "Y and BGR plane dimensions mismatch");
	assert(( (src_y.cols == (src_uv.cols<<1)) && (src_y.rows == (src_uv.rows<<1)))&& "Y and VU planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the VU plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}

	xFNv122bgr<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC)>(src_y,src_uv,_dst0,src_y.rows,src_y.cols);
}
///////////////////////////////////////////////////////end of NV122BGR////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////// NV122RGB////////////////////////////////////////////////////////////////
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void KernNv122Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba,uint16_t height,uint16_t width)
{
	unsigned long long int idx=0,idx1=0;
	hls::stream<XF_SNAME(WORDWIDTH_UV)> uvStream;
#pragma HLS STREAM variable=&uvStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_UV) uvPacked;
	XF_SNAME(WORDWIDTH_DST) rgba;
	uint8_t y1, y2;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t u, v;
	bool evenRow = true, evenBlock = true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			yPacked = _y.read(i*width+j);
			if(evenRow)
			{
				if(evenBlock)
				{
					uvPacked = _uv.read(idx++);
					uvStream.write(uvPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					uvPacked = uvStream.read();
				}
			}
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UVbuf, UVPacked, 0);
			uint8_t t = yPacked.range(7,0);
			y1 = t > 16 ? t - 16 : 0;
			v = (uint8_t)uvPacked.range(15,8) - 128;
			u = (uint8_t)uvPacked.range(7,0) - 128;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
			// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
			// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
			rgba.range(7,0)   = CalculateR(y1, V2Rtemp, v);		  //R
			rgba.range(15,8)  = CalculateG(y1, U2Gtemp, V2Gtemp); //G
			rgba.range(23,16) = CalculateB(y1, U2Btemp, u);		  //B

			//			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			_rgba.write(idx1++,rgba);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for(int i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			uvStream.read();
		}
	}
}
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv122Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0,uint16_t height,uint16_t width)
{

	width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
		KernNv122Rgb<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_Y,WORDWIDTH_UV,WORDWIDTH_DST>(src_y, src_uv, _dst0,height,width);
		}
		else
		{
		KernNv122Rgba_ro<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(DST_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(src_y, src_uv, _dst0,height,width);

		}

}
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)

#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int UV_T,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " UV image Type must be XF_8UC2");
	assert((DST_T  == XF_8UC3) && " RGB image Type must be XF_8UC3");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( src_y.cols == _dst0.cols) && (src_y.rows == _dst0.rows)) && "Y and RGB plane dimensions mismatch");
	assert(( (src_y.cols == (src_uv.cols<<1)) && (src_y.rows == (src_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}

	xFNv122Rgb<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC)>(src_y,src_uv,_dst0,src_y.rows,src_y.cols);
}
///////////////////////////////////////////////////////end of NV122RGB////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////NV212RGB////////////////////////////////////////////////////////////////
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y, int WORDWIDTH_VU, int WORDWIDTH_DST>
void KernNv212Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _vu,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba,uint16_t height,uint16_t width)
{
	hls::stream<XF_SNAME(WORDWIDTH_VU)> vuStream;
#pragma HLS STREAM variable=&vuStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_VU) vuPacked;
	unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_DST) rgba;
	ap_uint<13> i,j;
	uint8_t y1, y2;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t u, v;
	bool evenRow = true, evenBlock = true;
	RowLoop:
	for( i = 0; i < (height); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			yPacked = _y.read(i*width+j);
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				if(evenBlock)
				{
					vuPacked = _vu.read(idx++);
					vuStream.write(vuPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					vuPacked = vuStream.read();
				}
			}
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UVbuf, UVPacked, 0);
			uint8_t t = yPacked.range(7,0);
			y1 = t > 16 ? t - 16 : 0;
			u = (uint8_t)vuPacked.range(15,8) - 128;
			v = (uint8_t)vuPacked.range(7,0) - 128;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
			// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
			// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
			rgba.range(7,0)   = CalculateR(y1, V2Rtemp, v);		  //R
			rgba.range(15,8)  = CalculateG(y1, U2Gtemp, V2Gtemp); //G
			rgba.range(23,16) = CalculateB(y1, U2Btemp, u);		  //B

			//			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			_rgba.write(idx1++,rgba);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for( i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			vuStream.read();
		}
	}
}
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv212Rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
			KernNv212Rgb<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_Y,WORDWIDTH_UV,WORDWIDTH_DST>(src_y, src_uv, _dst0,height,width);
		}
		else
		{
			KernNv212Rgba_ro<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(DST_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(src_y, src_uv, _dst0,height,width);
		}

}
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int UV_T,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " vu image Type must be XF_8UC2");
	assert((DST_T  == XF_8UC3) && " RGB image Type must be XF_8UC3");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( src_y.cols == _dst0.cols) && (src_y.rows == _dst0.rows)) && "Y and RGB plane dimensions mismatch");
	assert(( (src_y.cols == (src_uv.cols<<1)) && (src_y.rows == (src_uv.rows<<1)))&& "Y and VU planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC values must be same  ");
	}


	xFNv212Rgb<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src_y,src_uv,_dst0,src_y.rows,src_y.cols);
}
///////////////////////////////////////////////////////end of NV122RGB////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////NV212BGR////////////////////////////////////////////////////////////////
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC, int NPC_UV, int PLANES, int WORDWIDTH_Y, int WORDWIDTH_VU, int WORDWIDTH_DST>
void KernNv212bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _vu,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba,uint16_t height,uint16_t width)
{
	hls::stream<XF_SNAME(WORDWIDTH_VU)> vuStream;
#pragma HLS STREAM variable=&vuStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_VU) vuPacked;
	unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_DST) rgba;
	ap_uint<13> i,j;
	uint8_t y1, y2;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t u, v;
	bool evenRow = true, evenBlock = true;
	RowLoop:
	for( i = 0; i < (height); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			yPacked = _y.read(i*width+j);
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				if(evenBlock)
				{
					vuPacked = _vu.read(idx++);
					vuStream.write(vuPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					vuPacked = vuStream.read();
				}
			}
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UVbuf, UVPacked, 0);
			uint8_t t = yPacked.range(7,0);
			y1 = t > 16 ? t - 16 : 0;
			u = (uint8_t)vuPacked.range(15,8) - 128;
			v = (uint8_t)vuPacked.range(7,0) - 128;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
			// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
			// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
			rgba.range(23,16)   = CalculateR(y1, V2Rtemp, v);		  //R
			rgba.range(15,8)  = CalculateG(y1, U2Gtemp, V2Gtemp); //G
			rgba.range(7,0) = CalculateB(y1, U2Btemp, u);		  //B

			//			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			_rgba.write(idx1++,rgba);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for( i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			vuStream.read();
		}
	}
}
template<int SRC_T,int UV_T,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv212bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0,uint16_t height,uint16_t width)
{


	width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
		KernNv212bgr<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(DST_T,NPC),WORDWIDTH_Y,WORDWIDTH_UV,WORDWIDTH_DST>(src_y, src_uv, _dst0,height,width);
		}
		else
		{
			KernNv212bgr_ro<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(DST_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(src_y, src_uv, _dst0,height,width);

		}

}
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)

#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int UV_T,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " VU image Type must be XF_8UC2");
	assert((DST_T  == XF_8UC3) && " BGR image Type must be XF_8UC3");
	assert(((src_y.rows <= ROWS ) && (src_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert((( src_y.cols == _dst0.cols) && (src_y.rows == _dst0.rows)) && "Y and BGR plane dimensions mismatch");
	assert(( (src_y.cols == (src_uv.cols<<1)) && (src_y.rows == (src_uv.rows<<1)))&& "Y and VU planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the VU plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}

	xFNv212bgr<SRC_T,UV_T,DST_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(DST_T,NPC)>(src_y,src_uv,_dst0,src_y.rows,src_y.cols);
}
///////////////////////////////////////////////////////end of NV122RGB////////////////////////////////////////////////////////////////

/////////////////////////////////	RGB2GRAY	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xfrgb2gray(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) RGB[XF_CHANNELS(SRC_T,NPC)*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=RGB complete

	XF_TNAME(SRC_T,NPC)  RGB_packed;//=0;
	XF_CTUNAME(DST_T,NPC)  GRAY[XF_NPIXPERCYCLE(NPC)];//=0;
	XF_TNAME(DST_T,NPC)  Gray_packed;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS


			RGB_packed=src.read(i*(width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(RGB_packed, RGB);
			for(ap_uint<13> k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
#pragma HLS UNROLL
				GRAY[k]=CalculateGRAY(RGB[offset], RGB[offset+1] , RGB[offset+2]);
				Gray_packed.range((k*XF_DTPIXELDEPTH(DST_T,NPC)+(XF_DTPIXELDEPTH(DST_T,NPC)-1)), k*XF_DTPIXELDEPTH(DST_T,NPC))=GRAY[k];
			}
				dst.write((i*(width>>XF_BITSHIFT(NPC)))+j, Gray_packed);
		}
	}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void rgb2gray(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC1)  && " GRAY image Type must be XF_8UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGB image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and GRAY plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfrgb2gray<SRC_T, DST_T, ROWS, COLS, NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(DST_T,NPC), (ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))> (_src, _dst, _src.rows, _src.cols);
}

/////////////////////////////////	BGR2GRAY	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfbgr2gray(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) RGB[XF_CHANNELS(SRC_T,NPC)*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=RGB complete

	XF_TNAME(SRC_T,NPC)  RGB_packed;//=0;
	XF_CTUNAME(DST_T,NPC)  GRAY[XF_NPIXPERCYCLE(NPC)];//=0;
	XF_TNAME(DST_T,NPC)  Gray_packed;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS


			RGB_packed=src.read(i*(width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(RGB_packed, RGB);
			for(ap_uint<13> k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
#pragma HLS UNROLL
				GRAY[k]=CalculateGRAY( RGB[offset+2], RGB[offset+1] ,RGB[offset]);
				Gray_packed.range((k*XF_DTPIXELDEPTH(DST_T,NPC)+(XF_DTPIXELDEPTH(DST_T,NPC)-1)), k*XF_DTPIXELDEPTH(DST_T,NPC))=GRAY[k];
			}
				dst.write((i*(width>>XF_BITSHIFT(NPC)))+j, Gray_packed);
		}
	}
}


#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void bgr2gray(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC1)  && " GRAY image Type must be XF_8UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " BGR image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and GRAY plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");


	xfbgr2gray<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}

//////////////////////////////////////	GRAY2RGB	////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfgray2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{

	XF_DTUNAME(DST_T,NPC)  RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(DST_T,NPC) RGB_packed;
	XF_TNAME(SRC_T,NPC) GRAY_packed;
	XF_TNAME(SRC_T,NPC)  GRAY[XF_NPIXPERCYCLE(NPC)];
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:

		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			GRAY_packed = src.read(i*(width>>XF_BITSHIFT(NPC))+j);

			for(int k=0;k<XF_NPIXPERCYCLE(NPC);k++)
				{

#pragma HLS UNROLL
					GRAY[k]=GRAY_packed.range(k*(XF_PIXELWIDTH(SRC_T,NPC))+XF_PIXELWIDTH(SRC_T,NPC)-1, k*XF_PIXELWIDTH(SRC_T,NPC) );
					RGB[k].range(7,0) = GRAY[k];
					RGB[k].range(15,8) = GRAY[k];
					RGB[k].range(23,16) = GRAY[k];
					RGB_packed.range(k*(XF_PIXELWIDTH(DST_T,NPC))+XF_PIXELWIDTH(DST_T,NPC)-1, k*XF_PIXELWIDTH(DST_T,NPC) )=RGB[k];
				}

			dst.write(i*(width>>XF_BITSHIFT(NPC))+j, RGB_packed);
		}
	}

}


#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void gray2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " GRAY image Type must be XF_8UC1");
	assert((DST_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " GRAY image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and GRAY plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfgray2rgb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
//////////////////////////////////////	GRAY2BGR	////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfgray2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_DTUNAME(DST_T,NPC)  RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(DST_T,NPC) RGB_packed;
	XF_TNAME(SRC_T,NPC) GRAY_packed;
	XF_TNAME(SRC_T,NPC)  GRAY[XF_NPIXPERCYCLE(NPC)];
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:

		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			GRAY_packed = src.read(i*(width>>XF_BITSHIFT(NPC))+j);

			for(int k=0;k<XF_NPIXPERCYCLE(NPC);k++)
				{

#pragma HLS UNROLL
					GRAY[k]=GRAY_packed.range(k*(XF_PIXELWIDTH(SRC_T,NPC))+XF_PIXELWIDTH(SRC_T,NPC)-1, k*XF_PIXELWIDTH(SRC_T,NPC) );
					RGB[k].range(7,0) = GRAY[k];
					RGB[k].range(15,8) = GRAY[k];
					RGB[k].range(23,16) = GRAY[k];
					RGB_packed.range(k*(XF_PIXELWIDTH(DST_T,NPC))+XF_PIXELWIDTH(DST_T,NPC)-1, k*XF_PIXELWIDTH(DST_T,NPC) )=RGB[k];
				}

			dst.write(i*(width>>XF_BITSHIFT(NPC))+j, RGB_packed);
		}
	}

}


#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void gray2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC1)  && " GRAY image Type must be XF_8UC1");
	assert((DST_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " GRAY image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and GRAY plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfgray2bgr<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	RGB2XYZ	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfrgb2xyz(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	ap_uint<8> RGB[3];
#pragma HLS ARRAY_PARTITION variable=RGB complete

	XF_TNAME(SRC_T,NPC)  RGB_packed=0;
	XF_TNAME(DST_T,NPC)  XYZ_packed=0;
	XF_DTUNAME(DST_T,NPC)  XYZ[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(DST_T,NPC)	X,Y,Z;
	short int depth=XF_PIXELWIDTH(DST_T,NPC)/XF_CHANNELS(SRC_T,NPC);
	int k=0;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < width>>XF_BITSHIFT(NPC); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			RGB_packed = src.read((i*(width>>XF_BITSHIFT(NPC)))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(RGB_packed, RGB);

			for(int k=0,offset=0;k< XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
#pragma HLS UNROLL
				X=Calculate_X(RGB[offset], RGB[offset+1] , RGB[offset+2]);
				Y=Calculate_Y(RGB[offset], RGB[offset+1] , RGB[offset+2]);
				Z=Calculate_Z(RGB[offset], RGB[offset+1] , RGB[offset+2]);

				XYZ[k].range((XF_DTPIXELDEPTH(DST_T,NPC)-1),0)=X;
				XYZ[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*2)-1,XF_DTPIXELDEPTH(DST_T,NPC))=Y;
				XYZ[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*3)-1,XF_DTPIXELDEPTH(DST_T,NPC)*2)=Z;
				XYZ_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=XYZ[k];
			}

			dst.write((i*(width>>XF_BITSHIFT(NPC)))+j,XYZ_packed);

		}
	}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void rgb2xyz(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " XYZ image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGB image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and XYZ plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfrgb2xyz<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	BGR2XYZ	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfbgr2xyz(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	ap_uint<8> RGB[3];
#pragma HLS ARRAY_PARTITION variable=RGB complete

	XF_TNAME(SRC_T,NPC)  RGB_packed=0;
	XF_TNAME(DST_T,NPC)  XYZ_packed=0;
	XF_DTUNAME(DST_T,NPC)  XYZ[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(DST_T,NPC)	X,Y,Z;
	short int depth=XF_PIXELWIDTH(DST_T,NPC)/XF_CHANNELS(SRC_T,NPC);
	int k=0;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < width>>XF_BITSHIFT(NPC); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			RGB_packed = src.read((i*(width>>XF_BITSHIFT(NPC)))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(RGB_packed, RGB);

			for(int k=0,offset=0;k< XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
#pragma HLS UNROLL
				X=Calculate_X(RGB[offset+2], RGB[offset+1],RGB[offset]);
				Y=Calculate_Y(RGB[offset+2], RGB[offset+1] , RGB[offset]);
				Z=Calculate_Z( RGB[offset+2], RGB[offset+1],RGB[offset]);

				XYZ[k].range((XF_DTPIXELDEPTH(DST_T,NPC)-1),0)=X;
				XYZ[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*2)-1,XF_DTPIXELDEPTH(DST_T,NPC))=Y;
				XYZ[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*3)-1,XF_DTPIXELDEPTH(DST_T,NPC)*2)=Z;
				XYZ_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=XYZ[k];
			}

			dst.write((i*(width>>XF_BITSHIFT(NPC)))+j,XYZ_packed);

		}
	}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void bgr2xyz(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " XYZ image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " BGR image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and XYZ plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfbgr2xyz<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	XYZ2RGB		/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfxyz2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) XYZ[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=XYZ complete

	XF_TNAME(DST_T,NPC)  RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(DST_T,NPC)  XYZ_packed=0,RGB_packed=0;
	XF_TNAME(DST_T,NPC)	R, G, B;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			XYZ_packed = src.read((i*(width>>XF_BITSHIFT(NPC)))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(XYZ_packed, XYZ);


			for(int k=0,offset=0;k< XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
#pragma HLS UNROLL
			R=Calculate_R(XYZ[offset], XYZ[offset+1] , XYZ[offset+2]);
			G=Calculate_G(XYZ[offset], XYZ[offset+1] , XYZ[offset+2]);
			B=Calculate_B(XYZ[offset], XYZ[offset+1] , XYZ[offset+2]);

			RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)-1),0)=R;
			RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*2)-1,XF_DTPIXELDEPTH(DST_T,NPC))=G;
			RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*3)-1,XF_DTPIXELDEPTH(DST_T,NPC)*2)=B;
			RGB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=RGB[k];
			}
			dst.write((i*(width>>XF_BITSHIFT(NPC)))+j,RGB_packed);

		}
	}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void xyz2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " XYZ image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " XYZ image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and XYZ plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfxyz2rgb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	XYZ2BGR		/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfxyz2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) XYZ[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=XYZ complete

	XF_TNAME(DST_T,NPC)  RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(DST_T,NPC)  XYZ_packed=0,RGB_packed=0;
	XF_TNAME(DST_T,NPC)	R, G, B;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			XYZ_packed = src.read((i*(width>>XF_BITSHIFT(NPC)))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(XYZ_packed, XYZ);


			for(int k=0,offset=0;k< XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
#pragma HLS UNROLL
			R=Calculate_R(XYZ[offset], XYZ[offset+1] , XYZ[offset+2]);
			G=Calculate_G(XYZ[offset], XYZ[offset+1] , XYZ[offset+2]);
			B=Calculate_B(XYZ[offset], XYZ[offset+1] , XYZ[offset+2]);

			RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)-1),0)=B;
			RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*2)-1,XF_DTPIXELDEPTH(DST_T,NPC))=G;
			RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*3)-1,XF_DTPIXELDEPTH(DST_T,NPC)*2)=R;
			RGB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=RGB[k];
			}
			dst.write((i*(width>>XF_BITSHIFT(NPC)))+j,RGB_packed);

		}
	}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void xyz2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " XYZ image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " XYZ image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and XYZ plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfxyz2bgr<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}

/////////////////////////////////	RGB2YCRCB	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfrgb2ycrcb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) RGB[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=RGB complete
	XF_DTUNAME(DST_T,NPC) YCRCB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(SRC_T,NPC)  RGB_packed=0;

	XF_TNAME(DST_T,NPC) YCRCB_packed=0;
	XF_TNAME(DST_T,NPC)	Y,CR,CB;

	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < width>>XF_BITSHIFT(NPC); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS


			RGB_packed = src.read((i*width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(RGB_packed, RGB);
			for(int k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
				Y=CalculateGRAY(RGB[offset], RGB[offset+1] , RGB[offset+2]);
				CR=Calculate_CR(RGB[offset], Y );
				CB=Calculate_CB(RGB[offset+2], Y);

				YCRCB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)-1),0)=Y;
				YCRCB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*2)-1,XF_DTPIXELDEPTH(DST_T,NPC))=CR;
				YCRCB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*3)-1,XF_DTPIXELDEPTH(DST_T,NPC)*2)=CB;
				YCRCB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=YCRCB[k];

			}

			dst.write((i*width>>XF_BITSHIFT(NPC))+j,YCRCB_packed);

		}
	}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void rgb2ycrcb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " YCrCb image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGB image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and YCrCb plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfrgb2ycrcb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	BGR2YCRCB	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfbgr2ycrcb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) RGB[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=RGB complete
	XF_DTUNAME(DST_T,NPC) YCRCB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(SRC_T,NPC)  RGB_packed=0;

	XF_TNAME(DST_T,NPC) YCRCB_packed=0;
	XF_TNAME(DST_T,NPC)	Y,CR,CB;

	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS


			RGB_packed = src.read((i*(width>>XF_BITSHIFT(NPC)))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(RGB_packed, RGB);
			for(int k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
				Y=CalculateGRAY(RGB[offset+2], RGB[offset+1] , RGB[offset]);
				CR=Calculate_CR(RGB[offset+2], Y );
				CB=Calculate_CB(RGB[offset], Y);

				YCRCB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)-1),0)=Y;
				YCRCB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*2)-1,XF_DTPIXELDEPTH(DST_T,NPC))=CR;
				YCRCB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*3)-1,XF_DTPIXELDEPTH(DST_T,NPC)*2)=CB;
				YCRCB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=YCRCB[k];

			}

			dst.write((i*width>>XF_BITSHIFT(NPC))+j,YCRCB_packed);

		}
	}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void bgr2ycrcb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " YCrCb image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " BGR image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and YCrCb plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfbgr2ycrcb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	YCRCB2RGB	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfycrcb2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) YCRCB[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=YCRCB complete

	XF_TNAME(SRC_T,NPC)  YCRCB_packed=0;
	XF_TNAME(DST_T,NPC) RGB_packed=0;
	XF_TNAME(DST_T,NPC)  RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(DST_T,NPC)	Y,R,B,G;

	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			YCRCB_packed = src.read((i*(width>>XF_BITSHIFT(NPC)))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YCRCB_packed, YCRCB);

			for(int k=0,offset=0;k< XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
				R=Calculate_Ycrcb2R( YCRCB[offset], YCRCB[offset+1]);
				G=Calculate_Ycrcb2G(YCRCB[offset], YCRCB[offset+1],YCRCB[offset+2]);
				B=Calculate_Ycrcb2B( YCRCB[offset], YCRCB[offset+2]);
				RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)-1),0)=R;
				RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*2)-1,XF_DTPIXELDEPTH(DST_T,NPC))=G;
				RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*3)-1,XF_DTPIXELDEPTH(DST_T,NPC)*2)=B;
				RGB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=RGB[k];
			}
			dst.write(i*(width>>XF_BITSHIFT(NPC))+j,RGB_packed);
		}
	}



}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void ycrcb2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " YCrCb image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " YCrCb image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and YCrCb plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfycrcb2rgb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	YCRCB2BGRs	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfycrcb2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) YCRCB[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=YCRCB complete

	XF_TNAME(SRC_T,NPC)  YCRCB_packed=0;
	XF_TNAME(DST_T,NPC) RGB_packed=0;
	XF_TNAME(DST_T,NPC)  RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(DST_T,NPC)	Y,R,B,G;

	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			YCRCB_packed = src.read((i*(width>>XF_BITSHIFT(NPC)))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(YCRCB_packed, YCRCB);

			for(int k=0,offset=0;k< XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
				R=Calculate_Ycrcb2R( YCRCB[offset], YCRCB[offset+1]);
				G=Calculate_Ycrcb2G(YCRCB[offset], YCRCB[offset+1],YCRCB[offset+2]);
				B=Calculate_Ycrcb2B( YCRCB[offset], YCRCB[offset+2]);
				RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)-1),0)=B;
				RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*2)-1,XF_DTPIXELDEPTH(DST_T,NPC))=G;
				RGB[k].range((XF_DTPIXELDEPTH(DST_T,NPC)*3)-1,XF_DTPIXELDEPTH(DST_T,NPC)*2)=R;
				RGB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=RGB[k];
			}
			dst.write(i*(width>>XF_BITSHIFT(NPC))+j,RGB_packed);
		}
	}



}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void ycrcb2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " YCrCb image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " YCrCb image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and YCrCb plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfycrcb2bgr<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}

/////////////////////////////////	RGB2HLS	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfrgb2hls(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) RGB[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=RGB complete

	XF_TNAME(SRC_T,NPC)  RGB_packed=0;
	XF_TNAME(DST_T,NPC)  HSV_packed=0;
	XF_TNAME(DST_T,NPC)  HSV[XF_NPIXPERCYCLE(NPC)];
	XF_CTUNAME(SRC_T,NPC) r,g,b;
	XF_CTUNAME(SRC_T,NPC) Vmax,Vmin;
//	int Vmax=0,Vmin=0;
	int consta;
	int sub;
	int two_L=0;
	int inv_sub=0;int less_if=0;
	short int depth=XF_PIXELWIDTH(DST_T,NPC)/XF_CHANNELS(SRC_T,NPC);
	int inv_add=0;
	int S=0;
	int k=0;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			RGB_packed = src.read((i*width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(RGB_packed, RGB);
			for(int k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
				 r=RGB[offset],g=RGB[offset+1],b=RGB[offset+2];
				 Vmax=b;	Vmin=b;

				if((g>r) &&(g>b))
				{
					Vmax=g;
				}
				else if((r>b))
				{
					Vmax=r;
				}
				if((g<r) &&(g<b))
				{
					Vmin=g;
				}
				else if((r<b))
				{
					Vmin=r;
				}


				short int v_add=(Vmax+Vmin);
				short int v_sub=(Vmax-Vmin);
				two_L=(Vmax+Vmin);
				int h=0;
				if(two_L < 255)
				{
					  inv_add = ((255 << 12)/(v_add));
					  less_if = (v_sub*inv_add+(1 << (11))) >> 12;
					  S=less_if;
				}
				else
				{
					if(Vmax==Vmin)
					{
						S=0;

					}
					else
					{
						 int inv_sub = ((255 << 12)/((2*255)-v_add));
						 int less_if = (v_sub*inv_sub +(1 << (11))) >> 12;
						 S=less_if;

					}

				}
				sub=(Vmax==b) ? (r-g) : (Vmax==g) ? (b-r) : (g-b);
				consta=(Vmax==b) ? 240 : (Vmax==g) ? 120 : 0;
				if(Vmax==Vmin)
				{
					h=0;

				}
				else
				{
					 inv_sub = ((1 << 15)/(v_sub));
					h= consta+((60*sub*inv_sub) >> 15);

					if(h < 0) {
					  h+=360;
					}
				}

					HSV[k].range(7, 0) = (h>>1);
					HSV[k].range(15, 8) = (unsigned char)((two_L+1)>>1);
					HSV[k].range(23, 16) = S;
					HSV_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=HSV[k];
			}


			dst.write(i*(width>>XF_BITSHIFT(NPC))+j,HSV_packed);
		}
	}


}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void rgb2hls(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " HLS image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGB image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and HLS plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfrgb2hls<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	BGR2HLS	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfbgr2hls(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) RGB[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=RGB complete

	XF_TNAME(SRC_T,NPC)  RGB_packed=0;
	XF_TNAME(DST_T,NPC)  HSV_packed=0;
	XF_TNAME(DST_T,NPC)  HSV[XF_NPIXPERCYCLE(NPC)];
	XF_CTUNAME(SRC_T,NPC) r,g,b;
	XF_CTUNAME(SRC_T,NPC) Vmax,Vmin;
//	int Vmax=0,Vmin=0;
	int consta;
	int sub;
	int two_L=0;
	int inv_sub=0;int less_if=0;
	short int depth=XF_PIXELWIDTH(DST_T,NPC)/XF_CHANNELS(SRC_T,NPC);
	int inv_add=0;
	int S=0;
	int k=0;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			RGB_packed = src.read((i*width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(RGB_packed, RGB);
			for(int k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
				 b=RGB[offset],g=RGB[offset+1],r=RGB[offset+2];
				 Vmax=b;	Vmin=b;

				if((g>r) &&(g>b))
				{
					Vmax=g;
				}
				else if((r>b))
				{
					Vmax=r;
				}
				if((g<r) &&(g<b))
				{
					Vmin=g;
				}
				else if((r<b))
				{
					Vmin=r;
				}


				short int v_add=(Vmax+Vmin);
				short int v_sub=(Vmax-Vmin);
				two_L=(Vmax+Vmin);
				int h=0;
				if(two_L < 255)
				{
					  inv_add = ((255 << 12)/(v_add));
					  less_if = (v_sub*inv_add+(1 << (11))) >> 12;
					  S=less_if;
				}
				else
				{
					if(Vmax==Vmin)
					{
						S=0;

					}
					else
					{
						 int inv_sub = ((255 << 12)/((2*255)-v_add));
						 int less_if = (v_sub*inv_sub +(1 << (11))) >> 12;
						 S=less_if;

					}

				}
				sub=(Vmax==b) ? (r-g) : (Vmax==g) ? (b-r) : (g-b);
				consta=(Vmax==b) ? 240 : (Vmax==g) ? 120 : 0;
				if(Vmax==Vmin)
				{
					h=0;

				}
				else
				{
					 inv_sub = ((1 << 15)/(v_sub));
					h= consta+((60*sub*inv_sub) >> 15);

					if(h < 0) {
					  h+=360;
					}
				}

					HSV[k].range(7, 0) = (h>>1);
					HSV[k].range(15, 8) = (unsigned char)((two_L+1)>>1);
					HSV[k].range(23, 16) = S;
					HSV_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=HSV[k];
			}


			dst.write(i*(width>>XF_BITSHIFT(NPC))+j,HSV_packed);
		}
	}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void bgr2hls(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " HLS image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " BGR image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and HLS plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfbgr2hls<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	HLS2RGB	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfhls2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) HLS[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=HLS complete
	XF_DTUNAME(SRC_T,NPC) RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(SRC_T,NPC)  HLS_packed=0;
	XF_TNAME(DST_T,NPC)  RGB_packed=0;
	short int depth=XF_PIXELWIDTH(DST_T,NPC)/XF_CHANNELS(SRC_T,NPC);
	unsigned long int r=0;
	unsigned long int g=0;
	unsigned long int b=0;
	XF_CTUNAME(SRC_T,NPC) H,L,S;
    ap_fixed<28,9> tab[4];
    ap_fixed<28,9> p1, p2;
	ap_ufixed<20,1,AP_RND> hscale=0.0333333333333333333;
	ap_ufixed<20,1,AP_RND> s_scale=0.0039215686274509803921568627451f;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			HLS_packed = src.read((i*width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(HLS_packed, HLS);

			for(int k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
				 H=HLS[offset],L=HLS[offset+1],S=HLS[offset+2];

		            if( S == 0 )
		                b = g = r = L;
		            else
		            {
		                static const int sector_data[][3]=
		                {{1,3,0}, {1,0,2}, {3,0,1}, {0,2,1}, {0,1,3}, {2,1,0}};

		                ap_fixed<28,9> mul_scl=s_scale*S;

		                if(2*L <= 255)
		                {
		                	 p2= L+L* mul_scl;
		                }
		                else
		                {
		                	 p2= L + S -(( L*mul_scl));
		                }

		                 p1 = 2*L - p2;

		                unsigned char H_scl = (unsigned char)H* hscale;
		                ap_fixed<28,9> h_fix=H*hscale - H_scl;
		            	if(H_scl >= 6)//for hrange=180, 0<H<255, then 0<h_i<8
		            		H_scl -= 6;

		            	tab[0] = p2;
		                tab[1] = p1;
		                tab[2] = p2 - (p2 - p1)*(h_fix);
		                tab[3] = p1 + (p2 - p1)*(h_fix);

		                b = (tab[sector_data[H_scl][0]]);
		                g = (tab[sector_data[H_scl][1]]);
		                r = (tab[sector_data[H_scl][2]]);
		            }
				RGB[k].range(7, 0) = (unsigned char)(r);
				RGB[k].range(15, 8) = (unsigned char)(g);
				RGB[k].range(23, 16) = (unsigned char)(b);
				RGB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=RGB[k];
			}
			dst.write(i*(width>>XF_BITSHIFT(NPC))+j,RGB_packed);
		}
	}


}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void hls2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " HLS image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " HLS image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and HLS plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfhls2rgb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	HLS2BGR	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfhls2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) HLS[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=HLS complete
	XF_DTUNAME(SRC_T,NPC) RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(SRC_T,NPC)  HLS_packed=0;
	XF_TNAME(DST_T,NPC)  RGB_packed=0;
	short int depth=XF_PIXELWIDTH(DST_T,NPC)/XF_CHANNELS(SRC_T,NPC);
	unsigned long int r=0;
	unsigned long int g=0;
	unsigned long int b=0;
	XF_CTUNAME(SRC_T,NPC) H,L,S;
    ap_fixed<28,9> tab[4];
    ap_fixed<28,9> p1, p2;
	ap_ufixed<20,1,AP_RND> hscale=0.0333333333333333333;
	ap_ufixed<20,1,AP_RND> s_scale=0.0039215686274509803921568627451f;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			HLS_packed = src.read((i*width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(HLS_packed, HLS);

			for(int k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{
				 H=HLS[offset],L=HLS[offset+1],S=HLS[offset+2];

		            if( S == 0 )
		                b = g = r = L;
		            else
		            {
		                static const int sector_data[][3]=
		                {{1,3,0}, {1,0,2}, {3,0,1}, {0,2,1}, {0,1,3}, {2,1,0}};

		                ap_fixed<28,9> mul_scl=s_scale*S;

		                if(2*L <= 255)
		                {
		                	 p2= L+L* mul_scl;
		                }
		                else
		                {
		                	 p2= L + S -(( L*mul_scl));
		                }

		                 p1 = 2*L - p2;

		                unsigned char H_scl = (unsigned char)H* hscale;
		                ap_fixed<28,9> h_fix=H*hscale - H_scl;
		            	if(H_scl >= 6)//for hrange=180, 0<H<255, then 0<h_i<8
		            		H_scl -= 6;

		            	tab[0] = p2;
		                tab[1] = p1;
		                tab[2] = p2 - (p2 - p1)*(h_fix);
		                tab[3] = p1 + (p2 - p1)*(h_fix);

		                b = (tab[sector_data[H_scl][0]]);
		                g = (tab[sector_data[H_scl][1]]);
		                r = (tab[sector_data[H_scl][2]]);
		            }
				RGB[k].range(7, 0) = (unsigned char)(b);
				RGB[k].range(15, 8) = (unsigned char)(g);
				RGB[k].range(23, 16) = (unsigned char)(r);
				RGB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=RGB[k];
			}
			dst.write(i*(width>>XF_BITSHIFT(NPC))+j,RGB_packed);
		}
	}


}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void hls2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " HLS image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " HLS image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and HLS plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfhls2bgr<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	HSV2RGB	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfhsv2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) HSV[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=HSV complete
	XF_DTUNAME(DST_T,NPC) RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(SRC_T,NPC)  HSV_packed=0;
	XF_TNAME(DST_T,NPC)  RGB_packed=0;
	XF_CTUNAME(SRC_T,NPC) H, S, V;
	unsigned long int r=0;
	unsigned long int g=0;
	unsigned long int b=0;
    ap_fixed<28,9> tab[4];
    ap_fixed<28,9> p1, p2;
	ap_ufixed<20,1,AP_RND> hscale=0.0333333333333333333;
	ap_ufixed<20,1,AP_RND> s_scale=0.0039215686274509803921568627451;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < width>>XF_BITSHIFT(NPC); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			HSV_packed = src.read((i*width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(HSV_packed, HSV);

			for(int k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{

					H=HSV[offset],S=HSV[offset+1],V=HSV[offset+2];


	                static const int sector_data[][3]=
	                {{1,3,0}, {1,0,2}, {3,0,1}, {0,2,1}, {0,1,3}, {2,1,0}};

	                ap_fixed<28,9> mul_scl=s_scale*S;



	                unsigned char H_scl = (unsigned char)H* hscale;
	                ap_fixed<28,9> h_fix=H*hscale - H_scl;
	            	if(H_scl >= 6)//for hrange=180, 0<H<255, then 0<h_i<8
	            		H_scl -= 6;

	            	tab[0] = V;
	                tab[1] = V*(1-mul_scl);
	                tab[2] = V*(1-mul_scl*h_fix);
	                tab[3] = V*(1-mul_scl+mul_scl*h_fix);

	                b = (tab[sector_data[H_scl][0]]);

	                g = (tab[sector_data[H_scl][1]]);
	                r = (tab[sector_data[H_scl][2]]);
			RGB[k].range(7, 0) = (unsigned char)(r);
			RGB[k].range(15, 8) = (unsigned char)(g);
			RGB[k].range(23, 16) = (unsigned char)(b);
			RGB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=RGB[k];

			}

			dst.write(i*(width>>XF_BITSHIFT(NPC))+j,RGB_packed);
		}
	}


}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void hsv2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " HSV image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " HSV image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and HSV plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfhsv2rgb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
/////////////////////////////////	HSV2BGR	/////////////////////////////////////

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void xfhsv2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
	XF_CTUNAME(SRC_T,NPC) HSV[3*XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=HSV complete
	XF_DTUNAME(DST_T,NPC) RGB[XF_NPIXPERCYCLE(NPC)];
	XF_TNAME(SRC_T,NPC)  HSV_packed=0;
	XF_TNAME(DST_T,NPC)  RGB_packed=0;
	XF_CTUNAME(SRC_T,NPC) H, S, V;
	unsigned long int r=0;
	unsigned long int g=0;
	unsigned long int b=0;
    ap_fixed<28,9> tab[4];
    ap_fixed<28,9> p1, p2;
	ap_ufixed<20,1,AP_RND> hscale=0.0333333333333333333;
	ap_ufixed<20,1,AP_RND> s_scale=0.0039215686274509803921568627451;
	rowloop:
	for(ap_uint<13> i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(ap_uint<13> j = 0; j < width>>XF_BITSHIFT(NPC); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			HSV_packed = src.read((i*width>>XF_BITSHIFT(NPC))+j);
			ExtractUYVYPixels<WORDWIDTH_SRC>(HSV_packed, HSV);

			for(int k=0,offset=0;k<XF_NPIXPERCYCLE(NPC);k++,offset+=3)
			{

					H=HSV[offset],S=HSV[offset+1],V=HSV[offset+2];


	                static const int sector_data[][3]=
	                {{1,3,0}, {1,0,2}, {3,0,1}, {0,2,1}, {0,1,3}, {2,1,0}};

	                ap_fixed<28,9> mul_scl=s_scale*S;



	                unsigned char H_scl = (unsigned char)H* hscale;
	                ap_fixed<28,9> h_fix=H*hscale - H_scl;
	            	if(H_scl >= 6)//for hrange=180, 0<H<255, then 0<h_i<8
	            		H_scl -= 6;

	            	tab[0] = V;
	                tab[1] = V*(1-mul_scl);
	                tab[2] = V*(1-mul_scl*h_fix);
	                tab[3] = V*(1-mul_scl+mul_scl*h_fix);

	                b = (tab[sector_data[H_scl][0]]);

	                g = (tab[sector_data[H_scl][1]]);
	                r = (tab[sector_data[H_scl][2]]);
			RGB[k].range(7, 0) = (unsigned char)(b);
			RGB[k].range(15, 8) = (unsigned char)(g);
			RGB[k].range(23, 16) = (unsigned char)(r);
			RGB_packed.range(k*XF_PIXELWIDTH(DST_T,NPC)+(XF_PIXELWIDTH(DST_T,NPC)-1),k*XF_PIXELWIDTH(DST_T,NPC))=RGB[k];

			}

			dst.write(i*(width>>XF_BITSHIFT(NPC))+j,RGB_packed);
		}
	}



}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void hsv2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " HSV image Type must be XF_8UC3");
	assert((DST_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " HSV image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and HSV plane dimensions mismatch");
	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC8))&& " 1,8 pixel parallelism is supported  ");

	xfhsv2bgr<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}

///////////////////////////////////////////////////////RGB2UYVY////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xfrgb2uyvy(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{

	//XF_PTNAME(XF_8UP) Y[],U,V;
	XF_PTNAME(XF_8UP) Y[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(XF_8UP) U[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(XF_8UP) V[XF_NPIXPERCYCLE(NPC)];

	ap_uint<24> RGB1[XF_NPIXPERCYCLE(NPC)];

#pragma HLS ARRAY_PARTITION variable=Y  complete
#pragma HLS ARRAY_PARTITION variable=U  complete
#pragma HLS ARRAY_PARTITION variable=V complete
#pragma HLS ARRAY_PARTITION variable=RGB1 complete

	unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_SRC)  Packed_rgb1;
	XF_PTNAME(XF_DEPTH(DST_T, NPC)) UYPacked,VYPacked,packed_uyvy[XF_NPIXPERCYCLE(NPC)];
	XF_SNAME(WORDWIDTH_DST) val_dst=0;
	uint8_t offset=0;
	uint16_t shift=0;
	bool evencol = true;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		evencol=true;

		columnloop:
		for(int j = 0; j < (width>>(XF_BITSHIFT(NPC))); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			val_dst=0;
//			evencol=true;
			Packed_rgb1 = src.read(idx++);
			xfExtractPixels<NPC, XF_WORDWIDTH(SRC_T,NPC), XF_DEPTH(SRC_T,NPC)>( RGB1,Packed_rgb1,0);
			 shift=0;
			for(int l = 0; l < (XF_NPIXPERCYCLE(NPC)); l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll

//				Y0[l]   = CalculateY(RGB1[offset+0].range(7,0), RGB1[offset+0].range(15,8), RGB1[offset+0].range(23,16));
				Y[l]   = CalculateY(RGB1[l].range(7,0), RGB1[l].range(15,8), RGB1[l].range(23,16));
				if(evencol)
				{
					U[l/2]   = CalculateU(RGB1[l].range(7,0), RGB1[l].range(15,8), RGB1[l].range(23,16));
					V[l/2]   = CalculateV(RGB1[l].range(7,0), RGB1[l].range(15,8), RGB1[l].range(23,16));

					//	U[l]    = CalculateU(RGB1[offset+0].range(7,0), RGB1[offset+0].range(15,8), RGB1[offset+0].range(23,16));
					//					V[l]    = CalculateV(RGB1[offset+0].range(7,0), RGB1[offset+0].range(15,8), RGB1[offset+0].range(23,16));

					UYPacked.range(7,0)=U[l/2];
					UYPacked.range(15,8)=Y[l];
					packed_uyvy[l]=UYPacked;
				}
				else
				{
					VYPacked.range(7,0)=V[l/2];
					VYPacked.range(15,8)=Y[l];
					packed_uyvy[l]=VYPacked;
				}
//				val_dst.range(l*8+) = packed_uyvy[l];
				xfPackPixels<NPC, XF_WORDWIDTH(DST_T,NPC), XF_DEPTH(DST_T,NPC)>(&packed_uyvy[l], val_dst, 0, 1, shift);

				evencol = evencol ? false : true;
			}
			dst.write(idx1++, val_dst);
		}
	}
}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void rgb2uyvy(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((DST_T == XF_16UC1)  && "  UYVY image Type must be XF_16UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGB image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and UYVY plane dimensions mismatch");

	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");

	xfrgb2uyvy<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
//////////////////////////////////////////////////////end of RGB2UYVY//////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////RGB2YUYV////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xfrgb2yuyv(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{

	//XF_PTNAME(XF_8UP) Y[],U,V;
	XF_PTNAME(XF_8UP) Y[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(XF_8UP) U[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(XF_8UP) V[XF_NPIXPERCYCLE(NPC)];

	ap_uint<24> RGB1[XF_NPIXPERCYCLE(NPC)];

#pragma HLS ARRAY_PARTITION variable=Y  complete
#pragma HLS ARRAY_PARTITION variable=U  complete
#pragma HLS ARRAY_PARTITION variable=V complete
#pragma HLS ARRAY_PARTITION variable=RGB1 complete

	unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_SRC)  Packed_rgb1;
	XF_PTNAME(XF_DEPTH(DST_T, NPC)) YUPacked,YVPacked,packed_yuyv[XF_NPIXPERCYCLE(NPC)];
	XF_SNAME(WORDWIDTH_DST) val_dst=0;
	uint8_t offset=0;
	uint16_t shift=0;
	bool evencol = true;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		evencol=true;

		columnloop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			val_dst=0;
			Packed_rgb1 = src.read(idx++);
			xfExtractPixels<NPC, XF_WORDWIDTH(SRC_T,NPC), XF_DEPTH(SRC_T,NPC)>( RGB1,Packed_rgb1,0);
			shift=0;
			for(int l = 0; l < (XF_NPIXPERCYCLE(NPC)); l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll

					Y[l]   = CalculateY(RGB1[l].range(7,0), RGB1[l].range(15,8), RGB1[l].range(23,16));
				if(evencol)
				{
					U[l/2]   = CalculateU(RGB1[l].range(7,0), RGB1[l].range(15,8), RGB1[l].range(23,16));
					V[l/2]   = CalculateV(RGB1[l].range(7,0), RGB1[l].range(15,8), RGB1[l].range(23,16));
					YUPacked.range(7,0)=Y[l];
					YUPacked.range(15,8)=U[l/2];
					packed_yuyv[l]=YUPacked;
				}
				else
				{
					YVPacked.range(7,0)=Y[l];
					YVPacked.range(15,8)=V[l/2];
					packed_yuyv[l]=YVPacked;
				}
				xfPackPixels<NPC, XF_WORDWIDTH(DST_T,NPC), XF_DEPTH(DST_T,NPC)>(&packed_yuyv[l], val_dst, 0, 1, shift);

				evencol = evencol ? false : true;
			}
			dst.write(idx1++, val_dst);
		}
	}
}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void rgb2yuyv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((DST_T == XF_16UC1)  && "  YUYV image Type must be XF_16UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGB image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "RGB and YUYV plane dimensions mismatch");

	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");


	xfrgb2yuyv<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
//////////////////////////////////////////////////////end of RGB2YUYV//////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////RGB2BGR////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xfrgb2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{

	ap_uint<24> RGB[XF_NPIXPERCYCLE(NPC)],BGR[XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=BGR complete


	unsigned long long int idx=0,idx1=0;
	XF_TNAME(SRC_T,NPC)  Packed_rgb1;
	XF_TNAME(DST_T,NPC)   val_dst=0;
	uint8_t offset=0;
	uint16_t shift=0;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			val_dst=0;
			Packed_rgb1 = src.read(idx++);
			for(int l = 0; l < (XF_NPIXPERCYCLE(NPC)); l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				RGB[l]=Packed_rgb1(l*24+23,l*24);
				BGR[l].range(23,16)  =  RGB[l].range(7,0);
				BGR[l].range(15,8)   =  RGB[l].range(15,8);
				BGR[l].range(7,0)    =  RGB[l].range(23,16);
				val_dst.range(l*24+23,l*24)=BGR[l];

			}
				dst.write(idx1++, val_dst);
		}
	}
}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void rgb2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " RGB image Type must be XF_8UC3");
	assert((DST_T == XF_8UC3)  && " BGR image Type must be XF_8UC3"); 
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " RGB image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and RGB plane dimensions mismatch");

	assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");


	xfrgb2bgr<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),((COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
//////////////////////////////////////////////////////end of RGB2BGR//////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////NV122UYVY////////////////////////////////////////////////////////////////////
template<int SRC_Y,int SRC_UV,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y,int WORDWIDTH_UV, int WORDWIDTH_DST, int TC>
void xfnv122uyvy(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
//assert();
	hls::stream<XF_SNAME(WORDWIDTH_UV)> uvStream;
#pragma HLS STREAM variable=&uvStream  depth=COLS/2
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_UV) uvPacked;
	XF_SNAME(WORDWIDTH_DST) uyvyPacked;
	unsigned long long int y_idx=0,uv_idx=0,out_idx=0;
	uint8_t y;

	int8_t u, v;
	bool evenRow = true, evenBlock = true,evenPix=true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			yPacked = _y.read(y_idx++);
			uyvyPacked=0;
			if(evenRow)
			{
				if(evenBlock)
				{
					uvPacked = _uv.read(uv_idx++);
					uvStream.write(uvPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					uvPacked = uvStream.read();
				}

			}
			for(int l=0;l<(XF_NPIXPERCYCLE(NPC));l++)
			{

				uint8_t y = yPacked.range(l*8+7,l*8+0);
					if(evenPix)
					{
					v = (uint8_t)uvPacked.range((l/2)*16+15,(l/2)*16+8) ;
					u = (uint8_t)uvPacked.range((l/2)*16+7, (l/2)*16+0) ;
					uyvyPacked.range(l*16+7,l*16+0)=u;
					uyvyPacked.range(l*16+15,l*16+8)=y;
					}
					else
					{
						uyvyPacked.range(l*16+7,l*16+0)=v;
						uyvyPacked.range(l*16+15,l*16+8)=y;
					}
					evenPix=evenPix?false:true;


			}
			dst.write(out_idx++,uyvyPacked);
			evenBlock = ((XF_NPIXPERCYCLE(NPC))!=1) ? true : evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for(int i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			uvStream.read();
		}
	}
}

#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_Y,int SRC_UV,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122uyvy(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_Y  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((SRC_UV == XF_8UC2)  && " UV image Type must be XF_8UC2");
	assert((DST_T  == XF_16UC1) && " UYVY image Type must be XF_16UC1");
	assert(((_y.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert((( _y.cols == _dst.cols) && (_y.rows == _dst.rows)) && "Y and UYVY plane dimensions mismatch");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");
	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC values must be same  ");
	}

	xfnv122uyvy<SRC_Y,SRC_UV,DST_T,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_Y,NPC),XF_WORDWIDTH(SRC_UV,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(COLS>>(XF_NPIXPERCYCLE(NPC)))>(_y,_uv,_dst,_y.rows,_y.cols);
}
//////////////////////////////////////////////////////end of NV122UYVY//////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////NV122UYVY////////////////////////////////////////////////////////////////////
template<int SRC_Y,int SRC_UV,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y,int WORDWIDTH_UV, int WORDWIDTH_DST, int TC>
void xfnv212uyvy(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
//assert();
	hls::stream<XF_SNAME(WORDWIDTH_UV)> uvStream;
#pragma HLS STREAM variable=&uvStream  depth=COLS/2
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_UV) uvPacked;
	XF_SNAME(WORDWIDTH_DST) uyvyPacked;
	unsigned long long int y_idx=0,uv_idx=0,out_idx=0;
	uint8_t y;

	int8_t u, v;
	bool evenRow = true, evenBlock = true,evenPix=true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			yPacked = _y.read(y_idx++);
			uyvyPacked=0;
			if(evenRow)
			{
				if(evenBlock)
				{
					uvPacked = _uv.read(uv_idx++);
					uvStream.write(uvPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					uvPacked = uvStream.read();
				}

			}
			for(int l=0;l<(XF_NPIXPERCYCLE(NPC));l++)
			{

					y = yPacked.range(l*8+7,l*8+0);
					if(evenPix)
					{
					u = (uint8_t)uvPacked.range((l/2)*16+15,(l/2)*16+8);
					v = (uint8_t)uvPacked.range((l/2)*16+7, (l/2)*16+0);
					uyvyPacked.range(l*16+7,l*16+0)=u;
					uyvyPacked.range(l*16+15,l*16+8)=y;
					}
					else
					{
						uyvyPacked.range(l*16+7,l*16+0)=v;
						uyvyPacked.range(l*16+15,l*16+8)=y;
					}
					evenPix=evenPix?false:true;


			}
			dst.write(out_idx++,uyvyPacked);
			evenBlock = ((XF_NPIXPERCYCLE(NPC))!=1) ? true : evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for(int i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			uvStream.read();
		}
	}
}

#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_Y,int SRC_UV,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212uyvy(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_Y  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((SRC_UV == XF_8UC2)  && " UV image Type must be XF_8UC2");
	assert((DST_T  == XF_16UC1) && " UYVY image Type must be XF_16UC1");
	assert(((_y.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert((( _y.cols == _dst.cols) && (_y.rows == _dst.rows)) && "Y and UYVY plane dimensions mismatch");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");
	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}
	xfnv212uyvy<SRC_Y,SRC_UV,DST_T,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_Y,NPC),XF_WORDWIDTH(SRC_UV,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_y,_uv,_dst,_y.rows,_y.cols);
}
//////////////////////////////////////////////////////end of NV122UYVY//////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////NV122YUYV////////////////////////////////////////////////////////////////////
template<int SRC_Y,int SRC_UV,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y,int WORDWIDTH_UV, int WORDWIDTH_DST, int TC>
void xfnv122yuyv(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
//assert();
	hls::stream<XF_SNAME(WORDWIDTH_UV)> uvStream;
#pragma HLS STREAM variable=&uvStream  depth=COLS/2
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_UV) uvPacked;
	XF_SNAME(WORDWIDTH_DST) uyvyPacked;
	unsigned long long int y_idx=0,uv_idx=0,out_idx=0;
	uint8_t y;

	int8_t u, v;
	bool evenRow = true, evenBlock = true,evenPix=true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			yPacked = _y.read(y_idx++);
			uyvyPacked=0;
			if(evenRow)
			{
				if(evenBlock)
				{
					uvPacked = _uv.read(uv_idx++);
					uvStream.write(uvPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					uvPacked = uvStream.read();
				}

			}
			for(int l=0;l<(XF_NPIXPERCYCLE(NPC));l++)
			{

				 y = yPacked.range(l*8+7,l*8+0);
					if(evenPix)
					{
					v = (uint8_t)uvPacked.range((l/2)*16+15,(l/2)*16+8);
					u = (uint8_t)uvPacked.range((l/2)*16+7, (l/2)*16+0);
					uyvyPacked.range(l*16+7,l*16+0)=y;
					uyvyPacked.range(l*16+15,l*16+8)=u;
					}
					else
					{
						uyvyPacked.range(l*16+7,l*16+0)=y;
						uyvyPacked.range(l*16+15,l*16+8)=v;
					}
					evenPix=evenPix?false:true;


			}
			dst.write(out_idx++,uyvyPacked);
			evenBlock = ((XF_NPIXPERCYCLE(NPC))!=1) ? true : evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for(int i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			uvStream.read();
		}
	}
}

#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_Y,int SRC_UV,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122yuyv(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_Y  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((SRC_UV == XF_8UC2)  && " UV image Type must be XF_8UC2");
	assert((DST_T  == XF_16UC1) && " YUYV image Type must be XF_16UC1");
	assert(((_y.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert((( _y.cols == _dst.cols) && (_y.rows == _dst.rows)) && "Y and Yuyv plane dimensions mismatch");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");
	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC,NPC_UV values must be same  ");
	}

	xfnv122yuyv<SRC_Y,SRC_UV,DST_T,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_Y,NPC),XF_WORDWIDTH(SRC_UV,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(COLS>>(XF_NPIXPERCYCLE(NPC)))>(_y,_uv,_dst,_y.rows,_y.cols);
}
//////////////////////////////////////////////////////end of NV122YUYV//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////NV122YUYV////////////////////////////////////////////////////////////////////
template<int SRC_Y,int SRC_UV,int DST_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y,int WORDWIDTH_UV, int WORDWIDTH_DST, int TC>
void xfnv212yuyv(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{
//assert();
	hls::stream<XF_SNAME(WORDWIDTH_UV)> uvStream;
#pragma HLS STREAM variable=&uvStream  depth=COLS/2
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_UV) uvPacked;
	XF_SNAME(WORDWIDTH_DST) uyvyPacked;
	unsigned long long int y_idx=0,uv_idx=0,out_idx=0;
	uint8_t y;

	int8_t u, v;
	bool evenRow = true, evenBlock = true,evenPix=true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			yPacked = _y.read(y_idx++);
			uyvyPacked=0;
			if(evenRow)
			{
				if(evenBlock)
				{
					uvPacked = _uv.read(uv_idx++);
					uvStream.write(uvPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					uvPacked = uvStream.read();
				}

			}
			for(int l=0;l<(XF_NPIXPERCYCLE(NPC));l++)
			{

					y = yPacked.range(l*8+7,l*8+0);
					if(evenPix)
					{
					u = (uint8_t)uvPacked.range((l/2)*16+15,(l/2)*16+8);
					v = (uint8_t)uvPacked.range((l/2)*16+7, (l/2)*16+0);
					uyvyPacked.range(l*16+7,l*16+0)=y;
					uyvyPacked.range(l*16+15,l*16+8)=u;
					}
					else
					{
						uyvyPacked.range(l*16+7,l*16+0)=y;
						uyvyPacked.range(l*16+15,l*16+8)=v;
					}
					evenPix=evenPix?false:true;


			}
			dst.write(out_idx++,uyvyPacked);
			evenBlock = ((XF_NPIXPERCYCLE(NPC))!=1) ? true : evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for(int i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			uvStream.read();
		}
	}
}

#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])


template<int SRC_Y,int SRC_UV,int DST_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212yuyv(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_Y  == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((SRC_UV == XF_8UC2)  && " VU image Type must be XF_8UC2");
	assert((DST_T  == XF_16UC1) && " YUYV image Type must be XF_16UC1");
	assert(((_y.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert((( _y.cols == _dst.cols) && (_y.rows == _dst.rows)) && "Y and Yuyv plane dimensions mismatch");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and VU planes dimensions mismatch");
	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the VU plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC values must be same  ");
	}
	xfnv212yuyv<SRC_Y,SRC_UV,DST_T,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_Y,NPC),XF_WORDWIDTH(SRC_UV,NPC_UV),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_y,_uv,_dst,_y.rows,_y.cols);
}
//////////////////////////////////////////////////////end of NV212YUYV//////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////BGR2UYVY////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xfbgr2uyvy(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{

	//XF_PTNAME(XF_8UP) Y[],U,V;
	XF_PTNAME(XF_8UP) Y[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(XF_8UP) U[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(XF_8UP) V[XF_NPIXPERCYCLE(NPC)];

	ap_uint<24> RGB1[XF_NPIXPERCYCLE(NPC)];

#pragma HLS ARRAY_PARTITION variable=Y  complete
#pragma HLS ARRAY_PARTITION variable=U  complete
#pragma HLS ARRAY_PARTITION variable=V complete
#pragma HLS ARRAY_PARTITION variable=RGB1 complete

	unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_SRC)  Packed_rgb1;
	XF_PTNAME(XF_DEPTH(DST_T, NPC)) UYPacked,VYPacked,packed_uyvy[XF_NPIXPERCYCLE(NPC)];
	XF_SNAME(WORDWIDTH_DST) val_dst=0;
	uint8_t offset=0;
	uint16_t shift=0;
	bool evencol = true;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		evencol=true;

		columnloop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			val_dst=0;
//			evencol=true;
			Packed_rgb1 = src.read(idx++);
			xfExtractPixels<NPC, XF_WORDWIDTH(SRC_T,NPC), XF_DEPTH(SRC_T,NPC)>( RGB1,Packed_rgb1,0);
			 shift=0;
			for(int l = 0; l < (XF_NPIXPERCYCLE(NPC)); l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll

//				Y0[l]   = CalculateY(RGB1[offset+0].range(7,0), RGB1[offset+0].range(15,8), RGB1[offset+0].range(23,16));
				Y[l]   = CalculateY( RGB1[l].range(23,16), RGB1[l].range(15,8),RGB1[l].range(7,0));
				if(evencol)
				{
					U[l/2]   = CalculateU(RGB1[l].range(23,16), RGB1[l].range(15,8),RGB1[l].range(7,0) );
					V[l/2]   = CalculateV(RGB1[l].range(23,16), RGB1[l].range(15,8),RGB1[l].range(7,0));

					//					U[l]    = CalculateU(RGB1[offset+0].range(7,0), RGB1[offset+0].range(15,8), RGB1[offset+0].range(23,16));
					//					V[l]    = CalculateV(RGB1[offset+0].range(7,0), RGB1[offset+0].range(15,8), RGB1[offset+0].range(23,16));

					UYPacked.range(7,0)=U[l/2];
					UYPacked.range(15,8)=Y[l];
					packed_uyvy[l]=UYPacked;
				}
				else
				{
					VYPacked.range(7,0)=V[l/2];
					VYPacked.range(15,8)=Y[l];
					packed_uyvy[l]=VYPacked;
				}
				//val_dst.range() = packed_uyvy[l];
				xfPackPixels<NPC, XF_WORDWIDTH(DST_T,NPC), XF_DEPTH(DST_T,NPC)>(&packed_uyvy[l], val_dst, 0, 1, shift);

				evencol = evencol ? false : true;
			}
			dst.write(idx1++, val_dst);
		}
	}
}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void bgr2uyvy(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((DST_T == XF_16UC1)  && "  UYVY image Type must be XF_16UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " BGR image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and UYVY plane dimensions mismatch");

	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");

	xfbgr2uyvy<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
//////////////////////////////////////////////////////end of BGR2UYVY//////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////BGR2YUYV////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xfbgr2yuyv(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{

	//XF_PTNAME(XF_8UP) Y[],U,V;
	XF_PTNAME(XF_8UP) Y[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(XF_8UP) U[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(XF_8UP) V[XF_NPIXPERCYCLE(NPC)];

	ap_uint<24> RGB1[XF_NPIXPERCYCLE(NPC)];

#pragma HLS ARRAY_PARTITION variable=Y  complete
#pragma HLS ARRAY_PARTITION variable=U  complete
#pragma HLS ARRAY_PARTITION variable=V complete
#pragma HLS ARRAY_PARTITION variable=RGB1 complete

	unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_SRC)  Packed_rgb1;
	XF_PTNAME(XF_DEPTH(DST_T, NPC)) YUPacked,YVPacked,packed_yuyv[XF_NPIXPERCYCLE(NPC)];
	XF_SNAME(WORDWIDTH_DST) val_dst=0;
	uint8_t offset=0;
	uint16_t shift=0;
	bool evencol = true;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		evencol=true;

		columnloop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			val_dst=0;
			Packed_rgb1 = src.read(idx++);
			xfExtractPixels<NPC, XF_WORDWIDTH(SRC_T,NPC), XF_DEPTH(SRC_T,NPC)>( RGB1,Packed_rgb1,0);
			shift=0;
			for(int l = 0; l < (XF_NPIXPERCYCLE(NPC)); l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll

					Y[l]   = CalculateY( RGB1[l].range(23,16), RGB1[l].range(15,8),RGB1[l].range(7,0));
				if(evencol)
				{
					U[l/2]   = CalculateU(RGB1[l].range(23,16), RGB1[l].range(15,8),RGB1[l].range(7,0));
					V[l/2]   = CalculateV(RGB1[l].range(23,16), RGB1[l].range(15,8),RGB1[l].range(7,0));
					YUPacked.range(7,0)=Y[l];
					YUPacked.range(15,8)=U[l/2];
					packed_yuyv[l]=YUPacked;
				}
				else
				{
					YVPacked.range(7,0)=Y[l];
					YVPacked.range(15,8)=V[l/2];
					packed_yuyv[l]=YVPacked;
				}
				xfPackPixels<NPC, XF_WORDWIDTH(DST_T,NPC), XF_DEPTH(DST_T,NPC)>(&packed_yuyv[l], val_dst, 0, 1, shift);

				evencol = evencol ? false : true;
			}
			dst.write(idx1++, val_dst);
		}
	}
}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void bgr2yuyv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((DST_T == XF_16UC1)  && "  YUYV image Type must be XF_16UC1");
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " BGR image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and YUYV plane dimensions mismatch");

	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");



	xfbgr2yuyv<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
////////////////////////////////////////////////////////end of BGR2YUYV//////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////BGR2RGB////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xfbgr2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<DST_T, ROWS, COLS, NPC> & dst, unsigned short int height,unsigned short int width)
{

	ap_uint<24> RGB[XF_NPIXPERCYCLE(NPC)],BGR[XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=BGR complete


	unsigned long long int idx=0,idx1=0;
	XF_SNAME(WORDWIDTH_SRC)  Packed_rgb1;
	XF_SNAME(WORDWIDTH_DST)   val_dst=0;
	uint8_t offset=0;
	uint16_t shift=0;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			val_dst=0;
			Packed_rgb1 = src.read(idx++);
			for(int l = 0; l < (XF_NPIXPERCYCLE(NPC)); l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				xfExtractPixels<NPC, XF_WORDWIDTH(SRC_T,NPC), XF_DEPTH(SRC_T,NPC)>( BGR,Packed_rgb1,0);

				RGB[l].range(23,16)  =  BGR[l].range(7,0);
				RGB[l].range(15,8)   =  BGR[l].range(15,8);
				RGB[l].range(7,0)    =  BGR[l].range(23,16);
				val_dst.range(l*XF_PIXELWIDTH(SRC_T,NPC)+XF_PIXELWIDTH(SRC_T,NPC)-1,l*XF_PIXELWIDTH(SRC_T,NPC))=RGB[l];
			}
			dst.write(idx1++, val_dst);
		}
	}
}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void bgr2rgb(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((DST_T == XF_8UC3)  && " RGB image Type must be XF_8UC3"); 
	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && " BGR image rows and cols should be less than ROWS, COLS");
	assert((( _dst.cols == _src.cols) && (_dst.rows == _src.rows)) && "BGR and RGB plane dimensions mismatch");

	assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");


	xfbgr2rgb<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_src,_dst,_src.rows,_src.cols);
}
//////////////////////////////////////////////////////end of BGR2RGB//////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////RGB2INV12////////////////////////////////////////////////////////////////////////////
template<int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void Kernbgr2Nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _rgba, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,uint16_t height,uint16_t width)
{
	unsigned long long int idx=0,idx1=0;
	ap_uint<32> rgba;
	ap_uint<16> val1;
	uint8_t y, u, v;
	bool evenRow = true, evenBlock = true;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			rgba = _rgba.read(i*width+j);
			uint8_t b = rgba.range(7,0);
			uint8_t g = rgba.range(15,8);
			uint8_t r = rgba.range(23,16);

			y = CalculateY(r, g, b);
			if(evenRow)
			{
				u = CalculateU(r, g, b);
				v = CalculateV(r, g, b);
			}
			_y.write(idx++,y);
			if(evenRow)
			{
				if((j & 0x01) == 0)
				//{
					_uv.write(idx1++,u | (uint16_t)v << 8);
					//_uv.write(v);
				//}
				//	_uv.write(u | (uint16_t)v << 8);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

/////////////////////////////////////////////////////////RGB2INV12////////////////////////////////////////////////////////////////////////////
template<int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFbgr2Nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,uint16_t height,uint16_t width)
{

		width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
			Kernbgr2Nv12<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV>(_src, _y, _uv,height,width);

		}else
		{
			Kernbgr2Nv12_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,(COLS>>XF_BITSHIFT(NPC)),(1<<(XF_BITSHIFT(NPC)+1))>(_src, _y, _uv,height,width);

		}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
template <int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC=1,int NPC_UV=1>
void bgr2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((Y_T == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " UV image Type must be XF_8UC2");

	assert(((_src.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( _src.cols == _y.cols) && (_src.rows == _y.rows)) && "Y and BGR plane dimensions mismatch");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC values must be same  ");
	}

	xFbgr2Nv12<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y,_uv,_src.rows,_src.cols);
}/////////////////////////////////////////////////////////end of RGB2NV12/////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////RGB2NV21//////////////////////////////////////////////////////////////////////
template<int SRC_T, int Y_T, int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_VU>
void Kernbgr2Nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _rgba, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _vu,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
	XF_SNAME(XF_32UW) rgba;
	unsigned long long int idx=0,idx1=0;
	uint8_t y, u, v;
	bool evenRow = true, evenBlock = true;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			rgba = _rgba.read(i*width+j);
			uint8_t b = rgba.range(7,0);
			uint8_t g = rgba.range(15,8);
			uint8_t r = rgba.range(23,16);

			y = CalculateY(r, g, b);
			if(evenRow)
			{
				u = CalculateU(r, g, b);
				v = CalculateV(r, g, b);
			}
			_y.write(idx++,y);
			if(evenRow)
			{
				if((j & 0x01)==0)
					_vu.write(idx1++,v | ((uint16_t)u << 8));
			}
		}
		evenRow = evenRow ? false : true;
	}
}
template<int SRC_T, int Y_T, int UV_T,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFbgr2Nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv,uint16_t height ,uint16_t width)
{
		width=width>>XF_BITSHIFT(NPC);
		if(NPC==1)
		{
			Kernbgr2Nv21<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV>(_src, _y, _uv,height,width);
		}
		else
		{
			Kernbgr2Nv21_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_CHANNELS(SRC_T,NPC),WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,(COLS>>XF_BITSHIFT(NPC)),(1<<(XF_BITSHIFT(NPC)+1))>(_src, _y, _uv,height,width);

		}


}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
template <int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC=1,int NPC_UV=1>
void bgr2nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_8UC3)  && " BGR image Type must be XF_8UC3");
	assert((Y_T == XF_8UC1)  && " Y image Type must be XF_8UC1");
	assert((UV_T == XF_8UC2)  && " UV image Type must be XF_8UC2");

	assert(((_src.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image ROWS and COLS should be less than ROWS, COLS");
	assert((( _src.cols == _y.cols) && (_src.rows == _y.rows)) && "Y and BGR plane dimensions mismatch");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC values must be same  ");
	}
	xFbgr2Nv21<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y,_uv,_src.rows,_src.cols);
}
////////////////////////////////////////////////////////end of BGR2NV21////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////YUYV2RGB//////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernYuyv2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _yuyv,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba,uint16_t height, uint16_t width)
{
	XF_SNAME(WORDWIDTH_DST) rgba;
	XF_SNAME(WORDWIDTH_SRC) yu,yv;
	XF_PTNAME(XF_8UP)  r, g, b;
	int8_t y1, y2, u, v;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	unsigned long long int idx=0;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ColLoop:
		for(int j = 0; j < width ; j += 2)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

			yu = _yuyv.read(i*width+j);
			yv = _yuyv.read(i*width+j+1);
			u = (uint8_t)yu.range(15,8) - 128;
			y1 = (yu.range(7,0) > 16) ? ((uint8_t)yu.range(7,0) - 16) : 0;

			v = (uint8_t)yv.range(15,8) - 128;
			y2 = (yv.range(7,0) > 16) ? ((uint8_t)yv.range(7,0) - 16) : 0;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			r = CalculateR(y1, V2Rtemp, v);
			g = CalculateG(y1, U2Gtemp, V2Gtemp);
			b = CalculateB(y1, U2Btemp, u);

			rgba = ((ap_uint24_t)b) | ((ap_uint24_t)g << 8) | ((ap_uint24_t)r << 16 );
			_rgba.write(idx++,rgba);

			r = CalculateR(y2, V2Rtemp, v);
			g = CalculateG(y2, U2Gtemp, V2Gtemp);
			b = CalculateB(y2, U2Btemp, u);

			rgba = ((ap_uint24_t)b) | ((ap_uint24_t)g << 8) | ((ap_uint24_t)r << 16);
			_rgba.write(idx++,rgba);
		}
	}
}

//Yuyv2Rgba
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFYuyv2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst,uint16_t height, uint16_t width)
{

	width=width>>XF_BITSHIFT(NPC);
	if(NPC==1)
	{
	KernYuyv2bgr<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);
	}
	else
	{
	KernYuyv2bgr_ro<SRC_T,DST_T,ROWS,COLS,NPC,XF_CHANNELS(DST_T,NPC),WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);

	}

}

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void yuyv2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  	 == XF_16UC1) &&  " YUYV plane Type must be XF_16UC1");
	assert((DST_T    == XF_8UC3)  &&  " BGR plane Type must be XF_8UC3");

	assert(( (_src.rows <= ROWS ) && (_src.cols <= COLS)) && " YUYV image rows and cols should be less than ROWS, COLS");
	assert(( (_dst.cols == _src.cols) && (_dst.rows == _src.rows))&& "YUYV and BGR planes dimensions mismatch");

	assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");

	xFYuyv2bgr<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_dst,_src.rows,_src.cols);
}
///////////////////////////////////////////////////////UYVY2BGR///////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernUyvy2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & _rgba, uint16_t height, uint16_t width)
{
	XF_SNAME(WORDWIDTH_DST) rgba;

	XF_SNAME(WORDWIDTH_SRC) uyvy;

	XF_SNAME(WORDWIDTH_SRC) uy;
	XF_SNAME(WORDWIDTH_SRC) vy;

	unsigned long long int idx=0;
	XF_PTNAME(XF_8UP)  r, g, b;
	int8_t y1, y2, u, v;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ColLoop:
		for(int j = 0; j < width; j+=2)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

			//uyvy = _uyvy.read();

			uy = _uyvy.read(i*width+j);
			vy = _uyvy.read(i*width+j+1);

			u = (uint8_t)uy.range(7,0) - 128;

/*			if(uyvy.range(15,8) > 16)
				y1 = (uint8_t)uyvy.range(15,8) - 16;
			else
				y1 = 0;*/

			y1 = (uy.range(15,8) > 16) ? ((uint8_t)uy.range(15,8) - 16) : 0;

			v = (uint8_t)vy.range(7,0) - 128;

/*			if(uyvy.range(31,24) > 16)
				y2 = ((uint8_t)uyvy.range(31,24) - 16);
			else
				y2 = 0;*/
			y2 = (vy.range(15,8) > 16) ? ((uint8_t)vy.range(15,8) - 16) : 0;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			r = CalculateR(y1, V2Rtemp, v);
			g = CalculateG(y1, U2Gtemp, V2Gtemp);
			b = CalculateB(y1, U2Btemp, u);

			rgba = ((ap_uint24_t)b) | ((ap_uint24_t)g << 8) | ((ap_uint24_t)r << 16) ;
			_rgba.write(idx,rgba);
			idx++;
			r = CalculateR(y2, V2Rtemp, v);
			g = CalculateG(y2, U2Gtemp, V2Gtemp);
			b = CalculateB(y2, U2Btemp, u);

			rgba = ((ap_uint24_t)b) | ((ap_uint24_t)g << 8) | ((ap_uint24_t)r << 16) ;
			_rgba.write(idx,rgba);
			idx++;
		}
	}
}

template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFUyvy2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst,uint16_t height, uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);

		if(NPC==1)
		{
			KernUyvy2bgr<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);
		}else
		{
			KernUyvy2bgr_ro<SRC_T,DST_T,ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);

		}

}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void uyvy2bgr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
	assert((SRC_T  	 == XF_16UC1) &&  " UYVY plane Type must be XF_16UC1");
	assert((DST_T    == XF_8UC3)  &&  " BGR plane Type must be XF_8UC3");

	assert(( (_src.rows <= ROWS ) && (_src.cols <= COLS)) && " UYVY image rows and cols should be less than ROWS, COLS");
	assert(( (_dst.cols == _src.cols) && (_dst.rows == _src.rows))&& "UYVY and BGR planes dimensions mismatch");

	assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");

	xFUyvy2bgr<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_dst,_src.rows,_src.cols);


}
////////////////////////////////////////////////////////end of UYVY2BGR////////////////////////////////////////////////////////////////
//Yuyv2Nv12
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFYuyv2Nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<Y_T, ROWS, COLS, NPC> & _y_image,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image,uint16_t height, uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);

	if(NPC == XF_NPPC1)
	{
		KernYuyv2Nv21<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC))>(_src, _y_image, _uv_image, height, width);
	}
	else
	{
	KernYuyv2Nv21_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(_src, _y_image, _uv_image, height, width);
	}
}
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_uv_image.data"[0:"_uv_image.size"])
template<int SRC_T,int Y_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void yuyv2nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<Y_T, ROWS, COLS, NPC> & _y_image,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_16UC1) && " YUYV plane Type must be XF_16UC1");
	assert((Y_T    == XF_8UC1)  &&  " Y plane Type must be XF_8UC1");
	assert((UV_T   == XF_8UC2)  &&  " VU image Type must be XF_8UC2");

	assert(( (_y_image.rows <= ROWS ) && (_y_image.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(( (_y_image.cols == (_uv_image.cols<<1)) && (_y_image.rows == (_uv_image.rows<<1)))&& "Y and UV planes dimensions mismatch");
	assert(( (_y_image.cols == _src.cols) && (_y_image.rows == _src.rows))&& "Y and YUYV planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC, NPC_UV values must be same  ");
	}
	xFYuyv2Nv21<SRC_T,Y_T,UV_T,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y_image,_uv_image,_src.rows,_src.cols);
}
//Yuyv2nv21
//Uyvy2Nv21
template<int SRC_T,int Y_T,int UV_T,int ROWS, int COLS, int NPC, int NPC_UV,int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFUyvy2Nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & uyvy,xf::Mat<Y_T, ROWS, COLS, NPC> & y_plane,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & uv_plane,uint16_t height, uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);


	if(NPC == XF_NPPC1)
	{

#pragma HLS DATAFLOW
		KernUyvy2Nv21<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC))> (uyvy, y_plane, uv_plane, height, width);
	}
	else
	{
		KernUyvy2Nv21_ro<SRC_T,Y_T,UV_T,ROWS,COLS,NPC,NPC_UV,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<NPC)>>1)>(uyvy, y_plane, uv_plane, height, width);
	}
}

	#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
	#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
	#pragma SDS data access_pattern("_uv_image.data":SEQUENTIAL)
	#pragma SDS data copy("_src.data"[0:"_src.size"])
	#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
	#pragma SDS data copy("_uv_image.data"[0:"_uv_image.size"])
template<int SRC_T,int Y_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void uyvy2nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<Y_T, ROWS, COLS, NPC> & _y_image,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image)
{
#pragma HLS INLINE OFF

	assert((SRC_T  == XF_16UC1) && " UYVY plane Type must be XF_16UC1");
	assert((Y_T    == XF_8UC1)  &&  " Y plane Type must be XF_8UC1");
	assert((UV_T   == XF_8UC2)  &&  " UV image Type must be XF_8UC2");

	assert(( (_y_image.rows <= ROWS ) && (_y_image.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(( (_y_image.cols == (_uv_image.cols<<1)) && (_y_image.rows == (_uv_image.rows<<1)))&& "Y and UV planes dimensions mismatch");
	assert(( (_y_image.cols == _src.cols) && (_y_image.rows == _src.rows))&& "Y and UYVY planes dimensions mismatch");

	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");


	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC, NPC_UV values must be same  ");
	}

	xFUyvy2Nv21<SRC_T,Y_T,UV_T,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(_src,_y_image,_uv_image,_src.rows,_src.cols);
}
///////////////////////////////////////////////////////NV122NV21////////////////////////////////////////////////////////////////////
template<int SRC_Y,int SRC_UV,int ROWS, int COLS, int NPC,int NPC_UV, int WORDWIDTH_Y,int WORDWIDTH_UV, int TC>
void xfnv122nv21(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<SRC_Y, ROWS, COLS, NPC> & out_y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & out_uv,unsigned short int height,unsigned short int width )
{
//assert();
	XF_SNAME(WORDWIDTH_Y) yPacked=0;
	XF_SNAME(WORDWIDTH_UV) uvPacked[8],vuPacked[8],packed_Data=0,val_dst=0;
	unsigned long long int y_idx=0,uv_idx=0;
	unsigned long long int outUV_idx=0;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		ColLoop:
		for(int j = 0; j < width>>XF_BITSHIFT(NPC); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			yPacked = _y.read(i*(width>>XF_BITSHIFT(NPC))+j);
			out_y.write(y_idx++, yPacked);

			if(i<_uv.rows && j<(_uv.cols>>XF_BITSHIFT(NPC_UV)))
			{
				packed_Data = _uv.read(uv_idx++);
			}

			for(int l=0;l<(XF_NPIXPERCYCLE(NPC_UV));l++)
			{
#pragma HLS unroll

				uvPacked[l]=packed_Data(l*16+15,l*16);
				vuPacked[l].range(15,8)=uvPacked[l].range(7,0);
				vuPacked[l].range(7,0)=uvPacked[l].range(15,8);
				val_dst.range(l*16+15,l*16)=vuPacked[l];

			}
			if(i<_uv.rows && j<(_uv.cols>>XF_BITSHIFT(NPC_UV)))
			{
				out_uv.write(outUV_idx++,val_dst);
			}
		}

	}
}

#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
#pragma SDS data access_pattern("out_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("out_uv.data":SEQUENTIAL)
#pragma SDS data copy("out_y.data"[0:"out_y.size"])
#pragma SDS data copy("out_uv.data"[0:"out_uv.size"])

template<int SRC_Y,int SRC_UV,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122nv21(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<SRC_Y, ROWS, COLS, NPC> & out_y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & out_uv)
{
#pragma HLS INLINE OFF
	assert((SRC_Y  == XF_8UC1)  && " Y plane Type must be XF_8UC1");
	assert((SRC_UV  == XF_8UC2)  && " UV image Type must be XF_8UC2");

	assert(((_y.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and UV planes dimensions mismatch");
	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8) ) && " 1,2,4,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC, NPC_UV values must be same  ");
	}

	xfnv122nv21<SRC_Y,SRC_UV,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_Y,NPC),XF_WORDWIDTH(SRC_UV,NPC_UV),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_y,_uv,out_y,out_uv,_y.rows,_y.cols);
}
//////////////////////////////////////////////////////end of NV122NV21//////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////NV212NV12////////////////////////////////////////////////////////////////////

#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
#pragma SDS data access_pattern("out_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("out_uv.data":SEQUENTIAL)
#pragma SDS data copy("out_y.data"[0:"out_y.size"])
#pragma SDS data copy("out_uv.data"[0:"out_uv.size"])
template<int SRC_Y,int SRC_UV,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212nv12(xf::Mat<SRC_Y, ROWS, COLS, NPC> & _y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & _uv,xf::Mat<SRC_Y, ROWS, COLS, NPC> & out_y,xf::Mat<SRC_UV, ROWS/2, COLS/2, NPC_UV> & out_uv)
{
#pragma HLS INLINE OFF
	assert((SRC_Y  == XF_8UC1)  && " Y plane Type must be XF_8UC1");
	assert((SRC_UV  == XF_8UC2)  && " VU image Type must be XF_8UC2");

	assert(((_y.rows <= ROWS ) && (_y.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert(( (_y.cols == (_uv.cols<<1)) && (_y.rows == (_uv.rows<<1)))&& "Y and VU planes dimensions mismatch");
	if(NPC != XF_NPPC1)
	{
		assert((NPC==(NPC_UV*2))&& " NPC of Y plane must be double the UV plane for multipixel parallelism  ");
		assert(( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8) ) && " 1,2,4,8 pixel parallelism is supported  ");
	}
	else
	{
		assert((NPC==NPC_UV==XF_NPPC1)&& " Both NPC, NPC_UV values must be same  ");
	}

	xfnv122nv21<SRC_Y,SRC_UV,ROWS, COLS, NPC,NPC_UV,XF_WORDWIDTH(SRC_Y,NPC),XF_WORDWIDTH(SRC_UV,NPC_UV),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(_y,_uv,out_y,out_uv,_y.rows,_y.cols);
}
//////////////////////////////////////////////////////end of NV212NV12//////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////UYVY2YUYV////////////////////////////////////////////////////////////////////
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC,int WORDWIDTH_SRC,int WORDWIDTH_DST, int TC>
void xfuyvy2yuyv(xf::Mat<SRC_T, ROWS, COLS, NPC> & uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & yuyv,unsigned short int height,unsigned short int width )
{
//assert();
	XF_SNAME(WORDWIDTH_SRC) uy=0;
	XF_SNAME(WORDWIDTH_DST) yu[8],uyPacked[8],packed_Data=0,val_dst=0;
	unsigned long long int y_idx=0,uv_idx=0;
	unsigned long long int outUV_idx=0;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		ColLoop:
		for(int j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			uy = uyvy.read(i*(width>>XF_BITSHIFT(NPC))+j);

			for(int l=0;l<(XF_NPIXPERCYCLE(NPC));l++)
			{
#pragma HLS unroll

				uyPacked[l]=uy(l*16+15,l*16);
				yu[l].range(15,8)=uyPacked[l].range(7,0);
				yu[l].range(7,0)=uyPacked[l].range(15,8);
				val_dst.range(l*16+15,l*16)=yu[l];

			}
			yuyv.write(outUV_idx++,val_dst);
		}

	}
}

#pragma SDS data access_pattern("yuyv.data":SEQUENTIAL)
#pragma SDS data access_pattern("uyvy.data":SEQUENTIAL)
#pragma SDS data copy("yuyv.data"[0:"yuyv.size"])
#pragma SDS data copy("uyvy.data"[0:"uyvy.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void uyvy2yuyv(xf::Mat<SRC_T, ROWS, COLS, NPC> & uyvy,xf::Mat<DST_T, ROWS, COLS, NPC> & yuyv)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_16UC1)  && " UYVY image Type must be XF_16UC1");
	assert((DST_T == XF_16UC1)  && " YUYV image Type must be XF_16UC1");
	assert(((yuyv.rows <= ROWS ) && (yuyv.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert((( yuyv.cols == uyvy.cols) && (yuyv.rows == uyvy.rows)) && "YUYV and UYVY plane dimensions mismatch");
	assert( ( (NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8) ) && " 1,2,4,8 pixel parallelism is supported  ");


	xfuyvy2yuyv<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(uyvy,yuyv,uyvy.rows,uyvy.cols);
}
//////////////////////////////////////////////////////end of UYVY2YUYV//////////////////////////////////////////////////////////////

#pragma SDS data access_pattern("yuyv.data":SEQUENTIAL)
#pragma SDS data access_pattern("uyvy.data":SEQUENTIAL)
#pragma SDS data copy("yuyv.data"[0:"yuyv.size"])
#pragma SDS data copy("uyvy.data"[0:"uyvy.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void yuyv2uyvy(xf::Mat<SRC_T, ROWS, COLS, NPC> & yuyv,xf::Mat<DST_T, ROWS, COLS, NPC> & uyvy)
{
#pragma HLS INLINE OFF
	assert((SRC_T  == XF_16UC1)  && " YUYV image Type must be XF_16UC1");
	assert((DST_T == XF_16UC1)  && " UYVY image Type must be XF_16UC1");
	assert(((yuyv.rows <= ROWS ) && (yuyv.cols <= COLS)) && " Y image rows and cols should be less than ROWS, COLS");
	assert((( yuyv.cols == uyvy.cols) && (yuyv.rows == uyvy.rows)) && "YUYV and UYVY plane dimensions mismatch");
	assert(((NPC==XF_NPPC1) || (NPC==XF_NPPC2) || (NPC==XF_NPPC4) ||(NPC==XF_NPPC8))&& " 1,2,4,8 pixel parallelism is supported  ");

	xfuyvy2yuyv<SRC_T,DST_T,ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(ROWS*(COLS>>(XF_NPIXPERCYCLE(NPC))))>(yuyv,uyvy,uyvy.rows,uyvy.cols);
}
//////////////////////////////////////////////////////end of YUYV2UYVY//////////////////////////////////////////////////////////////
}
#endif
