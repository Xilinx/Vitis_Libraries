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

#ifndef _XF_CROP_HPP_
#define _XF_CROP_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

typedef unsigned short  uint16_t;
typedef unsigned char  uchar;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{
/**
 * read_roi_row : reads the each row of ROI from DDR and  stores into BRAM.
 * Input       : _src_mat, offset, col_loop_cnt, end_loc, start_loc, left_over_pix
 * Output      : cnt, final_pix, pix_pos, temp
 */

template<int SRC_T, int ROWS, int COLS, int DEPTH, int NPC, int COLS_TRIP>
void read_roi_row(
		xf::Mat<SRC_T, ROWS, COLS, NPC>   &_src_mat,
		int                               offset,
		int                               col_loop_cnt,
		int                               &cnt,
		int                               end_loc,
		int                               start_loc,
		int                               &pix_pos,
		int                               left_over_pix,
		XF_TNAME(SRC_T,NPC)               &final_pix,
		XF_TNAME(SRC_T,NPC)               temp[COLS>>XF_BITSHIFT(NPC)]
) {

	// Local Variables
	XF_TNAME(SRC_T,NPC) pix=0;

	// Local Constants
	const int pix_width = XF_PIXELWIDTH(SRC_T, NPC);
	const int _npc_     = XF_NPIXPERCYCLE(NPC);

	// Initialization
	cnt=0;

	for(ap_uint<13> j = 0; j < col_loop_cnt; j++)
	{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS PIPELINE II=1

		pix=_src_mat.read(offset+j);

		int num_valid = end_loc-start_loc;
		int num_rem   = _npc_-pix_pos;

		if (num_rem > num_valid) {
			final_pix.range(pix_pos*pix_width + (num_valid*pix_width)-1, pix_pos*pix_width) = pix.range(end_loc*pix_width-1, start_loc*pix_width);
			pix_pos += num_valid;
		} else {
			final_pix.range(pix_pos*pix_width + (num_rem*pix_width)-1, pix_pos*pix_width) = pix.range((start_loc+num_rem)*pix_width-1, start_loc*pix_width);
			temp[cnt]=final_pix;
			cnt++;

			pix_pos=num_valid-num_rem;
			if(pix_pos!=0)
			final_pix.range(pix_pos*pix_width-1, 0) = pix.range(end_loc*pix_width-1, (start_loc+num_rem)*pix_width);
		}

		// Some reseting stuff...
		start_loc=0;
		if(j==(col_loop_cnt-2)) // For last column (column here refers to a unit of N-Pix per clock)
		{
			end_loc=left_over_pix;
		}
	}
}
/**
 * write_roi_row : writes the each row of ROI from BRAM to DDR.
 * Input       : temp, offset, write_offset, end_loc, start_loc, left_over_pix
 * Output      : _dst_mat
 */
template<int SRC_T,  int ROWS, int COLS,int DEPTH, int NPC,int COLS_TRIP>
void write_roi_row(int 											cnt,
				   XF_TNAME(SRC_T,NPC)  						temp[COLS>>XF_BITSHIFT(NPC)],
				   int 											write_offset,
				   xf::Mat<SRC_T, ROWS, COLS, NPC> 				&_dst_mat)
{
	for(int k=0;k<cnt;k++)
	{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS PIPELINE II=1
		_dst_mat.write(write_offset+k,temp[k]);
	}
}

/**
 * CROP kernel : crops the ROI of an input image and produces an output.
 * Input       : _src_mat, roi
 * Output      : _dst_mat
 */
template<int SRC_T,  int ROWS, int COLS,int DEPTH, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int COLS_TRIP>
void xFcropkernel_memorymapped(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat, xf::Rect_<unsigned int> &roi, unsigned short height, unsigned short width)
{
#pragma HLS INLINE

	// Ping-Pong buffer declaration:
	XF_TNAME(SRC_T,NPC) buf0[COLS>>XF_BITSHIFT(NPC)];
#pragma HLS RESOURCE variable=buf0 core=RAM_S2P_BRAM

	XF_TNAME(SRC_T,NPC) buf1[COLS>>XF_BITSHIFT(NPC)];
#pragma HLS RESOURCE variable=buf1 core=RAM_S2P_BRAM

	// Local constants
	const int _npc_ = XF_NPIXPERCYCLE(NPC);

	// For ROI bottom-right co-ordinates
	ap_uint<32> r=roi.y;
	ap_uint<32> r_new=roi.y+roi.height-1;
	ap_uint<32> c=roi.x ;
	ap_uint<32> c_new=roi.x+roi.width-1;

	// Temporary register for holding output column
	XF_TNAME(SRC_T,NPC) final_pix=0;

	// Local variables and their initial values
	int col_offset=0, write_offset=0, pix_pos=0, toggle_flag=1;
	int col_loop_cnt=0, cnt=0, start_loc=0, end_loc=0, offset;

	ap_uint<32> start_pix_loc=0; // Location of first pix in every row
	ap_uint<32> left_over_pix=0; // Left over pixels in each row after reading col_loop_cnt-1 columns

	// NPC will always in powers of 2
	// So c%NPC or c_new%NPC is just lest XF_BITSHIFT(NPC) bits
	if (XF_BITSHIFT(NPC)!=0) {
		start_pix_loc.range(XF_BITSHIFT(NPC)-1, 0)=c.range(XF_BITSHIFT(NPC)-1, 0);
		left_over_pix.range(XF_BITSHIFT(NPC)-1, 0)=c_new.range(XF_BITSHIFT(NPC)-1, 0);
	}
	start_loc = start_pix_loc; // Ranges from 0->NPC-1
	end_loc   = _npc_;         // Ranges from 1->NPC, this is done for easy loop-count/range calculations.

	col_offset   = (roi.x >> XF_BITSHIFT(NPC));
	col_loop_cnt = (c_new >> XF_BITSHIFT(NPC))-col_offset+1;

	if(left_over_pix==0)
	{
		left_over_pix=_npc_;
	} else {
		left_over_pix++;
	}

	offset = (r * width)+col_offset;
	read_roi_row<SRC_T, ROWS, COLS, DEPTH, NPC,COLS_TRIP>(_src_mat,offset,col_loop_cnt,cnt,end_loc,start_loc,pix_pos,left_over_pix,final_pix,buf0);

	rowLoop:
	for(ap_uint<32> i = 1; i < roi.height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		start_loc = start_pix_loc;
		end_loc   = _npc_;

		offset +=width;

		if(toggle_flag)
		{
			write_roi_row<SRC_T, ROWS, COLS, DEPTH, NPC,COLS_TRIP>(cnt,buf0,write_offset,_dst_mat);
			write_offset+=cnt;
			read_roi_row<SRC_T, ROWS, COLS, DEPTH, NPC,COLS_TRIP>(_src_mat,offset,col_loop_cnt,cnt,end_loc,start_loc,pix_pos,left_over_pix,final_pix,buf1);
		}
		else
		{
			write_roi_row<SRC_T, ROWS, COLS, DEPTH, NPC,COLS_TRIP>(cnt,buf1,write_offset,_dst_mat);
			write_offset+=cnt;
			read_roi_row<SRC_T, ROWS, COLS, DEPTH, NPC,COLS_TRIP>(_src_mat,offset,col_loop_cnt,cnt,end_loc,start_loc,pix_pos,left_over_pix,final_pix,buf0);
		}

		//toogle_flag = ~toggle_flag;
		toggle_flag = ((toggle_flag==0)?1:0);
	}
	// For last row
	if (roi.height==1) {
		write_roi_row<SRC_T, ROWS, COLS, DEPTH, NPC,COLS_TRIP>(cnt, buf0, write_offset, _dst_mat);
	} else {
		write_roi_row<SRC_T, ROWS, COLS, DEPTH, NPC,COLS_TRIP>(cnt, buf1, write_offset, _dst_mat);
	}

	if(pix_pos!=0)
	{
		_dst_mat.write(write_offset+cnt,final_pix);
	}
}
/**
 * xFcropkernel_stream kernel : crops the ROI of an input image and produces an output .
 * Input       : _src_mat, roi
 * Output      : _dst_mat
 */
template<int SRC_T, int ROWS, int COLS,int DEPTH, int NPC, int WORDWIDTH_SRC,
int WORDWIDTH_DST, int COLS_TRIP>
void xFcropkernel_stream(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat, xf::Rect_<unsigned int> &roi, unsigned short height, unsigned short width)
{
	XF_SNAME(WORDWIDTH_SRC) val_src=0;
	XF_SNAME(WORDWIDTH_DST) val_dst=0;
	XF_PTNAME(DEPTH)  pix_pos=0;

	// computing bottom right loaction of ROI using roi structure
	ap_uint<13> r=roi.x;
	ap_uint<13> r_new=roi.x+roi.height;
	ap_uint<13> c=roi.y ;
	ap_uint<13> c_new=(roi.y+roi.width);
	ap_uint<13> i, j, k;

	// intializing the local variables
	unsigned long long int idx=0;

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

			val_src = (XF_SNAME(WORDWIDTH_SRC)) (_src_mat.read(i*width+j));  //reading the source stream _src into val_src

			for(k = 0; k < NPC; k++)
			{
#pragma HLS unroll
				int col=(j*NPC)+k;

				if(((i>=r) && (i<r_new)) && ((col>=c) && (col<c_new)))
				{
					val_dst.range((pix_pos*XF_PIXELWIDTH(SRC_T,NPC)+(XF_PIXELWIDTH(SRC_T,NPC)-1)),pix_pos*XF_PIXELWIDTH(SRC_T,NPC)) = val_src.range((k*XF_PIXELWIDTH(SRC_T,NPC))+XF_PIXELWIDTH(SRC_T,NPC)-1,k*XF_PIXELWIDTH(SRC_T,NPC));
					pix_pos++;
					if(pix_pos==NPC)
					{
						_dst_mat.write(idx++, val_dst);
						 pix_pos=0;
					}
				}
			}
		}
	}

	if(pix_pos!=0)
	{
		_dst_mat.write(idx++,val_dst);
	}
}


