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

#ifndef _AU_GAUSSIAN_FILTER_BLOCK_UTILS_HPP_
#define _AU_GAUSSIAN_FILTER_BLOCK_UTILS_HPP_

#include "au_gaussian_filter_lut.hpp"


void auApplyG7_block(uchar_t D1,  uchar_t D2,  uchar_t D3,  uchar_t D4,  uchar_t D5,  uchar_t D6,  uchar_t D7,
			   uchar_t D8,  uchar_t D9,  uchar_t D10, uchar_t D11, uchar_t D12, uchar_t D13, uchar_t D14,
			   uchar_t D15, uchar_t D16, uchar_t D17, uchar_t D18, uchar_t D19, uchar_t D20, uchar_t D21,
			   uchar_t D22, uchar_t D23, uchar_t D24, uchar_t D25, uchar_t D26, uchar_t D27, uchar_t D28,
			   uchar_t D29, uchar_t D30, uchar_t D31, uchar_t D32, uchar_t D33, uchar_t D34, uchar_t D35,
			   uchar_t D36, uchar_t D37, uchar_t D38, uchar_t D39, uchar_t D40, uchar_t D41, uchar_t D42,
			   uchar_t D43, uchar_t D44, uchar_t D45, uchar_t D46, uchar_t D47, uchar_t D48, uchar_t D49,
			   uchar_t *out_pix)
{

	unsigned int sum = 0.0, s1, s2, s3, s4, s5, s6, s7,s8,s9;

	sum = CENTER_COEFF_7x7 * D25;

	s1 =  D1 + D7 + D43 + D49;
	s2 =  D2 + D6 + D8  + D14 + D36 + D42 + D44 + D48;
	s3 =  D3 + D5 + D15 + D21 + D29 + D35 + D45 + D47;
	s4 =  D4 + D22 + D28 + D46;
	s5 =  D9 + D13 + D37 + D41;
	s6 = D10 + D12 + D16 + D20 + D30 + D34 + D38 + D40;
	s7 = D11 + D23 + D27 + D39;
	s8 = D17 + D19 + D31 + D33;
	s9 = D18 + D24 + D26 + D32;

	sum += s1*wts_7x7[0] + s2*wts_7x7[1] + s3*wts_7x7[2] +
		   s4*wts_7x7[3] + s5*wts_7x7[4] + s6*wts_7x7[5] +
		   s7*wts_7x7[6] + s8*wts_7x7[7] + s9*wts_7x7[8];

	unsigned short val = (sum >> 16);

	//ap_ufixed<8, 8> val = sum >> 16;
	*out_pix = (uchar_t)val;
}


template<int NPC,int WORDWIDTH,int DEPTH>
void auProcess7x7_block(AU_PTNAME(DEPTH)* tbuf1, AU_PTNAME(DEPTH)* tbuf2,
                     AU_PTNAME(DEPTH)* tbuf3, AU_PTNAME(DEPTH)* tbuf4,
                     AU_PTNAME(DEPTH)* tbuf5, AU_PTNAME(DEPTH)* tbuf6,
                     AU_PTNAME(DEPTH)* tbuf7, AU_TNAME(WORDWIDTH) *out1)
{
	uchar_t filter_vals[(1<<NPC)];
#pragma HLS ARRAY_PARTITION variable=filter_vals complete
	AU_TNAME(WORDWIDTH) out_val;
	for(int i = 0; i < (1<<NPC); i++)
	{
#pragma HLS UNROLL
		int shift = i << 3;
		int a=i, b=i,c=i,d=i,e=i,f=i;
		auApplyG7_block(tbuf1[i], tbuf1[a+1], tbuf1[b+2] ,tbuf1[c+3], tbuf1[d+4],tbuf1[e+5],tbuf1[f+6],
				  tbuf2[i], tbuf2[a+1], tbuf2[b+2], tbuf2[c+3], tbuf2[d+4],tbuf2[e+5],tbuf2[f+6],
				  tbuf3[i], tbuf3[a+1], tbuf3[b+2], tbuf3[c+3], tbuf3[d+4],tbuf3[e+5],tbuf3[f+6],
				  tbuf4[i], tbuf4[a+1], tbuf4[b+2], tbuf4[c+3], tbuf4[d+4],tbuf4[e+5],tbuf4[f+6],
				  tbuf5[i], tbuf5[a+1], tbuf5[b+2], tbuf5[c+3], tbuf5[d+4],tbuf5[e+5],tbuf5[f+6],
				  tbuf6[i], tbuf6[a+1], tbuf6[b+2], tbuf6[c+3], tbuf6[d+4],tbuf6[e+5],tbuf6[f+6],
				  tbuf7[i], tbuf7[a+1], tbuf7[b+2], tbuf7[c+3], tbuf7[d+4],tbuf7[e+5],tbuf7[f+6],
				  &filter_vals[i]);
		out_val.range(shift+7,shift) = filter_vals[i];
	}

	*out1 = out_val;
}
//auProcess7x7_block

