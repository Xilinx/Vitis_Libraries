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

#ifndef _XF_CANNY_HPP_
#define _XF_CANNY_HPP_

#ifndef __cplusplus
#error C++ is needed to use this file!
#endif


typedef unsigned short  uint16_t;
typedef unsigned char  uchar;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

#include "core/xf_math.h"
//#include "imgproc/xf_sobel.hpp"
#include "xf_canny_sobel.hpp"
#include "xf_averagegaussianmask.hpp"
#include "xf_magnitude.hpp"
#include "xf_canny_utils.hpp"


namespace xf{
/**
 *  xFDuplicate_rows
 */
template<int IN_T,int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int TC>
void xFDuplicate_rows(xf::Mat<IN_T, ROWS, COLS, NPC> & _src_mat,//hls::stream< XF_SNAME(WORDWIDTH) > &_src_mat,
		xf::Mat<IN_T, ROWS, COLS, NPC> &_src_mat1,
		xf::Mat<IN_T, ROWS, COLS, NPC> & _dst1_mat,
		xf::Mat<IN_T, ROWS, COLS, NPC> & _dst2_mat,
		xf::Mat<IN_T, ROWS, COLS, NPC> & _dst1_out_mat,
		xf::Mat<IN_T, ROWS, COLS, NPC> & _dst2_out_mat,
		uint16_t img_height, uint16_t img_width)
{
	img_width = img_width >> XF_BITSHIFT(NPC);

	ap_uint<13> row, col;
	Row_Loop:
	for(row = 0 ; row < img_height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		Col_Loop:
		for(col = 0; col < img_width; col++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			XF_SNAME(WORDWIDTH) tmp_src,tmp_src1;
			tmp_src1 = _src_mat1.read(row*img_width+col);
			tmp_src = _src_mat.read(row*img_width+col);
			_dst1_mat.write(row*img_width+col,tmp_src);
			_dst2_mat.write(row*img_width+col,tmp_src);
			_dst1_out_mat.write(row*img_width+col,tmp_src1);
			_dst2_out_mat.write(row*img_width+col,tmp_src1);
		}
	}
}

template<int SRC_T,int DST_T,int ROWS, int COLS, int DEPTH_SRC,int DEPTH_DST, int NPC,int NPC1, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFPackNMS(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,//hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src_mat,
			   xf::Mat<DST_T, ROWS, COLS, NPC1> & _dst_mat,//hls::stream< XF_SNAME(WORDWIDTH_DST)>& _dst_mat,
			   uint16_t imgheight,uint16_t imgwidth)
{
	const int num_clks_32pix = 32/NPC;
	int col_loop_count = (imgwidth/NPC);
	ap_uint<64> val;
	int read_ind=0,write_ind=0;
	rowLoop:
	for(int i = 0; i < (imgheight); i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(int j = 0; j < col_loop_count; j=j+(num_clks_32pix))
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS/32 max=COLS/32
#pragma HLS pipeline

			for(int k=0;k< num_clks_32pix ;k++)
			{
#pragma HLS UNROLL
				val.range(k*2*NPC+(NPC*2-1), k*2*NPC) = _src_mat.read(read_ind++);
			}
			_dst_mat.write(write_ind++,val);
		}
	}


}

// xFDuplicate_rows

