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

#ifndef _XF_UTILITY_H_
#define _XF_UTILITY_H_

#include <string.h>
#include <assert.h>
#include "xf_common.h"


/**
 * Extract Pixels from a packed word into an array from the index pos.
 * The number of pixels to be extracted is determined by the NPC.
 */


template<int NPC, int WORDWIDTH, int PIXELDEPTH>
void xfPackPixels(XF_PTNAME(PIXELDEPTH)* tmp_buf, XF_SNAME(WORDWIDTH) &val, uint16_t pos, int16_t loopIter, uint16_t &shift)
{
#pragma HLS INLINE
	ap_uint<8> STEP = XF_PIXELDEPTH(PIXELDEPTH);

	for(ap_int<9> i = 0; i < loopIter; i++)
	{
#pragma HLS unroll
		XF_PTUNAME(PIXELDEPTH) tmp =  tmp_buf[pos];
		val = val | (((XF_SNAME(WORDWIDTH)) tmp) << (shift*STEP));
		pos++;
		shift++;
	}
}



template<int NPC, int WORDWIDTH, int PIXELDEPTH>
void xfExtractPixels(XF_PTNAME(PIXELDEPTH)* tmp_buf, XF_SNAME(WORDWIDTH) &val1, int pos)
{
#pragma HLS inline off
	XF_SNAME(WORDWIDTH) v = val1;
	
	int shift = 0;
	int STEP = XF_PIXELDEPTH(PIXELDEPTH);
	Extract_pixels_loop:
	for (int i = 0; i < (1<<(XF_BITSHIFT(NPC))); i++)
	{
#pragma HLS UNROLL
		tmp_buf[pos+i] = v.range(shift+STEP-1, shift);
		shift = shift + STEP;
	}
}

template<int NPC, int WORDWIDTH_SRC, int DEPTH_SRC>
void xfExtractData(XF_PTNAME(DEPTH_SRC) *src_buf1, XF_PTNAME(DEPTH_SRC) *src_buf2,
		XF_PTNAME(DEPTH_SRC) *src_buf3, XF_PTNAME(DEPTH_SRC) *src_buf4,
		XF_PTNAME(DEPTH_SRC) *src_buf5, XF_PTNAME(DEPTH_SRC) *src_buf6,
		XF_PTNAME(DEPTH_SRC) *src_buf7, XF_SNAME(WORDWIDTH_SRC) buf0, XF_SNAME(WORDWIDTH_SRC) buf1, XF_SNAME(WORDWIDTH_SRC) buf2,
		XF_SNAME(WORDWIDTH_SRC) buf3, XF_SNAME(WORDWIDTH_SRC) buf4, XF_SNAME(WORDWIDTH_SRC) buf5, XF_SNAME(WORDWIDTH_SRC) buf6)
{
#pragma HLS INLINE
	xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf1[6], buf0, 0);
	xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf2[6], buf1, 0);
	xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf3[6], buf2, 0);
	xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf4[6], buf3, 0);
	xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf5[6], buf4, 0);
	xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf6[6], buf5, 0);
	xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf7[6], buf6, 0);
}


