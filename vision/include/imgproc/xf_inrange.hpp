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

#ifndef _XF_INRANGE_HPP_
#define _XF_INRANGE_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

typedef unsigned short  uint16_t;
typedef unsigned char  uchar;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{


template<int WORDWIDTH_SRC,int WORDWIDTH_DST,int DEPTH_SRC,int DEPTH_DST,int SRC_T,int NPC>
void inrangeproc(XF_SNAME(WORDWIDTH_SRC) &val_src,XF_SNAME(WORDWIDTH_DST) &tmp_val,XF_PTNAME(DEPTH_DST) channel_out[XF_CHANNELS(SRC_T,NPC)],XF_PTNAME(DEPTH_DST) _lower_thresh[XF_CHANNELS(SRC_T,NPC)],XF_PTNAME(DEPTH_DST) _upper_thresh[XF_CHANNELS(SRC_T,NPC)])
{
	XF_PTNAME(DEPTH_SRC)  p;
	for(ap_uint<13> k = 0,c=0; k < ((XF_WORDDEPTH(WORDWIDTH_SRC)));k += XF_PIXELDEPTH(DEPTH_DST),c++)
		{
#pragma HLS unroll
			p=val_src.range(k+(XF_PIXELDEPTH(DEPTH_DST)-1),k);

			channel_out[c] = (( p >= _lower_thresh[c]) && ( p <= _upper_thresh[c]))? (ap_uint<8>)255 : (ap_uint<8>)0;

		}
		if(XF_CHANNELS(SRC_T,NPC)!=1)
		{
			tmp_val= (channel_out[0] & channel_out[1] & channel_out[2]);
		}
		else
		{
			tmp_val= (channel_out[0]);
		}
}

/**
 * xFThresholdKernel: Thresholds an input image and produces an output boolean image depending
 * 		upon the type of thresholding.
 * Input   : _src_mat, _thresh_type, _binary_thresh_val,  _upper_range and _lower_range
 * Output  : _dst_mat
 */
template<int SRC_T,int DST_T, int ROWS, int COLS,int DEPTH_SRC,int DEPTH_DST, int NPC, int WORDWIDTH_SRC,
int WORDWIDTH_DST, int COLS_TRIP>
void xFinRangeKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat,
		unsigned char lower_thresh[XF_CHANNELS(SRC_T,NPC)],unsigned char upper_thresh[XF_CHANNELS(SRC_T,NPC)], unsigned short height, unsigned short width)
{
	XF_SNAME(WORDWIDTH_SRC) val_src;
	XF_SNAME(WORDWIDTH_DST) val_dst,tmp_val;
	//,out;
	XF_PTNAME(DEPTH_DST)  _lower_thresh[XF_CHANNELS(SRC_T,NPC)];//=(XF_PTNAME(DEPTH))lower_thresh;
	XF_PTNAME(DEPTH_DST)  _upper_thresh[XF_CHANNELS(SRC_T,NPC)];//=(XF_PTNAME(DEPTH))upper_thresh;
	XF_PTNAME(DEPTH_DST)  channel_out[XF_CHANNELS(SRC_T,NPC)];

	for(int i=0;i< XF_CHANNELS(SRC_T,NPC);i++)
	{
		_lower_thresh[i]=(XF_PTNAME(DEPTH_SRC))lower_thresh[i];
		_upper_thresh[i]=(XF_PTNAME(DEPTH_SRC))upper_thresh[i];
	}
	ap_uint<13> i, j, k,c;
	rowLoop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src = (XF_SNAME(WORDWIDTH_SRC)) (_src_mat.read(i*width+j));  

			inrangeproc<WORDWIDTH_SRC,WORDWIDTH_DST,DEPTH_SRC,DEPTH_DST,SRC_T,NPC>(val_src,tmp_val,channel_out,_lower_thresh,_upper_thresh);

			_dst_mat.write(i*width+j, tmp_val);  //writing the val_dst into output stream _dst
		}
	}
}


#pragma SDS data access_pattern("src.data":SEQUENTIAL, "dst.data":SEQUENTIAL)
#pragma SDS data copy("src.data"[0:"src.size"], "dst.data"[0:"dst.size"])
//#pragma SDS data mem_attribute("src.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
//#pragma SDS data mem_attribute("dst.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)


template<int SRC_T,int DST_T, int ROWS, int COLS,int NPC=1>
void inRange(xf::Mat<SRC_T, ROWS, COLS, NPC> & src,unsigned char lower_thresh[XF_CHANNELS(SRC_T,NPC)],unsigned char upper_thresh[XF_CHANNELS(SRC_T,NPC)],xf::Mat<DST_T, ROWS, COLS, NPC> & dst)
{

	unsigned short width = src.cols >> XF_BITSHIFT(NPC);
	unsigned short height = src.rows;

	assert(((SRC_T == XF_8UC1) || (SRC_T == XF_8UC3)) && "Type must be XF_8UC1 or XF_8UC3");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) &&
			"NPC must be XF_NPPC1, XF_NPPC8");

	assert(((lower_thresh[0] >= 0) && (lower_thresh[0] <= 255)) &&
			"lower_thresh must be with the range of 0 to 255");

	assert(((upper_thresh[0] >= 0) && (upper_thresh[0] <= 255)) &&
			"lower_thresh must be with the range of 0 to 255");

	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");

#pragma HLS INLINE OFF

	xFinRangeKernel<SRC_T, DST_T, ROWS,COLS,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC))>
	(src,dst,lower_thresh,upper_thresh,height,width);

}
}

#endif
