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


template<int SRC_T, int DST_T, int ROWS, int COLS,int ONE_D_HEIGHT, int ONE_D_WIDTH,int DEPTH, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int COLS_TRIP,int REDUCE_OP>
void xFreduceKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,  xf::Mat<DST_T, ONE_D_HEIGHT, ONE_D_WIDTH, 1> & _dst_mat, unsigned char dim, unsigned short height, unsigned short width)
{
	XF_SNAME(WORDWIDTH_SRC) val_src;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	unsigned long long int  p=0,q=0;
	unsigned char max=0;

	short int depth=XF_DTPIXELDEPTH(SRC_T,NPC)/XF_CHANNELS(SRC_T,NPC);

	XF_SNAME(WORDWIDTH_DST) internal_res;

	XF_SNAME(WORDWIDTH_DST) line_buf[(COLS >> XF_BITSHIFT(NPC))];
#pragma HLS RESOURCE variable=line_buf core=RAM_S2P_BRAM

if(dim==0)
{

	for(int i=0;i<(width >> XF_BITSHIFT(NPC));i++)
	{
#pragma HLS pipeline
			line_buf[i]=_src_mat.read(i);
	}
}




	ap_uint<13> i, j, k, planes;

	unsigned int var;
	if(dim==0)
	{
		 var=1;
	}
	else
	{
		 var=0;
	}
	rowLoop:
	for(i = var; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		if(REDUCE_OP ==REDUCE_MIN)
		{
			internal_res=-1;
			max=255;
		}
		else
		{
			internal_res=0;
			max=0;
		}
		colLoop:
		for(j = 0; j < width; j++)
		{

#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

				val_src = (XF_SNAME(WORDWIDTH_SRC)) (_src_mat.read(i*width+j));  //reading the source stream _src into val_src
				if(dim==0)
				{

					internal_res = line_buf[j];
				}


					switch (REDUCE_OP)
					{
					case REDUCE_SUM:
						internal_res=internal_res+val_src;
						break;
					case REDUCE_AVG:
						internal_res=internal_res+val_src;
						break;
					case REDUCE_MAX:
						internal_res=((XF_SNAME(WORDWIDTH_SRC))internal_res>val_src?(XF_SNAME(WORDWIDTH_SRC))internal_res:val_src);
						break;
					case REDUCE_MIN:
						internal_res=((XF_SNAME(WORDWIDTH_SRC))internal_res<val_src?(XF_SNAME(WORDWIDTH_SRC))internal_res:val_src);
						break;
					}
					if(dim==1&&j==width-1)
					{

						if(REDUCE_OP==REDUCE_AVG) {
							val_dst = internal_res/width;
						} else {
							val_dst = internal_res;
						}
					}
					if(dim==0)
					{
					val_dst=internal_res;
					line_buf[j]=val_dst;
					}



		}

		if(dim==1)
		{
			_dst_mat.write(q,val_dst);
			q++;
		}


	}
	if(dim==0)
	{
		for(unsigned int out=0; out<((width >> XF_BITSHIFT(NPC)));out++ )
		{
				if((REDUCE_OP==REDUCE_SUM))
				{
					_dst_mat.write(q,line_buf[out]);
					 q++;
				}
				else if (REDUCE_OP==REDUCE_AVG)
				{
					_dst_mat.write(q ,line_buf[out]/height);
					 q++;
				}
				else
				{

						_dst_mat.write(q, line_buf[out]);
						 q=q+1;
				}

		}
	}


}



#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])
//#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
//#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)

template< int REDUCE_OP, int SRC_T,int DST_T, int ROWS, int COLS,int ONE_D_HEIGHT, int ONE_D_WIDTH, int NPC=1>
void reduce(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,  xf::Mat<DST_T, ONE_D_HEIGHT, ONE_D_WIDTH, 1> & _dst_mat, unsigned char dim)
{
	unsigned short width = _src_mat.cols >> XF_BITSHIFT(NPC);
	unsigned short height = _src_mat.rows;

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) &&
			"NPC must be XF_NPPC1, XF_NPPC8");
	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");

#pragma HLS INLINE OFF


		xFreduceKernel<SRC_T, DST_T, ROWS,COLS,ONE_D_HEIGHT, ONE_D_WIDTH, XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC)),REDUCE_OP>(_src_mat, _dst_mat, dim, height, width);
	
}
}

#endif