template <int SRC_T,int DST_T,int NORM_TYPE,int ROWS, int COLS, int DEPTH_IN,int DEPTH_OUT, int NPC,int NPC1,
int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC,int TC1,int FILTER_TYPE,bool USE_URAM>
void xFCannyKernel(
		xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,
		xf::Mat<DST_T, ROWS, COLS, NPC1> & _dst_mat,
		unsigned char _lowthreshold, unsigned char _highthreshold,uint16_t img_height,uint16_t img_width)
{
#pragma HLS INLINE OFF
	if(NPC == 8)
	{
		 xf::Mat<XF_8UC1, ROWS, COLS, NPC> gaussian_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> gradx_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> gradx1_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> gradx2_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> grady_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> grady1_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> grady2_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> magnitude_mat(img_height,img_width);
		 xf::Mat<XF_8UC1, ROWS, COLS, NPC> phase_mat(img_height,img_width);
		 xf::Mat<XF_2UC1, ROWS, COLS, NPC> nms_mat(img_height,img_width);

#pragma HLS STREAM variable=gaussian_mat.data depth=2
#pragma HLS STREAM variable=gradx_mat.data depth=2
#pragma HLS STREAM variable=gradx1_mat.data depth=2
#pragma HLS STREAM variable=gradx2_mat.data depth=2
#pragma HLS STREAM variable=grady_mat.data depth=2
#pragma HLS STREAM variable=grady1_mat.data depth=2
#pragma HLS STREAM variable=grady2_mat.data depth=2
//#pragma HLS STREAM variable=magnitude_mat.data depth=2
//#pragma HLS STREAM variable=phase_mat.data depth=2
#pragma HLS STREAM variable=nms_mat.data depth=2

	
		if(NORM_TYPE == 1){
#pragma HLS STREAM variable=phase_mat.data depth=TC1
#pragma HLS STREAM variable=magnitude_mat.data depth=2
		}else{
#pragma HLS STREAM variable=phase_mat.data depth=2
#pragma HLS STREAM variable=magnitude_mat.data depth=TC1
		}

#pragma HLS DATAFLOW
		xFAverageGaussianMask3x3<SRC_T,SRC_T,ROWS,COLS,DEPTH_IN, NPC, WORDWIDTH_SRC,(COLS>>XF_BITSHIFT(NPC))>(_src_mat,gaussian_mat,img_height,img_width);
		xFSobel<SRC_T,XF_16SC1,ROWS,COLS,DEPTH_IN,XF_16SP,NPC,WORDWIDTH_SRC,XF_128UW,FILTER_TYPE,USE_URAM>(gaussian_mat,gradx_mat, grady_mat,XF_BORDER_REPLICATE, img_height, img_width);
		//Sobel<XF_BORDER_CONSTANT, FILTER_TYPE, SRC_T, XF_16SC1, ROWS, COLS, NPC,USE_URAM>(_src_mat, gradx_mat, grady_mat);
		xFDuplicate_rows<XF_16SC1,ROWS, COLS, XF_16SP, NPC, XF_128UW,TC>(gradx_mat,grady_mat,gradx1_mat,gradx2_mat, grady1_mat, grady2_mat,img_height,img_width);
		//xFMagnitude<ROWS, COLS, XF_16SP,XF_16SP, NPC, XF_128UW,XF_128UW>(gradx1_mat,grady1_mat,magnitude_mat,_norm_type,img_height,img_width);
		magnitude<NORM_TYPE,XF_16SC1,XF_16SC1,ROWS,COLS,NPC>(gradx1_mat,grady1_mat,magnitude_mat);
		xFAngle<XF_16SC1,XF_8UC1,ROWS, COLS, XF_16SP,XF_8UP, NPC, XF_128UW,XF_64UW>(gradx2_mat,grady2_mat,phase_mat,img_height,img_width);
		xFSuppression3x3<XF_16SC1,XF_8UC1,XF_2UC1,ROWS,COLS,XF_16SP,XF_8UP,DEPTH_OUT,NPC,XF_128UW,XF_64UW,XF_16UW,(COLS>>XF_BITSHIFT(NPC))>
		(magnitude_mat, phase_mat, nms_mat, _lowthreshold, _highthreshold, img_height, img_width);
		xFPackNMS<XF_2UC1,DST_T,ROWS,COLS,XF_2UP,DEPTH_OUT,NPC,NPC1,XF_16UW,WORDWIDTH_DST>(nms_mat,_dst_mat,img_height,img_width);
	}

	if(NPC == 1)
	{
		 xf::Mat<XF_8UC1, ROWS, COLS, NPC> gaussian_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> gradx_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> gradx1_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> gradx2_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> grady_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> grady1_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> grady2_mat(img_height,img_width);
		 xf::Mat<XF_16SC1, ROWS, COLS, NPC> magnitude_mat(img_height,img_width);
		 xf::Mat<XF_8UC1, ROWS, COLS, NPC> phase_mat(img_height,img_width);
		 xf::Mat<XF_2UC1, ROWS, COLS, NPC> nms_mat(img_height,img_width);



#pragma HLS STREAM variable=gaussian_mat.data depth=2
#pragma HLS STREAM variable=gradx_mat.data depth=2
#pragma HLS STREAM variable=gradx1_mat.data depth=2
#pragma HLS STREAM variable=gradx2_mat.data depth=2
#pragma HLS STREAM variable=grady_mat.data depth=2
#pragma HLS STREAM variable=grady1_mat.data depth=2
#pragma HLS STREAM variable=grady2_mat.data depth=2
//#pragma HLS STREAM variable=magnitude_mat.data depth=2
//#pragma HLS STREAM variable=phase_mat.data depth=2
#pragma HLS STREAM variable=nms_mat.data depth=2

		if(NORM_TYPE == 1){
#pragma HLS STREAM variable=phase_mat.data depth=TC1
#pragma HLS STREAM variable=magnitude_mat.data depth=2
		}else{
#pragma HLS STREAM variable=phase_mat.data depth=2
#pragma HLS STREAM variable=magnitude_mat.data depth=TC1
		}

#pragma HLS DATAFLOW

		xFAverageGaussianMask3x3<SRC_T,SRC_T,ROWS,COLS,DEPTH_IN, NPC, WORDWIDTH_SRC,(COLS>>XF_BITSHIFT(NPC))>(_src_mat,gaussian_mat,img_height,img_width);
		//Sobel<XF_BORDER_CONSTANT, FILTER_TYPE, SRC_T, XF_16SC1, ROWS, COLS, NPC,USE_URAM>(_src_mat, gradx_mat, grady_mat);
		xFSobel<SRC_T,XF_16SC1,ROWS,COLS,DEPTH_IN,XF_16SP,NPC,WORDWIDTH_SRC,XF_16UW,FILTER_TYPE,USE_URAM>(gaussian_mat,gradx_mat, grady_mat,XF_BORDER_REPLICATE, img_height, img_width);
		xFDuplicate_rows<XF_16SC1,ROWS, COLS, XF_16SP, NPC, XF_16UW,TC>(gradx_mat,grady_mat,gradx1_mat,gradx2_mat, grady1_mat, grady2_mat,img_height,img_width);
		//xFMagnitude<ROWS, COLS, XF_16SP,XF_16SP, NPC, XF_16UW,XF_16UW>(gradx1_mat,grady1_mat,magnitude_mat,_norm_type,img_height,img_width);
		magnitude<NORM_TYPE,XF_16SC1,XF_16SC1,ROWS,COLS,NPC>(gradx1_mat,grady1_mat,magnitude_mat);
		xFAngle<XF_16SC1,XF_8UC1,ROWS, COLS, XF_16SP,XF_8UP, NPC, XF_16UW,XF_8UW>(gradx2_mat,grady2_mat,phase_mat,img_height,img_width);
		xFSuppression3x3<XF_16SC1,XF_8UC1,XF_2UC1,ROWS,COLS,XF_16SP,XF_8UP,XF_2UP,NPC,XF_16UW,XF_8UW,XF_2UW,(COLS>>XF_BITSHIFT(NPC))>(magnitude_mat,phase_mat,nms_mat,_lowthreshold,_highthreshold,img_height,img_width);
		xFPackNMS<XF_2UC1,DST_T,ROWS,COLS,XF_2UP,DEPTH_OUT,NPC,NPC1,XF_2UW,WORDWIDTH_DST>(nms_mat,_dst_mat,img_height,img_width);
	}

}