template<int NPC, int DEPTH_SRC>
void xfCopyData(XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf2[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf3[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf4[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf5[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf6[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf7[XF_NPIXPERCYCLE(NPC)+6]){
#pragma HLS INLINE
	ap_uint<5> buf_size = (XF_NPIXPERCYCLE(NPC)+6);
	ap_uint<4> i = 0;
	ap_uint<4> ind = buf_size-6;
	for(i = 0; i < 6; i++, ind++)
	{
#pragma HLS LOOP_TRIPCOUNT min=6 max=6
#pragma HLS unroll
		src_buf1[i] = src_buf1[ind];
		src_buf2[i] = src_buf2[ind];
		src_buf3[i] = src_buf3[ind];
		src_buf4[i] = src_buf4[ind];
		src_buf5[i] = src_buf5[ind];
		src_buf6[i] = src_buf6[ind];
		src_buf7[i] = src_buf7[ind];
	}
}


/****************************************************************
 *  auReadToArray: Read the input data from the DDR and writes
 *  	it into the array
 ***************************************************************/
//template<int TC_1, typename IN_T_1, typename OUT_T_1>
//void auReadToArray(IN_T_1* _in_ptr, OUT_T_1* _out_arr, uint16_t n)
//{
//	readToArrayLoop:
//	for(uint16_t i = 0; i < n; i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=TC_1 max=TC_1 avg=TC_1
//#pragma HLS PIPELINE
//		_out_arr[i] = (OUT_T_1)_in_ptr[i];
//	}
//}
/**
 * CopyMemoryIn: Copies memory from DDR to BRAM
 */
//template<int SIZE, int WORDWIDTH>
//void auCopyMemoryIn(AU_TNAME(WORDWIDTH)* _src, AU_TNAME(WORDWIDTH)* _dst, int _src_offset, int nbytes)
//{
//#if _AU_SYNTHESIS_
//	memcpy((AU_TNAME(WORDWIDTH)*)_dst, (AU_TNAME(WORDWIDTH)*)_src + _src_offset, SIZE);
//#else
//	memcpy((AU_TNAME(WORDWIDTH)*)_dst, (AU_TNAME(WORDWIDTH)*)_src + _src_offset, nbytes);
//#endif
//}


/**
 * CopyMemoryOut: Copies memory from BRAM to DDR
 */
//template<int SIZE, int WORDWIDTH>
//void auCopyMemoryOut(AU_TNAME(WORDWIDTH)* _src, AU_TNAME(WORDWIDTH)* _dst, int _dst_offset, int nbytes)
//{
//#if _AU_SYNTHESIS_
//	memcpy((AU_TNAME(WORDWIDTH)*)_dst + _dst_offset, (AU_TNAME(WORDWIDTH)*)_src , SIZE);
//#else
//	memcpy((AU_TNAME(WORDWIDTH)*)_dst + _dst_offset, (AU_TNAME(WORDWIDTH)*)_src , nbytes);
//#endif
//
//
//}


/**
 * 	WriteIntoMat:
 */
//template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH,int SIZE>
//void auWriteIntoMat(AU_TNAME(WORDWIDTH)* _src, auviz::Mat<ROWS,COLS,DEPTH,NPC,WORDWIDTH>& _dst)
//{
//#if _AU_SYNTHESIS_
//	WR_STRM:
//	for(int j = 0; j < SIZE; ++j)
//	{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=SIZE max=SIZE
//		_dst.write(_src[j]);
//	}
//#else
//	WR_STRM:
//	for(int j = 0; j < _dst.cols; ++j)
//	{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=SIZE max=SIZE
//		_dst.write(_src[j]);
//	}
//#endif
//}


/**
 * 	ReadFromMat:
 */
//template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH,int SIZE>
//void auReadFromMat(auviz::Mat<ROWS, COLS, DEPTH, NPC, WORDWIDTH>& _src, AU_TNAME(WORDWIDTH)* _dst)
//{
//#if _AU_SYNTHESIS_
//	RD_STRM:
//	for(int j = 0; j < SIZE; ++j)
//	{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=SIZE max=SIZE
//		_dst[j] = _src.read();
//	}
//#else
//	RD_STRM:
//	for(int j = 0; j < _src.cols; ++j)
//	{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=SIZE max=SIZE
//		_dst[j] = _src.read();
//	}
//#endif
//}
//
//template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH>
//void auReadImage(AU_TNAME(WORDWIDTH)* in,
//		auviz::Mat<ROWS, COLS, DEPTH, NPC, WORDWIDTH>& out_mat)
//{
//	assert(((((DEPTH == AU_8UP) || (DEPTH == AU_8SP)) && (NPC == AU_NPPC16) && (WORDWIDTH == AU_128UW)) ||
//			(((DEPTH == AU_8UP) || (DEPTH == AU_8SP)) && (NPC == AU_NPPC8)  && (WORDWIDTH == AU_64UW))  ||
//			(((DEPTH == AU_8UP) || (DEPTH == AU_8SP)) && (NPC == AU_NPPC1)  && (WORDWIDTH == AU_8UW))   ||
//			(((DEPTH == AU_16SP) || (DEPTH == AU_16UP)) && (NPC == AU_NPPC16) && (WORDWIDTH == AU_256UW)) ||
//			(((DEPTH == AU_16SP) || (DEPTH == AU_16UP)) && (NPC == AU_NPPC8)  && (WORDWIDTH == AU_128UW)) ||
//			(((DEPTH == AU_16SP) || (DEPTH == AU_16UP)) && (NPC == AU_NPPC1)  && (WORDWIDTH == AU_16UW))  ||
//			(((DEPTH == AU_32SP) || (DEPTH == AU_32UP)) && (NPC == AU_NPPC16) && (WORDWIDTH == AU_512UW)) ||
//			(((DEPTH == AU_32SP) || (DEPTH == AU_32UP)) && (NPC == AU_NPPC8)  && (WORDWIDTH == AU_256UW)) ||
//			(((DEPTH == AU_32SP) || (DEPTH == AU_32UP)) && (NPC == AU_NPPC1)  && (WORDWIDTH == AU_32UW)))
//			&& "Incorrect Combination of NPC, DEPTH and WORDWIDTH ");
//
//	AU_TNAME(WORDWIDTH) lbuf1[IM_WIDTH(COLS,NPC)];
//	AU_TNAME(WORDWIDTH) lbuf2[IM_WIDTH(COLS,NPC)];
//
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = out_mat.cols * nppc*(AU_PIXELDEPTH(DEPTH)>>3);
//	int offset = 0;
//
//	bool flag = true;
//
//	RD_IMG:
//	for(int i = 0; i < out_mat.rows; i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//
//
//		auCopyMemoryIn<COLS*(AU_PIXELDEPTH(DEPTH)>>3),WORDWIDTH>(in, lbuf1, offset, wordsize);
//		auWriteIntoMat<ROWS,COLS,DEPTH,NPC,WORDWIDTH,(COLS>>NPC)>(lbuf1, out_mat);
//		flag = false;
//		offset = offset + out_mat.cols;
//
//	}
//
//}
//
//
//template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH>
//void auWriteImage(auviz::Mat<ROWS, COLS, DEPTH, NPC, WORDWIDTH>& in_mat,
//		AU_TNAME(WORDWIDTH)* out)
//{
//	assert(((((DEPTH == AU_8UP) || (DEPTH == AU_8SP)) && (NPC == AU_NPPC16) && (WORDWIDTH == AU_128UW)) ||
//			(((DEPTH == AU_8UP) || (DEPTH == AU_8SP)) && (NPC == AU_NPPC8)  && (WORDWIDTH == AU_64UW))  ||
//			(((DEPTH == AU_8UP) || (DEPTH == AU_8SP)) && (NPC == AU_NPPC1)  && (WORDWIDTH == AU_8UW))   ||
//			(((DEPTH == AU_16SP) || (DEPTH == AU_16UP)) && (NPC == AU_NPPC16) && (WORDWIDTH == AU_256UW)) ||
//			(((DEPTH == AU_16SP) || (DEPTH == AU_16UP)) && (NPC == AU_NPPC8)  && (WORDWIDTH == AU_128UW)) ||
//			(((DEPTH == AU_16SP) || (DEPTH == AU_16UP)) && (NPC == AU_NPPC1)  && (WORDWIDTH == AU_16UW))  ||
//			(((DEPTH == AU_32SP) || (DEPTH == AU_32UP)) && (NPC == AU_NPPC16) && (WORDWIDTH == AU_512UW)) ||
//			(((DEPTH == AU_32SP) || (DEPTH == AU_32UP)) && (NPC == AU_NPPC8)  && (WORDWIDTH == AU_256UW)) ||
//			(((DEPTH == AU_32SP) || (DEPTH == AU_32UP)) && (NPC == AU_NPPC1)  && (WORDWIDTH == AU_32UW)))
//			&& "Incorrect Combination of NPC, DEPTH and WORDWIDTH");
//
//	AU_TNAME(WORDWIDTH) lbuf1[IM_WIDTH(COLS,NPC)];
//	AU_TNAME(WORDWIDTH) lbuf2[IM_WIDTH(COLS,NPC)];
//
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = in_mat.cols * nppc * (AU_PIXELDEPTH(DEPTH)>>3);
//
//	bool flag = true;
//
//
//	int i, offset = 0;
//	WR_IMG:
//	for(i = 0; i < in_mat.rows; i++, offset += (in_mat.cols))
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//
//		auReadFromMat<ROWS,COLS,DEPTH,NPC,WORDWIDTH,(COLS>>NPC)>(in_mat,lbuf1);
//		auCopyMemoryOut<COLS*(AU_PIXELDEPTH(DEPTH)>>3),WORDWIDTH>(lbuf1, out, offset, wordsize);
//
//
//	}
//
//}

//
//
//template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH>
//void auCopyMat(auviz::Mat<ROWS, COLS, DEPTH, NPC, WORDWIDTH>& _dst,
//		auviz::Mat<ROWS, COLS, DEPTH, NPC, WORDWIDTH>& _src)
//{
//	assert((_src.rows == _dst.rows) && (_src.cols == _dst.cols) && "Source and Destination dimensions should be equal.");
//	for (int i = 0; i < _src.rows; i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		for(int j = 0; j < _dst.cols; j++)
//		{
//			AU_TNAME(WORDWIDTH) val;
//			val = _src.read();
//			_dst.write(val);
//		}
//	}
//}


//template<int NPC, int WORDWIDTH, int MAXPTS>
//void auWriteList(auviz::List<NPC,WORDWIDTH,MAXPTS>& list, AU_TNAME(WORDWIDTH) *OUT)
//{
//	AU_TNAME(WORDWIDTH) lbuf[MAXPTS+1];
//	AU_TNAME(WORDWIDTH) val;
//	int wordsize;
//#if _AU_SYNTHESIS_
//	int N = MAXPTS;
//#else
//	int N = list.size();
//#endif
//
//	int nbytes = AU_WORDDEPTH(WORDWIDTH)>>3;
//	wordsize = N*nbytes;
//
//	uint16_t size = list.sz;
//	ap_uint32_t tmp;
//	tmp.range(15,0) = size;
//	tmp.range(31,16) = size;
//	lbuf[0] = tmp;
//
//	for(int i = 0; i < list.size(); i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=MAXPTS max=MAXPTS
//		val = list.read();
//		lbuf[i+1] = val;
//	}
//	memcpy((AU_TNAME(WORDWIDTH)*)OUT, lbuf, wordsize );
//}


/**
 * CopyMemoryOut: Copies memory from BRAM to DDR
 */
template<int SIZE, int WORDWIDTH>
void xFCopyBlockMemoryOut1(XF_SNAME(WORDWIDTH)* _src, unsigned long long int *_dst, int nbytes)
{
#if _XF_SYNTHESIS_
    memcpy((unsigned long long int*)_dst, (unsigned long long int*)_src, SIZE);
#else
    if(nbytes)
    memcpy((unsigned long long int*)_dst, (unsigned long long int*)_src, nbytes);
#endif
}

/**
 * CopyMemoryIn: Copies memory from DDR to BRAM if y_offset and x_offset is provided
 */
template<int SIZE, int WORDWIDTH>
void xFCopyBlockMemoryIn1(unsigned long long int* _src, XF_SNAME(WORDWIDTH)* _dst, int nbytes)
{
#if _XF_SYNTHESIS_
    memcpy((XF_SNAME(WORDWIDTH)*)_dst, (XF_SNAME(WORDWIDTH)*)_src, SIZE);
#else
    memcpy((XF_SNAME(WORDWIDTH)*)_dst, (XF_SNAME(WORDWIDTH)*)_src, nbytes);
#endif
}

/**
 * CopyMemoryIn: Copies memory from DDR to BRAM if y_offset and x_offset is provided
 */
template<int SIZE, int WORDWIDTH>
void auCopyBlockMemoryIn(XF_SNAME(WORDWIDTH)* _src, XF_SNAME(WORDWIDTH)* _dst, int nbytes)
{
#if _AU_SYNTHESIS_
	memcpy((AU_TNAME(WORDWIDTH)*)_dst, (AU_TNAME(WORDWIDTH)*)_src, SIZE);
#else
	memcpy((XF_SNAME(WORDWIDTH)*)_dst, (XF_SNAME(WORDWIDTH)*)_src, nbytes);
#endif
}

/**
 * CopyMemoryOut: Copies memory from BRAM to DDR
 */
template<int SIZE, int WORDWIDTH>
void auCopyBlockMemoryOut(XF_SNAME(WORDWIDTH)* _src, XF_SNAME(WORDWIDTH)* _dst, int nbytes)
{
#if _AU_SYNTHESIS_
	memcpy((XF_SNAME(WORDWIDTH)*)_dst, (XF_SNAME(WORDWIDTH)*)_src, SIZE);
#else
	memcpy((XF_SNAME(WORDWIDTH)*)_dst, (XF_SNAME(WORDWIDTH)*)_src, nbytes);
#endif
}

//
//template<int NPC, int WORDWIDTH,int MAXPTS>
//void auWriteIntoList(AU_TNAME(WORDWIDTH)* _src, auviz::List<NPC,WORDWIDTH,MAXPTS>& _dst)
//{
//	WR_STRM:
//	for(int j = 0; j < _dst.size(); ++j)
//	{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=MAXPTS max=MAXPTS
//		_dst.write(_src[j]);
//	}
//}
//
///*
// * Read list of points from DDR to List object.
// */
//
//template<int NPC, int WORDWIDTH, int MAXPTS>
//void auReadList(AU_TNAME(WORDWIDTH) *SRC_PTR,auviz::List<NPC,WORDWIDTH,MAXPTS>& list)
//{
//	AU_TNAME(WORDWIDTH) lbuf[MAXPTS];
//	AU_TNAME(WORDWIDTH) val;
//	int wordsize;
//#if _AU_SYNTHESIS_
//	int N = MAXPTS;
//#else
//	int N = list.sz;
//#endif
//	int nbytes = AU_WORDDEPTH(WORDWIDTH)>>3;
//	wordsize = N*nbytes;
//
//	auCopyMemoryIn<MAXPTS,WORDWIDTH>(SRC_PTR,lbuf,0,wordsize);
//	auWriteIntoList<NPC,WORDWIDTH,MAXPTS>(lbuf,list);
//
//}
//
//template<int BLOCKROWS,int BLOCKCOLS,int NPC,int IN_WW>
//void auFetchData(AU_TNAME(IN_WW) *lbuf,unsigned char *block_data)
//{
//
//	data_fetch_loop:
//	int elem_cnt = 0;
//	for (int l = 0; l<((BLOCKCOLS>>NPC)+1);l++ ){
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=7 max=7
//
//		AU_TNAME(IN_WW) tmp = lbuf[l];
//		data_fetch_loop00:
//		for (int j = 0; j<(1<<NPC);j++ )
//		{
//#pragma HLS UNROLL
//			int shift = j<<3;
//			block_data[elem_cnt+j] = tmp.range(shift+7,shift) ;
//		}
//		elem_cnt = elem_cnt+8;
//	}
//
//}
/*
 * Reads a rectangular block (of size BLOCKROWSxBLOCKCOLS) from DDR with
 * (x,y) as its center and writes it into a Mat object.
 */

//template<int ROWS,int COLS,int BLOCKROWS,int BLOCKCOLS,int DEPTH,int NPC,int IN_WW>
//void auReadBlock(AU_TNAME(IN_WW) *in,int imwidth,int imheight,int x,int y,auviz::Mat<BLOCKROWS,BLOCKCOLS,DEPTH,NPC,IN_WW> &block){
//
//	int block_start,sx1;
//	unsigned char block_data[BLOCKCOLS+8];
//#pragma HLS ARRAY_PARTITION variable=block_data complete dim=1
//
//
//	AU_TNAME(IN_WW) linebuf1[(BLOCKCOLS>>NPC)+1],linebuf2[(BLOCKCOLS>>NPC)+1];
//
//	int i,j,k,l,m;
//
//	int sx,sy;
//	sx = x - (BLOCKCOLS>>1);
//	sy = y - (BLOCKROWS>>1);
//
//	block_start = sy*(imwidth>>NPC) + (sx>>3);
//	sx1 = sx&7;
//
//	int elem_cnt = 0;
//	unsigned char flag = 0;
//	int  cnt = 0;
//
//	auCopyMemoryIn<(BLOCKCOLS+8)*(AU_PIXELDEPTH(DEPTH)>>3),IN_WW>(in,linebuf1,block_start,((BLOCKCOLS>>NPC)+1)*8);
//	block_start += (imwidth>>NPC);
//
//	dr_row_loop:
//	for (i=1;i<(BLOCKROWS);i++)
//	{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=BLOCKROWS max=BLOCKROWS
//		elem_cnt = 0;
//
//		if (flag == 0){
//
//			auCopyMemoryIn<(BLOCKCOLS+8)*(AU_PIXELDEPTH(DEPTH)>>3),IN_WW>(in,linebuf2,block_start,((BLOCKCOLS>>NPC)+1)*8);
//			auFetchData<BLOCKROWS,BLOCKCOLS,NPC,IN_WW>(linebuf1,block_data);
//
//			flag = 1;
//		}
//		else{
//
//			auCopyMemoryIn<(BLOCKCOLS+8)*(AU_PIXELDEPTH(DEPTH)>>3),IN_WW>(in,linebuf1,block_start,((BLOCKCOLS>>NPC)+1)*8);
//			auFetchData<BLOCKROWS,BLOCKCOLS,NPC,IN_WW>(linebuf2,block_data);
//
//			flag = 0;
//		}
//		block_start = block_start+(imwidth>>NPC);
//		m = sx1;
//		block_strm_write_loop0:
//		for(k=0;k<(BLOCKCOLS>>NPC);k++)
//		{
//#pragma HLS LOOP_TRIPCOUNT min=6 max=6
//#pragma HLS pipeline
//			AU_TNAME(IN_WW) tmp_val = 0;
//			block_strm_write_loop00:for (j=0;j<(1<<NPC);j++){
//#pragma HLS UNROLL
//#pragma HLS LOOP_TRIPCOUNT min=8 max=8
//				l = j << 3;
//				tmp_val.range(l+(1<<NPC)-1,l) = block_data[m+j];
//			}
//			block.write(tmp_val);
//			m = m+8;
//		}
//		cnt = cnt+(BLOCKCOLS>>NPC);
//	}
//	elem_cnt = 0;
//
//
//	auFetchData<BLOCKROWS,BLOCKCOLS,NPC,IN_WW>(linebuf2,block_data);
//
//
//	m = sx1;
//	block_strm_write_loop1:
//	for(k=0;k<(BLOCKCOLS>>NPC);k++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=6 max=6
//#pragma HLS pipeline
//		AU_TNAME(IN_WW) tmp_val = 0;
//		block_strm_write_loop10:
//		for (j=0;j<(1<<NPC);j++)
//		{
//#pragma HLS UNROLL
//#pragma HLS LOOP_TRIPCOUNT min=8 max=8
//			l = j << 3;
//			tmp_val.range(l+(1<<NPC)-1,l) = block_data[m+j];
//
//		}
//
//		block.write(tmp_val);
//		m = m+(1<<NPC);
//	}
//
//}

template<int WORDWIDTH,int NPC,int IN_BH,int IN_BW>
void xFDuplicateStream(hls::stream< XF_SNAME(WORDWIDTH) > &in_strm,
					   hls::stream< XF_SNAME(WORDWIDTH) > &out_strm1,
					   hls::stream< XF_SNAME(WORDWIDTH) > &out_strm2,
					   int imwidth,int imheight)
{
	for (int i=0;i<imheight;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=IN_BH max=IN_BH
#pragma HLS LOOP_FLATTEN off
		for (int j=0;j<(imwidth>>NPC);j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=IN_BW max=IN_BW
			XF_SNAME(WORDWIDTH) tmp = in_strm.read();
			out_strm1.write(tmp);
			out_strm2.write(tmp);
		}
	}
}

namespace xf {
	class accel_utils {
public:
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 ***********************    READ MODULE     ****************************************
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
template <int ptr_width, int ROWS, int COLS, int NPC, int COLOR_T, int CH_WIDTH, int TRIPCOUNT>
void Array2hlsStrm(ap_uint<ptr_width> *srcPtr, hls::stream<ap_uint<ptr_width> > &dstStrm, int rows, int cols)
{
	int pixel_width = COLOR_T*CH_WIDTH;
	int loop_count = (((rows*cols*pixel_width)+ptr_width-1)/ptr_width);

	for (int i=0; i<loop_count; i++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=TRIPCOUNT
#pragma HLS PIPELINE
		dstStrm.write(srcPtr[i]);
	}
}

template <int ptr_width, int MAT_T, int ROWS, int COLS, int NPC, int TRIPCOUNT>
void hlsStrm2xfMat(hls::stream<ap_uint<ptr_width> > &srcStrm, xf::Mat<MAT_T,ROWS,COLS,NPC>& dstMat)
{
	int rows = dstMat.rows;
	int cols = dstMat.cols;
	int loop_count = (rows*cols)/XF_NPIXPERCYCLE(NPC);

	int valid_bits = 0;
	const int N_size = XF_PIXELWIDTH(MAT_T,NPC) * XF_NPIXPERCYCLE(NPC);
	ap_uint<ptr_width> r;
	XF_TNAME(MAT_T,NPC) out;

	L1:for (int i=0; i<loop_count; i++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=TRIPCOUNT
#pragma HLS PIPELINE

		if (valid_bits < N_size)
		{
			if (valid_bits !=0)
			{
				out.range(valid_bits-1, 0) = r.range(ptr_width-1, ptr_width-valid_bits);
			}
			r = srcStrm.read();
			out.range(N_size-1, valid_bits) = r.range(N_size-valid_bits-1,0);
			valid_bits = ptr_width - (N_size - valid_bits);
		}
		else
		{
			out = r.range(ptr_width - valid_bits + N_size -1, ptr_width-valid_bits);
			valid_bits -= N_size;
		}
		dstMat.write(i,out);
	}
}

template <int ptr_width, int MAT_T, int ROWS, int COLS, int NPC>
void Array2xfMat(ap_uint<ptr_width> *srcPtr, xf::Mat<MAT_T,ROWS,COLS,NPC>& dstMat)
{
#pragma HLS DATAFLOW
	assert((ptr_width >= XF_WORDDEPTH(XF_WORDWIDTH(MAT_T,NPC))) && "The ptr_width must be always greater than or equal to the minimum width for the corresponding configuration");
	const int ch_width = XF_DTPIXELDEPTH(MAT_T,NPC);

	hls::stream<ap_uint<ptr_width> > strm;
	int rows = dstMat.rows;
	int cols = dstMat.cols;
	Array2hlsStrm<ptr_width,ROWS,COLS,NPC,XF_CHANNELS(MAT_T,NPC),ch_width,((ROWS*COLS*XF_CHANNELS(MAT_T,NPC)*ch_width)/ptr_width)>(srcPtr,strm,rows,cols);
	hlsStrm2xfMat<ptr_width,MAT_T,ROWS,COLS,NPC,(ROWS*COLS)/NPC>(strm,dstMat);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 ***********************    WRITE MODULE     ****************************************
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
template <int ptr_width, int MAT_T, int ROWS, int COLS, int NPC, int TRIPCOUNT>
void xfMat2hlsStrm(xf::Mat<MAT_T,ROWS,COLS,NPC>& srcMat, hls::stream<ap_uint<ptr_width> > &dstStrm)
{
	int rows = srcMat.rows;
	int cols = srcMat.cols;
	int loop_count = (rows*cols)/XF_NPIXPERCYCLE(NPC);

	int bits_to_add = ptr_width;
	const int N_size = XF_PIXELWIDTH(MAT_T,NPC) * XF_NPIXPERCYCLE(NPC);
	ap_uint<ptr_width> r;
	XF_TNAME(MAT_T,NPC) in;

	L1:for (int i=0; i<loop_count; i++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=TRIPCOUNT
#pragma HLS PIPELINE

		in = srcMat.read(i);

		if (bits_to_add <= N_size)
		{
			r.range(ptr_width-1,ptr_width-bits_to_add) = in.range(bits_to_add-1, 0);
			dstStrm.write(r);
			
			if (bits_to_add != N_size)
			{
				r.range(N_size-bits_to_add-1, 0) = in.range(N_size-1,bits_to_add);
			}			
			bits_to_add = ptr_width - (N_size - bits_to_add);
		}
		else
		{
			r.range(ptr_width - bits_to_add + N_size -1, ptr_width-bits_to_add) = in;
			bits_to_add -= N_size;
		}
	}

	if (bits_to_add != ptr_width)
	{
		dstStrm.write(r);
	}
}

template <int ptr_width, int ROWS, int COLS, int NPC, int COLOR_T, int CH_WIDTH, int TRIPCOUNT>
void hlsStrm2Array(hls::stream<ap_uint<ptr_width> > &srcStrm, ap_uint<ptr_width> *dstPtr, int rows, int cols)
{
	int pixel_width = COLOR_T*CH_WIDTH;
	int loop_count = (((rows*cols*pixel_width)+ptr_width-1)/ptr_width);

	for (int i=0; i<loop_count; i++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=TRIPCOUNT
#pragma HLS PIPELINE
		dstPtr[i] = srcStrm.read();
	}
}

template <int ptr_width, int MAT_T, int ROWS, int COLS, int NPC>
void xfMat2Array(xf::Mat<MAT_T,ROWS,COLS,NPC>& srcMat, ap_uint<ptr_width> *dstPtr)
{
#pragma HLS DATAFLOW
	assert((ptr_width >= XF_WORDDEPTH(XF_WORDWIDTH(MAT_T,NPC))) && "The ptr_width must be always greater than or equal to the minimum width for the corresponding configuration");
	const int ch_width = XF_DTPIXELDEPTH(MAT_T,NPC);

	hls::stream<ap_uint<ptr_width> > strm;
	int rows = srcMat.rows;
	int cols = srcMat.cols;

	xfMat2hlsStrm<ptr_width,MAT_T,ROWS,COLS,NPC,(ROWS*COLS)/NPC>(srcMat,strm);
	hlsStrm2Array<ptr_width,ROWS,COLS,NPC,XF_CHANNELS(MAT_T,NPC),ch_width,((ROWS*COLS*XF_CHANNELS(MAT_T,NPC)*ch_width)/ptr_width)>(strm,dstPtr,rows,cols);
}
};
template <int ptr_width, int MAT_T, int ROWS, int COLS, int NPC>
void xfMat2Array(xf::Mat<MAT_T,ROWS,COLS,NPC>& srcMat, ap_uint<ptr_width> *dstPtr)
{
   accel_utils au;
   au.xfMat2Array<ptr_width, MAT_T, ROWS, COLS, NPC>(srcMat, dstPtr);
}

template <int ptr_width, int MAT_T, int ROWS, int COLS, int NPC>
void Array2xfMat(ap_uint<ptr_width> *srcPtr, xf::Mat<MAT_T,ROWS,COLS,NPC>& dstMat)
{
   accel_utils au;
   au.Array2xfMat<ptr_width, MAT_T, ROWS, COLS, NPC>(srcPtr, dstMat);
}
}
//
//template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH>
//void auCopyMat(hls::stream< AU_TNAME(WORDWIDTH) >& _dst,
//		hls::stream< AU_TNAME(WORDWIDTH) >& _src,uint16_t imageheight,uint16_t image_widht)
//{
//
//	for (int i = 0; i < imageheight; i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		for(int j = 0; j < image_widht>>NPC; j++)
//		{
//			AU_TNAME(WORDWIDTH) val;
//			val = _src.read();
//			_dst.write(val);
//		}
//	}
//}


#endif//_XF_UTILITY_H_