template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH,int SRC_TC>
void auGaussianMat7x7_block(AU_TNAME(WORDWIDTH)* lbuf1, AU_TNAME(WORDWIDTH)* lbuf2,
                      AU_TNAME(WORDWIDTH)* lbuf3, AU_TNAME(WORDWIDTH)* lbuf4,
                      AU_TNAME(WORDWIDTH)* lbuf5, AU_TNAME(WORDWIDTH)* lbuf6,
                      AU_TNAME(WORDWIDTH)* lbuf7,
                      AU_TNAME(WORDWIDTH) linebuffer_out[COLS >> NPC])
{
	AU_PTNAME(DEPTH) tmp_buf1[(1<<NPC)+6];
#pragma HLS ARRAY_PARTITION variable=tmp_buf1 complete dim=1
	AU_PTNAME(DEPTH) tmp_buf2[(1<<NPC)+6];
#pragma HLS ARRAY_PARTITION variable=tmp_buf2 complete dim=1
	AU_PTNAME(DEPTH) tmp_buf3[(1<<NPC)+6];
#pragma HLS ARRAY_PARTITION variable=tmp_buf3 complete dim=1
	AU_PTNAME(DEPTH) tmp_buf4[(1<<NPC)+6];
#pragma HLS ARRAY_PARTITION variable=tmp_buf4 complete dim=1
	AU_PTNAME(DEPTH) tmp_buf5[(1<<NPC)+6];
#pragma HLS ARRAY_PARTITION variable=tmp_buf5 complete dim=1
	AU_PTNAME(DEPTH) tmp_buf6[(1<<NPC)+6];
#pragma HLS ARRAY_PARTITION variable=tmp_buf6 complete dim=1
	AU_PTNAME(DEPTH) tmp_buf7[(1<<NPC)+6];
#pragma HLS ARRAY_PARTITION variable=tmp_buf7 complete dim=1
	int x, k, i;

	uchar_t *t1;
	AU_TNAME(WORDWIDTH) tmp_out1, inter_val = 0, out_val = 0;
	int rpos1, rpos2, tbufsize;

	int nppc = AU_NPIXPERCYCLE(NPC);
	tbufsize = nppc + 6;
	rpos1 = (nppc - 3)*8;

	for(i = 0; i < 6; i++)
	{
#pragma HLS unroll
		tmp_buf1[i] = 0;
		tmp_buf2[i] = 0;
		tmp_buf3[i] = 0;
		tmp_buf4[i] = 0;
		tmp_buf5[i] = 0;
		tmp_buf6[i] = 0;
		tmp_buf7[i] = 0;
	}

	int si;
	if (NPC >= AU_NPPC8)
	{
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf1,lbuf1[0],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf2,lbuf2[0],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf3,lbuf3[0],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf4,lbuf4[0],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf5,lbuf5[0],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf6,lbuf6[0],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf7,lbuf7[0],6);

		auProcess7x7_block<NPC, WORDWIDTH, DEPTH>(tmp_buf1, tmp_buf2, tmp_buf3, tmp_buf4,
                                            tmp_buf5, tmp_buf6, tmp_buf7, &tmp_out1);

		out_val.range(rpos1+23, rpos1) = tmp_out1.range(23, 0);
		inter_val.range(rpos1-1, 0) = tmp_out1.range(rpos1+23, 24);

		si = 1;
	}
	else if(NPC == AU_NPPC1)
	{

		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf1,lbuf1[0],3);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf2,lbuf2[0],3);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf3,lbuf3[0],3);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf4,lbuf4[0],3);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf5,lbuf5[0],3);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf6,lbuf6[0],3);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf7,lbuf7[0],3);

		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf1,lbuf1[1],4);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf2,lbuf2[1],4);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf3,lbuf3[1],4);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf4,lbuf4[1],4);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf5,lbuf5[1],4);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf6,lbuf6[1],4);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf7,lbuf7[1],4);

		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf1,lbuf1[2],5);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf2,lbuf2[2],5);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf3,lbuf3[2],5);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf4,lbuf4[2],5);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf5,lbuf5[2],5);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf6,lbuf6[2],5);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf7,lbuf7[2],5);

		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf1,lbuf1[3],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf2,lbuf2[3],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf3,lbuf3[3],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf4,lbuf4[3],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf5,lbuf5[3],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf6,lbuf6[3],6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf7,lbuf7[3],6);

		auProcess7x7_block<NPC, WORDWIDTH, DEPTH>(tmp_buf1, tmp_buf2, tmp_buf3, tmp_buf4,
                                                 tmp_buf5, tmp_buf6, tmp_buf7, &tmp_out1);

		out_val = tmp_out1;
		linebuffer_out[0] = out_val;
		si = 4;
	}


	tmp_buf1[0] = tmp_buf1[tbufsize-6];
	tmp_buf2[0] = tmp_buf2[tbufsize-6];
	tmp_buf3[0] = tmp_buf3[tbufsize-6];
	tmp_buf4[0] = tmp_buf4[tbufsize-6];
	tmp_buf5[0] = tmp_buf5[tbufsize-6];
	tmp_buf6[0] = tmp_buf6[tbufsize-6];
	tmp_buf7[0] = tmp_buf7[tbufsize-6];

	tmp_buf1[1] = tmp_buf1[tbufsize-5];
	tmp_buf2[1] = tmp_buf2[tbufsize-5];
	tmp_buf3[1] = tmp_buf3[tbufsize-5];
	tmp_buf4[1] = tmp_buf4[tbufsize-5];
	tmp_buf5[1] = tmp_buf5[tbufsize-5];
	tmp_buf6[1] = tmp_buf6[tbufsize-5];
	tmp_buf7[1] = tmp_buf7[tbufsize-5];

	tmp_buf1[2] = tmp_buf1[tbufsize-4];
	tmp_buf2[2] = tmp_buf2[tbufsize-4];
	tmp_buf3[2] = tmp_buf3[tbufsize-4];
	tmp_buf4[2] = tmp_buf4[tbufsize-4];
	tmp_buf5[2] = tmp_buf5[tbufsize-4];
	tmp_buf6[2] = tmp_buf6[tbufsize-4];
	tmp_buf7[2] = tmp_buf7[tbufsize-4];

	tmp_buf1[3] = tmp_buf1[tbufsize-3];
	tmp_buf2[3] = tmp_buf2[tbufsize-3];
	tmp_buf3[3] = tmp_buf3[tbufsize-3];
	tmp_buf4[3] = tmp_buf4[tbufsize-3];
	tmp_buf5[3] = tmp_buf5[tbufsize-3];
	tmp_buf6[3] = tmp_buf6[tbufsize-3];
	tmp_buf7[3] = tmp_buf7[tbufsize-3];

	tmp_buf1[4] = tmp_buf1[tbufsize-2];
	tmp_buf2[4] = tmp_buf2[tbufsize-2];
	tmp_buf3[4] = tmp_buf3[tbufsize-2];
	tmp_buf4[4] = tmp_buf4[tbufsize-2];
	tmp_buf5[4] = tmp_buf5[tbufsize-2];
	tmp_buf6[4] = tmp_buf6[tbufsize-2];
	tmp_buf7[4] = tmp_buf7[tbufsize-2];

	tmp_buf1[5] = tmp_buf1[tbufsize-1];
	tmp_buf2[5] = tmp_buf2[tbufsize-1];
	tmp_buf3[5] = tmp_buf3[tbufsize-1];
	tmp_buf4[5] = tmp_buf4[tbufsize-1];
	tmp_buf5[5] = tmp_buf5[tbufsize-1];
	tmp_buf6[5] = tmp_buf6[tbufsize-1];
	tmp_buf7[5] = tmp_buf7[tbufsize-1];

	col_loop:
	for(k = si; k < (COLS >> NPC); k++)
	{
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max=SRC_TC
#pragma HLS pipeline

		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf1, lbuf1[k], 6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf2, lbuf2[k], 6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf3, lbuf3[k], 6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf4, lbuf4[k], 6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf5, lbuf5[k], 6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf6, lbuf6[k], 6);
		auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf7, lbuf7[k], 6);

		auProcess7x7_block<NPC, WORDWIDTH, DEPTH>(tmp_buf1, tmp_buf2, tmp_buf3, tmp_buf4,
                                                 tmp_buf5, tmp_buf6, tmp_buf7, &tmp_out1);

		if (NPC >= AU_NPPC8)
		{
			out_val = inter_val;
			out_val.range(rpos1+23,rpos1) = tmp_out1.range(23,0);
			inter_val.range(rpos1-1,0) = tmp_out1.range(rpos1 +23,24);
			linebuffer_out[k-si] = out_val;
		}
		else
		{
			out_val = tmp_out1;
			linebuffer_out[k-si+1] = out_val;
		}


		tmp_buf1[0] = tmp_buf1[tbufsize-6];
		tmp_buf2[0] = tmp_buf2[tbufsize-6];
		tmp_buf3[0] = tmp_buf3[tbufsize-6];
		tmp_buf4[0] = tmp_buf4[tbufsize-6];
		tmp_buf5[0] = tmp_buf5[tbufsize-6];
		tmp_buf6[0] = tmp_buf6[tbufsize-6];
		tmp_buf7[0] = tmp_buf7[tbufsize-6];

		tmp_buf1[1] = tmp_buf1[tbufsize-5];
		tmp_buf2[1] = tmp_buf2[tbufsize-5];
		tmp_buf3[1] = tmp_buf3[tbufsize-5];
		tmp_buf4[1] = tmp_buf4[tbufsize-5];
		tmp_buf5[1] = tmp_buf5[tbufsize-5];
		tmp_buf6[1] = tmp_buf6[tbufsize-5];
		tmp_buf7[1] = tmp_buf7[tbufsize-5];

		tmp_buf1[2] = tmp_buf1[tbufsize-4];
		tmp_buf2[2] = tmp_buf2[tbufsize-4];
		tmp_buf3[2] = tmp_buf3[tbufsize-4];
		tmp_buf4[2] = tmp_buf4[tbufsize-4];
		tmp_buf5[2] = tmp_buf5[tbufsize-4];
		tmp_buf6[2] = tmp_buf6[tbufsize-4];
		tmp_buf7[2] = tmp_buf7[tbufsize-4];

		tmp_buf1[3] = tmp_buf1[tbufsize-3];
		tmp_buf2[3] = tmp_buf2[tbufsize-3];
		tmp_buf3[3] = tmp_buf3[tbufsize-3];
		tmp_buf4[3] = tmp_buf4[tbufsize-3];
		tmp_buf5[3] = tmp_buf5[tbufsize-3];
		tmp_buf6[3] = tmp_buf6[tbufsize-3];
		tmp_buf7[3] = tmp_buf7[tbufsize-3];

		tmp_buf1[4] = tmp_buf1[tbufsize-2];
		tmp_buf2[4] = tmp_buf2[tbufsize-2];
		tmp_buf3[4] = tmp_buf3[tbufsize-2];
		tmp_buf4[4] = tmp_buf4[tbufsize-2];
		tmp_buf5[4] = tmp_buf5[tbufsize-2];
		tmp_buf6[4] = tmp_buf6[tbufsize-2];
		tmp_buf7[4] = tmp_buf7[tbufsize-2];

		tmp_buf1[5] = tmp_buf1[tbufsize-1];
		tmp_buf2[5] = tmp_buf2[tbufsize-1];
		tmp_buf3[5] = tmp_buf3[tbufsize-1];
		tmp_buf4[5] = tmp_buf4[tbufsize-1];
		tmp_buf5[5] = tmp_buf5[tbufsize-1];
		tmp_buf6[5] = tmp_buf6[tbufsize-1];
		tmp_buf7[5] = tmp_buf7[tbufsize-1];

	}
	AU_TNAME(WORDWIDTH) var = 0;
	auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf1,var,6);
	auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf2,var,6);
	auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf3,var,6);
	auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf4,var,6);
	auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf5,var,6);
	auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf6,var,6);
	auExtractPixels<NPC, WORDWIDTH, DEPTH>(tmp_buf7,var,6);

	if (NPC >= AU_NPPC8)
	{
		auProcess7x7_block<NPC, WORDWIDTH, DEPTH>(tmp_buf1, tmp_buf2, tmp_buf3, tmp_buf4,
                                                 tmp_buf5, tmp_buf6, tmp_buf7, &tmp_out1);

		out_val = inter_val;
		out_val.range(rpos1+23, rpos1) = tmp_out1.range(23, 0);
		linebuffer_out[k-si] = out_val;
	}
	else
	{
		for(int k = 1; k < si; k++)
		{
			auProcess7x7_block<NPC, WORDWIDTH, DEPTH>(tmp_buf1, tmp_buf2, tmp_buf3, tmp_buf4,
                                                    tmp_buf5, tmp_buf6, tmp_buf7, &tmp_out1);
			out_val = tmp_out1;
			linebuffer_out[(COLS >> NPC)-si+k-1] = out_val;

			tmp_buf1[0] = tmp_buf1[tbufsize-6];
			tmp_buf2[0] = tmp_buf2[tbufsize-6];
			tmp_buf3[0] = tmp_buf3[tbufsize-6];
			tmp_buf4[0] = tmp_buf4[tbufsize-6];
			tmp_buf5[0] = tmp_buf5[tbufsize-6];
			tmp_buf6[0] = tmp_buf6[tbufsize-6];
			tmp_buf7[0] = tmp_buf7[tbufsize-6];

			tmp_buf1[1] = tmp_buf1[tbufsize-5];
			tmp_buf2[1] = tmp_buf2[tbufsize-5];
			tmp_buf3[1] = tmp_buf3[tbufsize-5];
			tmp_buf4[1] = tmp_buf4[tbufsize-5];
			tmp_buf5[1] = tmp_buf5[tbufsize-5];
			tmp_buf6[1] = tmp_buf6[tbufsize-5];
			tmp_buf7[1] = tmp_buf7[tbufsize-5];

			tmp_buf1[2] = tmp_buf1[tbufsize-4];
			tmp_buf2[2] = tmp_buf2[tbufsize-4];
			tmp_buf3[2] = tmp_buf3[tbufsize-4];
			tmp_buf4[2] = tmp_buf4[tbufsize-4];
			tmp_buf5[2] = tmp_buf5[tbufsize-4];
			tmp_buf6[2] = tmp_buf6[tbufsize-4];
			tmp_buf7[2] = tmp_buf7[tbufsize-4];

			tmp_buf1[3] = tmp_buf1[tbufsize-3];
			tmp_buf2[3] = tmp_buf2[tbufsize-3];
			tmp_buf3[3] = tmp_buf3[tbufsize-3];
			tmp_buf4[3] = tmp_buf4[tbufsize-3];
			tmp_buf5[3] = tmp_buf5[tbufsize-3];
			tmp_buf6[3] = tmp_buf6[tbufsize-3];
			tmp_buf7[3] = tmp_buf7[tbufsize-3];

			tmp_buf1[4] = tmp_buf1[tbufsize-2];
			tmp_buf2[4] = tmp_buf2[tbufsize-2];
			tmp_buf3[4] = tmp_buf3[tbufsize-2];
			tmp_buf4[4] = tmp_buf4[tbufsize-2];
			tmp_buf5[4] = tmp_buf5[tbufsize-2];
			tmp_buf6[4] = tmp_buf6[tbufsize-2];
			tmp_buf7[4] = tmp_buf7[tbufsize-2];

			tmp_buf1[5] = tmp_buf1[tbufsize-1];
			tmp_buf2[5] = tmp_buf2[tbufsize-1];
			tmp_buf3[5] = tmp_buf3[tbufsize-1];
			tmp_buf4[5] = tmp_buf4[tbufsize-1];
			tmp_buf5[5] = tmp_buf5[tbufsize-1];
			tmp_buf6[5] = tmp_buf6[tbufsize-1];
			tmp_buf7[5] = tmp_buf7[tbufsize-1];

			tmp_buf1[6] = 0;
			tmp_buf2[6] = 0;
			tmp_buf3[6] = 0;
			tmp_buf4[6] = 0;
			tmp_buf5[6] = 0;
			tmp_buf6[6] = 0;
			tmp_buf7[6] = 0;

		}
	}
}
//auGaussianMat7x7_block

#endif//_AU_GAUSSIAN_FILTER_BLOCK_UTILS_HPP_