#pragma SDS data zero_copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])

template<int SRC_T, int ROWS, int COLS,int ARCH_TYPE=0,int NPC=1>
void crop(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<SRC_T, ROWS, COLS, NPC>  &_dst_mat,xf::Rect_<unsigned int> &roi)
{

	unsigned short width = _src_mat.cols ;
	unsigned short height = _src_mat.rows;

	//assert((SRC_T == XF_8UC1) ||(SRC_T == XF_8UC3)  && "Type must be XF_8UC1 or XF_8UC3");
	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((roi.height <= height ) && (roi.width <= width)) && "ROI dimensions should be smaller or equal to the input image");
	assert(((roi.height > 0 ) && (roi.width > 0)) && "ROI dimensions should be greater than 0");
	assert(((roi.height + roi.y <= height ) && (roi.width + roi.x <= width)) && "ROI area exceeds the input image area");


	// Width in terms of NPPC
	width = _src_mat.cols >> XF_BITSHIFT(NPC);


#pragma HLS INLINE OFF

	if(ARCH_TYPE == XF_MEMORYMAPPED)
	{
		xFcropkernel_memorymapped<SRC_T,ROWS,COLS,XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(SRC_T,NPC),(COLS>>XF_BITSHIFT(NPC))> (_src_mat,_dst_mat,roi,height,width);
	}
	else
	{
		xFcropkernel_stream<SRC_T, ROWS,COLS,XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(SRC_T,NPC),(COLS>>XF_BITSHIFT(NPC))> (_src_mat,_dst_mat,roi,height,width);
	}

} //crop

}//xf namespace

#endif

