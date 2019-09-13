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

#ifndef _XF_PHASE_HPP_
#define _XF_PHASE_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

typedef unsigned short  uint16_t;

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "core/xf_math.h"

namespace xf{

// to convert the radians value to degrees
#define XF_NORM_FACTOR 	58671					// (180/PI)  in  Q6.10


/* xfPhaseKernel : The Gradient Phase Computation Kernel. This kernel takes
 * two gradients in AU_16SP depth and computes the angles for each pixel and
 * store this in a AU_16SP image.
 *  The Input arguments are _src1, _src2.
 *  _src1 --> Gradient X image from the output of sobel of depth AU_16SP.
 *  _src2 --> Gradient Y image from the output of sobel of depth AU_16SP.
 *  _dst  --> phase computed image of depth AU_16SP.
 *  Depending on NPC, 16 or 8 pixels are read and gradient values are calculated.
 */
template <int SRC_T, int DST_T, int ROWS, int COLS, int DEPTH_SRC,int DEPTH_DST, int NPC,
int WORDWIDTH_SRC, int WORDWIDTH_DST, int COLS_TRIP>
void xfPhaseKernel(
		xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,xf::Mat<DST_T, ROWS, COLS, NPC> & _src2,
		xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat,
		int _out_format,uint16_t &imgheight,uint16_t &imgwidth)
{
	int M1, N1, M2, N2; 	// Fixed point format of x and y, x = QM1.N1, y = QM2.N2
	M1 = 1;
	N1 = (XF_PIXELDEPTH(DEPTH_SRC)) - M1;
	M2 = M1;
	N2 = (XF_PIXELDEPTH(DEPTH_SRC)) - M2;

	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;

	int16_t p, q, ret = 0;
	int16_t result;
	int result_temp = 0;

	rowLoop:
	for(ap_uint<13> i = 0; i < (imgheight); i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(ap_uint<13> j = 0; j < (imgwidth); j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read(i*imgwidth+j));
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read(i*imgwidth+j));

			int proc_loop = XF_WORDDEPTH(WORDWIDTH_DST),
					step  = XF_PIXELDEPTH(DEPTH_DST);

			procLoop:
			for(ap_uint<9> k = 0; k < proc_loop; k += step)
			{
#pragma HLS unroll
				p = val_src1.range(k + (step - 1), k);		// Get bits from certain range of positions.
				q = val_src2.range(k + (step - 1), k);		// Get bits from certain range of positions.

				ret = xf::Atan2LookupFP(p, q, M1, N1, M2, N2);

				if(ret < 0)
				{
					result_temp = ret + XF_PI_FIXED + XF_PI_FIXED;
				}
				else if(ret == 0 && q < 0)
				{
					result_temp = ret + XF_PI_FIXED + XF_PI_FIXED;
				}
				else
				{
					result_temp = ret ;
				}
				if(_out_format == XF_DEGREES)
				{
					//result_temp = result_temp + 0x40;
					//result = (XF_NORM_FACTOR * result_temp)>>16;
					result = (XF_NORM_FACTOR * result_temp + 0x8000)>>16;
				}
				else if(_out_format == XF_RADIANS)
				{
					result =result_temp;
				}
				val_dst.range(k + (step - 1), k) = result;  //set the values in val_dst.
			} // end of proc loop
			_dst_mat.write(i*imgwidth+j,(val_dst));
		} // end of col loop
	} // end of row loop
}



/**
 * xfPhase: This function acts as a wrapper function and
 * calls the Kernel function.
 */
template <int ROWS, int COLS, int DEPTH_SRC,int DEPTH_DST, int NPC,
int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFPhaseComputation(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src1,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src2,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _dst,
		int _out_format,uint16_t imgheight,uint16_t imgwidth)
{


	imgwidth=imgwidth>>XF_BITSHIFT(NPC);

	xfPhaseKernel<ROWS,COLS,DEPTH_SRC,DEPTH_DST,NPC,WORDWIDTH_SRC,
	WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src1,_src2,_dst,_out_format,imgheight,imgwidth);



}
#pragma SDS data data_mover("_src_matx.data":FASTDMA,"_src_maty.data":FASTDMA,"_dst_mat.data":FASTDMA)
#pragma SDS data mem_attribute("_src_matx.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_src_maty.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src_matx.data":SEQUENTIAL, "_src_maty.data":SEQUENTIAL,"_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_matx.data"[0:"_src_matx.size"], "_src_maty.data"[0:"_src_maty.size"],"_dst_mat.data"[0:"_dst_mat.size"])

template<int RET_TYPE,int SRC_T,int DST_T, int ROWS, int COLS,int NPC>
void phase(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_matx,xf::Mat<DST_T, ROWS, COLS, NPC> & _src_maty,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat)
{
	assert(((_src_matx.rows <= ROWS ) && (_src_matx.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_src_maty.rows <= ROWS ) && (_src_maty.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_src_matx.rows == _src_maty.rows ) && (_src_matx.cols == _src_maty.cols)) && "Both input images should have same size");
	assert(((_src_matx.rows == _dst_mat.rows ) && (_src_matx.cols == _dst_mat.cols)) && "Input and output image should be of same size");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) && "NPC must be XF_NPPC1, XF_NPPC8 ");

	uint16_t imgwidth = _src_matx.cols >> XF_BITSHIFT(NPC);
	uint16_t imgheight = _src_matx.rows;


#pragma HLS INLINE OFF


	xfPhaseKernel<SRC_T, DST_T, ROWS,COLS,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(COLS>>XF_BITSHIFT(NPC))>(_src_matx,_src_maty,_dst_mat,RET_TYPE,imgheight,imgwidth);

}
}

#endif