/**********************************************************************
 * xFCanny :  Calls the Main Function depends on requirements
 **********************************************************************/
template<int ROWS, int COLS, int DEPTH_IN,int DEPTH_OUT, int NPC,
int WORDWIDTH_SRC, int WORDWIDTH_DST,int FILTER_TYPE,bool USE_URAM>
void xFCannyEdgeDetector(hls::stream< XF_SNAME(WORDWIDTH_SRC)>&   _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST)>& out_strm,
		unsigned char _lowthreshold, unsigned char _highthreshold,int _norm_type, uint16_t imgheight,uint16_t imgwidth)
{

	assert(((_norm_type == XF_L1NORM) || (_norm_type == XF_L2NORM)) &&
			"The _norm_type must be 'XF_L1NORM' or'XF_L2NORM'");


	xFCannyKernel<ROWS, COLS, DEPTH_IN, DEPTH_OUT, NPC, WORDWIDTH_SRC, WORDWIDTH_DST,
	(COLS>>XF_BITSHIFT(NPC)),((COLS>>XF_BITSHIFT(NPC))*3),FILTER_TYPE,USE_URAM>(_src_mat, out_strm, _lowthreshold, _highthreshold,_norm_type,imgheight,imgwidth);

}


#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])

template<int FILTER_TYPE,int NORM_TYPE,int SRC_T,int DST_T, int ROWS, int COLS,int NPC,int NPC1,bool USE_URAM=false>
void Canny(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<DST_T, ROWS, COLS, NPC1> & _dst_mat,unsigned char _lowthreshold,unsigned char _highthreshold)
{

#pragma HLS INLINE OFF
	assert(((NORM_TYPE == XF_L1NORM) || (NORM_TYPE == XF_L2NORM)) &&
			"The _norm_type must be 'XF_L1NORM' or'XF_L2NORM'");


	xFCannyKernel<SRC_T,DST_T,NORM_TYPE,ROWS, COLS, XF_DEPTH(SRC_T,NPC), XF_DEPTH(DST_T,NPC1), NPC,NPC1, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(DST_T,NPC1),
			(COLS>>XF_BITSHIFT(NPC)),((COLS>>XF_BITSHIFT(NPC))*3),FILTER_TYPE,USE_URAM>(_src_mat, _dst_mat, _lowthreshold, _highthreshold,_src_mat.rows,_src_mat.cols);


}
}

#endif
