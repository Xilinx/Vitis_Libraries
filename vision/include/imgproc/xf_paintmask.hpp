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

#ifndef _XF_THRESHOLD_HPP_
#define _XF_THRESHOLD_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

typedef unsigned short  uint16_t;
typedef unsigned char  uchar;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{

/*Paint mask function masks certain area of image depends on input mask */
template<int SRC_T, int MASK_T, int ROWS, int COLS,int DEPTH, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int WORDWIDTH_MASK,int COLS_TRIP>
void xFpaintmaskKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, xf::Mat<SRC_T, ROWS, COLS, NPC> & _in_mask, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat, xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>& color, unsigned short height, unsigned short width)
{
	XF_SNAME(WORDWIDTH_SRC) val_src;
	XF_SNAME(WORDWIDTH_MASK) in_mask;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH)  p,mask;
	short int depth=XF_DTPIXELDEPTH(SRC_T,NPC)/XF_CHANNELS(SRC_T,NPC);
	XF_PTNAME(DEPTH) arr_color[XF_CHANNELS(SRC_T,NPC)];
#pragma HLS ARRAY_PARTITION variable=arr_color dim=1 complete
	for(int i=0;i<(XF_CHANNELS(SRC_T,NPC));i++)
	{
		arr_color[i]=color.val[i];
	}
	ap_uint<13> i, j, k, planes;
	rowLoop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ap_uint<8> channels=XF_CHANNELS(SRC_T,NPC);
		colLoop:
		for(j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src = (XF_SNAME(WORDWIDTH_SRC)) (_src_mat.read(i*width+j));  //reading the source stream _src into val_src
			in_mask = (XF_SNAME(WORDWIDTH_MASK)) (_in_mask.read(i*width+j)); //reading the input mask stream _in_mask into in_mask
			for(k = 0, planes=0;k < (XF_WORDDEPTH(WORDWIDTH_SRC));k += depth,planes++)
			{
#pragma HLS unroll
				p = val_src.range(k+(depth-1),k);
				mask = in_mask.range(k+(depth-1),k);
	            if(mask!=0)
	            {
	            	if(NPC!=1)
	            		val_dst.range(k+(depth-1),k) = arr_color[0];
	            	else
	            	val_dst.range(k+(depth-1),k) = arr_color[planes];
	            }
	            else
	            {
	            	val_dst.range(k+(depth-1),k) = p;
	            }
			}

			_dst_mat.write(i*width+j,val_dst);  //writing the val_dst into output stream _dst
		}
	}
}


#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data access_pattern("in_mask.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"], "in_mask.data"[0:"in_mask.size"])
//#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
//#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
//#pragma SDS data mem_attribute("in_mask.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)


/* Paint mask API call*/
template< int SRC_T,int MASK_T, int ROWS, int COLS,int NPC=1>
void paintmask(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, xf::Mat<MASK_T, ROWS, COLS, NPC> & in_mask, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat, unsigned char _color[XF_CHANNELS(SRC_T,NPC)])
{

	unsigned short width = _src_mat.cols >> XF_BITSHIFT(NPC);
	unsigned short height = _src_mat.rows;
	xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>  color;
	for(int i=0; i<XF_CHANNELS(SRC_T,NPC); i++)
	{
		color.val[i]=_color[i];
	}
	assert((SRC_T == XF_8UC1) && "Type must be XF_8UC1");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) &&
			"NPC must be XF_NPPC1, XF_NPPC8");
	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");

#pragma HLS INLINE OFF

	xFpaintmaskKernel<SRC_T, MASK_T, ROWS,COLS,XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(MASK_T,NPC),(COLS>>XF_BITSHIFT(NPC))>
	(_src_mat, in_mask, _dst_mat, color, height, width);

}
}

#endif
