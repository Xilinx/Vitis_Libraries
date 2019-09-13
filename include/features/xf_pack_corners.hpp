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
HOWEVER CASED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _XF_PACK_CORNERS_HPP_
#define _XF_PACK_CORNERS_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif
#include <stdio.h>


template<int NPC,int WORDWIDTH_DST,int MAXSIZE,int TC>
void auFillList(ap_uint<32> listcorners[MAXSIZE],XF_SNAME(WORDWIDTH_DST) tmp_cor_bufs[][MAXSIZE>> XF_BITSHIFT(NPC)], uint16_t *cor_cnts, uint32_t *nCorners )
{
	int sz = 0;
	ap_uint<9> i;
	uint32_t EOL = 0;
	for (i = 0; i < (1<<XF_BITSHIFT(NPC)); i++)
	{
#pragma HLS unroll

		for(int crn_cnt=0;crn_cnt < cor_cnts[i]; crn_cnt++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			XF_SNAME(WORDWIDTH_DST) val = tmp_cor_bufs[i][crn_cnt];
			listcorners[sz+crn_cnt]=val;
		}
		sz += cor_cnts[i];
	}
	//listcorners[sz]=EOL;
	*nCorners = sz;
}

template<int NPC,int WORDWIDTH_DST,int MAXSIZE,int TC>
void auFillList_points(ap_uint32_t *listcorners,ap_uint<32> tmp_cor_bufs[][MAXSIZE>> XF_BITSHIFT(NPC)],int *cor_cnts )
{
	int sz = 0;
	for (int i=0;i<(1<<XF_BITSHIFT(NPC));i++)
	{
#pragma HLS unroll
		for(int crn_cnt=0;crn_cnt < cor_cnts[i]; crn_cnt++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			listcorners[i*sz+crn_cnt] = (ap_uint32_t)(tmp_cor_bufs[i][crn_cnt]);
		}
		sz += cor_cnts[i];
	}
}


template<int NPC,int IN_WW,int MAXSIZE,int OUT_WW>
void auCheckForCorners(XF_SNAME(IN_WW) val, uint16_t *corners_cnt, XF_SNAME(OUT_WW) out_keypoints[][MAXSIZE>> XF_BITSHIFT(NPC)], ap_uint<13> row, ap_uint<13> col)
{
	XF_SNAME(OUT_WW) tmp_store[(1<<XF_BITSHIFT(NPC))];

	for(ap_uint<9> i = 0; i < (1<<XF_BITSHIFT(NPC)); i++)
	{
#pragma HLS unroll
		ap_uint<9> shift = i << 3;
		ap_uint<8> v = val >> shift;
		uint16_t cnt = corners_cnt[i];

		if ((cnt < (MAXSIZE>> XF_BITSHIFT(NPC))) && (v != 0))
		{
			XF_SNAME(OUT_WW) tmp = 0;

			tmp.range(15,0) = col+i;
			tmp.range(31,16) = row;
			out_keypoints[i][cnt] = tmp;
			cnt++;
		}
		else
		{
			tmp_store[i] = 0;
		}
		corners_cnt[i] = cnt;
	}
}

template<int ROWS, int COLS, int IN_DEPTH, int NPC, int IN_WW, int MAXPNTS, int OUT_WW, int SRC_TC>
void xFWriteCornersToList(hls::stream< XF_SNAME(IN_WW) > &_max_sup, ap_uint<32> list[MAXPNTS], uint32_t *nCorners, uint16_t img_height, uint16_t img_width)
{

	img_width = img_width >> XF_BITSHIFT(NPC);
	if(NPC == XF_NPPC1)
	{
		int cnt = 0;
		uint32_t EOL = 0;
		ap_uint<13> row, col;
		for(row = 0; row < img_height; row++)
		{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max = ROWS
#pragma HLS LOOP_FLATTEN off
			for (col = 0; col < img_width; col++){
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max = SRC_TC
#pragma HLS pipeline
				XF_SNAME(IN_WW) val = _max_sup.read();

				if ((cnt < (MAXPNTS)) && (val != 0))
				{
					XF_SNAME(OUT_WW) tmp = 0;

					tmp.range(15,0) = col;
					tmp.range(31,16) = row;
					list[cnt]=tmp;//.write(tmp);
					cnt++;
				}
			}
		}
		//list[cnt]=EOL;
		*nCorners = cnt;
	}
	else
	{
		XF_SNAME(OUT_WW) tmp_corbufs[(1<<XF_BITSHIFT(NPC))][(MAXPNTS>> XF_BITSHIFT(NPC))];
		uint16_t corners_cnt[(1<<XF_BITSHIFT(NPC))];
#pragma HLS ARRAY_PARTITION variable=corners_cnt complete dim=0
#pragma HLS ARRAY_PARTITION variable=tmp_corbufs complete dim=1

		for (ap_uint<9> i = 0; i < (1<<XF_BITSHIFT(NPC)); i++)
		{
#pragma HLS unroll
			corners_cnt[i] = 0;
		}
		ap_uint<13> row, col;
		for(row = 0; row < img_height; row++)
		{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max = ROWS
#pragma HLS LOOP_FLATTEN off
			for (col = 0; col < img_width; col++){
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max = SRC_TC
#pragma HLS pipeline
				XF_SNAME(IN_WW) val = _max_sup.read();
				auCheckForCorners<NPC,IN_WW,MAXPNTS,OUT_WW>(val,corners_cnt,tmp_corbufs,row,(col<<XF_BITSHIFT(NPC)));
			}
		}

		auFillList<NPC,OUT_WW,MAXPNTS,(MAXPNTS>> XF_BITSHIFT(NPC))>(list,tmp_corbufs,corners_cnt, nCorners);
	}

}

#endif	// _XF_MAX_SUPPRESSION_H_
