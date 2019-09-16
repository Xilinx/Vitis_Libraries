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

#ifndef _XF_RESIZE_NN_BILINEAR_
#define _XF_RESIZE_NN_BILINEAR_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#ifndef __SYNTHESIS__
#include <iostream>
#endif

template<int DEPTH, int INTERPOLATION_TYPE, int NPPC>
void interpolatePixel(XF_CTUNAME(DEPTH,NPPC) A0, XF_CTUNAME(DEPTH,NPPC) B0, XF_CTUNAME(DEPTH,NPPC) A1, XF_CTUNAME(DEPTH,NPPC) B1, ap_ufixed<12,2> Wx, ap_ufixed<12,2> Wy, XF_CTUNAME(DEPTH,NPPC) &pixel)
{
#pragma HLS inline
	if(INTERPOLATION_TYPE==XF_INTERPOLATION_NN)
	{
		pixel = A0;
	}
	else
	{
		ap_ufixed<12,2> Wxy;
		ap_int<16> val0,val1,val2;
		ap_fixed<28,18> P1,P2,P3,P4;
		ap_ufixed<28,18> one_num = 1.0;

		Wxy = (Wx*Wy);    // Wx - 0.32, Wy-0.32  (Wx*Wy-0.64)  Wxy - 0.32
		val0 = (A0+B1-(B0+A1));
		val1 = (B0-A0);
		val2 = (A1-A0);

		P1 = (val0*Wxy);		// val0(16.0) * Wxy(0.32) = P1(16.32)
		P2 = (val1*Wy);		// val1(16.0) * Wy(0.32) = P2(16.32)
		P3 = (val2*Wx);		// val1(16.0) * Wx(0.32) = P3(16.32)
		P4 = (A0);					// A0(8.0) P4(8.32)

		pixel = (XF_CTUNAME(DEPTH,NPPC))((ap_fixed<32,22>)(P1  + P2 + P3 + P4));
		// to get only integer part from sum of 8.32's , right shift by 32
	}
}
template<int DEPTH, int INTERPOLATION_TYPE, int NPPC, int T_INDEX_INT, int NUMBEROFINPUTWORDS>
void computeOutputPixel(XF_TNAME(DEPTH,NPPC) A0[NUMBEROFINPUTWORDS], XF_TNAME(DEPTH,NPPC) B0[NUMBEROFINPUTWORDS], ap_uint<T_INDEX_INT> initIndex, ap_uint<T_INDEX_INT> indexx[XF_NPIXPERCYCLE(NPPC)], ap_ufixed<12,2> Wx[XF_NPIXPERCYCLE(NPPC)], ap_ufixed<12,2> Wy, XF_TNAME(DEPTH,NPPC) &pixel)
{
#pragma HLS inline
	const int PIXELDEPTH = XF_DTPIXELDEPTH(DEPTH,NPPC);
	/*if(indexx[XF_NPIXPERCYCLE(NPPC)-1] > (initIndex+NUMBEROFINPUTWORDS*XF_NPIXPERCYCLE(NPPC)-1))
		{
			std::cout << "Insufficient number of words to resize in X" << std::endl;
			return;
		}*/
	assert((indexx[XF_NPIXPERCYCLE(NPPC)-1] < (initIndex+NUMBEROFINPUTWORDS*XF_NPIXPERCYCLE(NPPC)-1)) && "Insufficient number of words to resize in X");

	XF_PTUNAME(DEPTH) unpackX1[XF_NPIXPERCYCLE(NPPC)*NUMBEROFINPUTWORDS];
#pragma HLS ARRAY_PARTITION variable=unpackX1 complete dim=1
	XF_PTUNAME(DEPTH) unpackX2[XF_NPIXPERCYCLE(NPPC)*NUMBEROFINPUTWORDS];
#pragma HLS ARRAY_PARTITION variable=unpackX2 complete dim=1
	XF_PTUNAME(DEPTH) outputPixel[XF_NPIXPERCYCLE(NPPC)];
#pragma HLS ARRAY_PARTITION variable=outputPixel complete dim=1
	for(int k=0; k<NUMBEROFINPUTWORDS; k++)
	{
#pragma HLS UNROLL
		for(int i=0; i<XF_NPIXPERCYCLE(NPPC); i++)
		{
#pragma HLS UNROLL
			unpackX1[k*XF_NPIXPERCYCLE(NPPC)+i] = A0[k].range((i+1)*XF_DTPIXELDEPTH(DEPTH,NPPC)*XF_CHANNELS(DEPTH,NPPC)-1,i*XF_DTPIXELDEPTH(DEPTH,NPPC)*XF_CHANNELS(DEPTH,NPPC));
			unpackX2[k*XF_NPIXPERCYCLE(NPPC)+i] = B0[k].range((i+1)*XF_DTPIXELDEPTH(DEPTH,NPPC)*XF_CHANNELS(DEPTH,NPPC)-1,i*XF_DTPIXELDEPTH(DEPTH,NPPC)*XF_CHANNELS(DEPTH,NPPC));
		}
	}
	for(int i=0; i<XF_NPIXPERCYCLE(NPPC); i++)
	{
#pragma HLS UNROLL

		for(int k=0; k<XF_CHANNELS(DEPTH,NPPC); k++)
		{
#pragma HLS UNROLL
			XF_CTUNAME(DEPTH,NPPC) unpackX1temp[XF_NPIXPERCYCLE(NPPC)*NUMBEROFINPUTWORDS];
#pragma HLS ARRAY_PARTITION variable=unpackX1temp complete dim=1
			XF_CTUNAME(DEPTH,NPPC) unpackX2temp[XF_NPIXPERCYCLE(NPPC)*NUMBEROFINPUTWORDS];
#pragma HLS ARRAY_PARTITION variable=unpackX2temp complete dim=1
			for(int l=0; l<XF_NPIXPERCYCLE(NPPC)*NUMBEROFINPUTWORDS; l++)
			{
#pragma HLS UNROLL
				unpackX1temp[l] = unpackX1[l].range((k+1)*PIXELDEPTH-1,k*PIXELDEPTH);
				unpackX2temp[l] = unpackX2[l].range((k+1)*PIXELDEPTH-1,k*PIXELDEPTH);
			}
			XF_CTUNAME(DEPTH,NPPC) currentoutput;
			interpolatePixel<DEPTH, INTERPOLATION_TYPE, NPPC>(unpackX1temp[indexx[i]-initIndex], unpackX2temp[indexx[i]-initIndex], unpackX1temp[indexx[i]-initIndex+1], unpackX2temp[indexx[i]-initIndex+1], Wx[i], Wy, currentoutput);
			outputPixel[i].range((k+1)*PIXELDEPTH-1,k*PIXELDEPTH) = currentoutput;
		}
	}

	for(int i=0; i<XF_NPIXPERCYCLE(NPPC); i++)
	{
#pragma HLS UNROLL
		pixel.range((i+1)*XF_DTPIXELDEPTH(DEPTH,NPPC)*XF_CHANNELS(DEPTH,NPPC)-1,i*XF_DTPIXELDEPTH(DEPTH,NPPC)*XF_CHANNELS(DEPTH,NPPC)) = outputPixel[i];
	}
}
static uint64_t xfUDivResize (uint64_t in_n, unsigned short in_d)
{
#pragma HLS INLINE OFF
	uint32_t out_res = in_n/in_d;
	return out_res;
}

template<int NPPC, int T_SCALE_WIDTH, int T_SCALE_INT, int T_COMP_INDEX_WIDTH, int T_COMP_INDEX_INT>
void scaleMult(ap_ufixed<T_SCALE_WIDTH,T_SCALE_INT> scalex, ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT> scaleXParallel[XF_NPIXPERCYCLE(NPPC)])
{
#pragma HLS INLINE
	for(int i=0; i<XF_NPIXPERCYCLE(NPPC); i++)
	{
#pragma HLS PIPELINE
		scaleXParallel[i] = (ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT>)scalex*(ap_uint<8>)i;
	}
	return;
}
template<int T_INDEX_INT, int T_COMP_INDEX_WIDTH, int T_COMP_INDEX_INT, int T_SCALE_WIDTH, int T_SCALE_INT, int INTERPOLATION_TYPE>
void scaleCompute(int currindex, ap_ufixed<T_SCALE_WIDTH,T_SCALE_INT> inscale, ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT> &ind_pre)
{
	if(INTERPOLATION_TYPE==XF_INTERPOLATION_NN)
	{
		ind_pre = (ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT>)currindex*inscale + (ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT>)0.001;

	}
	else
	{
		ind_pre = ((ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT>)currindex + (ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT>)0.5)*inscale - (ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT>)0.5;
	}
}
template <int INTERPOLATION_TYPE, int T_COMP_INDEX_WIDTH, int T_COMP_INDEX_INT, int T_INDEX_INT, int T_SCALE_WIDTH, int T_SCALE_INT, int T_WEIGHT_WIDTH, int T_WEIGHT_INT, int NPPC>
void computeInterpolation(int inrows, int incols, int j, int output_rows_count, ap_ufixed<T_SCALE_WIDTH,T_SCALE_INT> scalex, ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT> scaleXParallel[XF_NPIXPERCYCLE(NPPC)], ap_ufixed<T_SCALE_WIDTH,T_SCALE_INT> scaley, ap_uint<T_INDEX_INT> indexx[XF_NPIXPERCYCLE(NPPC)], ap_uint<T_INDEX_INT> &indexy, ap_uint<T_INDEX_INT> &nextYScale, ap_ufixed<T_WEIGHT_WIDTH,T_WEIGHT_INT> WeightX[XF_NPIXPERCYCLE(NPPC)], ap_ufixed<T_WEIGHT_WIDTH,T_WEIGHT_INT> &WeightY, ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT> indexx_pre_comp, ap_fixed<T_COMP_INDEX_WIDTH,T_COMP_INDEX_INT> indexy_pre_comp)
{
	const int INDEX_INT = T_INDEX_INT;
	const int WEIGHT_WIDTH = T_WEIGHT_WIDTH;
	const int WEIGHT_INT = T_WEIGHT_INT;
	const int SCALE_WIDTH = T_SCALE_WIDTH;
	const int SCALE_INT = T_SCALE_INT;
	const int COMP_INDEX_WIDTH = T_COMP_INDEX_WIDTH;
	const int COMP_INDEX_INT = T_COMP_INDEX_INT;

	ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT> indexx_pre = 0;
	ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT> indexy_pre = 0;
	if(INTERPOLATION_TYPE==XF_INTERPOLATION_NN)
	{
		indexy_pre = indexy_pre_comp;
		nextYScale = indexy_pre+scaley;
		indexy = (ap_uint<INDEX_INT>)indexy_pre;
	}
	else
	{
		indexy_pre = indexy_pre_comp;
		nextYScale = indexy_pre+(ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT>)scaley;
		if(indexy_pre < 0)
		{
			indexy_pre = 0;
		}
		else if(indexy_pre > inrows-1)
		{
			indexy_pre = inrows-1;
		}
		indexy = (ap_uint<INDEX_INT>)indexy_pre;
		WeightY = ((ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT>)indexy_pre - (ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT>)indexy);
	}
	for(int i=0; i<XF_NPIXPERCYCLE(NPPC); i++)
	{
		ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT> indexy_pre = 0;
		if(INTERPOLATION_TYPE==XF_INTERPOLATION_NN)
		{
			indexx_pre = indexx_pre_comp + scaleXParallel[i];
			indexx[i] = (ap_uint<INDEX_INT>)indexx_pre;
		}
		else
		{
			indexx_pre = indexx_pre_comp + scaleXParallel[i];
			if(indexx_pre < 0)
			{
				indexx_pre = 0;
			}
			else if(indexx_pre > incols-1)
			{
				indexx_pre = incols-1;
			}
			indexx[i] = (ap_uint<INDEX_INT>)indexx_pre;
			WeightX[i] = ((ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT>)indexx_pre - (ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT>)indexx[i]);
		}
	}
}

template<int SRC_TYPE, int INHEIGHT, int INWIDTH, int NPPC, int OUTHEIGHT, int OUTWIDTH, int INTERPOLATION_TYPE, int MAX_DOWN_SCALE>
void resizeNNBilinear(xf::Mat<SRC_TYPE, INHEIGHT, INWIDTH, NPPC> &imgInput,xf::Mat<SRC_TYPE, OUTHEIGHT, OUTWIDTH, NPPC> &imgOutput)
{
#pragma HLS ALLOCATION instances=scaleCompute limit=1 function
#pragma HLS ALLOCATION instances=xfUDivResize limit=1 function
	const int INDEX_INT = 17;
	const int WEIGHT_WIDTH = 12;
	const int WEIGHT_INT = 2;
	const int SCALE_WIDTH = 32;
	const int SCALE_INT = 3;
	const int PRE_INDEX_WIDTH = 10;
	const int PRE_INDEX_INT = 17;
	const int COMP_INDEX_WIDTH = SCALE_WIDTH+PRE_INDEX_WIDTH;
	const int COMP_INDEX_INT = SCALE_INT+PRE_INDEX_INT;
	const int BUFFER_WORDS = MAX_DOWN_SCALE;
	const int BUFFER_DUP_FACTOR = (BUFFER_WORDS+1)>>1;

	uint64_t xnew,ynew;

	xnew = (imgInput.cols);///(float)(out_width<<XF_BITSHIFT(NPPC));
	ynew = (imgInput.rows);//(float)(out_height);

	xnew = xnew << 28;
	ynew = ynew << 28;
	ap_ufixed<SCALE_WIDTH,SCALE_INT> scalex,scaley;
	uint64_t Xscale64,Yscale64;
	Xscale64 = xfUDivResize (xnew , (imgOutput.cols));
	Yscale64 = xfUDivResize (ynew , (imgOutput.rows));
	ap_ufixed<64,32> temp_scale_conv;

	temp_scale_conv = Xscale64;
	temp_scale_conv = temp_scale_conv >> 28;
	scalex = temp_scale_conv;

	temp_scale_conv = Yscale64;
	temp_scale_conv = temp_scale_conv >> 28;
	scaley = temp_scale_conv;

	ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT> scaleXParallel[XF_NPIXPERCYCLE(NPPC)];
#pragma HLS ARRAY_PARTITION variable=scaleXParallel complete dim=1
	scaleMult<NPPC,SCALE_WIDTH,SCALE_INT,COMP_INDEX_WIDTH,COMP_INDEX_INT>(scalex,scaleXParallel);

	XF_TNAME(SRC_TYPE,NPPC) line_buffer[3][BUFFER_DUP_FACTOR][INWIDTH>>(XF_BITSHIFT(NPPC))];
#pragma HLS ARRAY_PARTITION variable=line_buffer complete dim=1
#pragma HLS ARRAY_PARTITION variable=line_buffer complete dim=2
	int input_read_pointer=0;
	int read_rows_count = 0;
	int output_write_pointer = 0;
	for(int i=0; i<2; i++) //read two rows
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=2
		for(int j=0; j<(imgInput.cols>>(XF_BITSHIFT(NPPC))); j++)
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=1 max=INWIDTH/NPPC
			for(int k=0; k<BUFFER_DUP_FACTOR ; k++)
			{
				line_buffer[i][k][j] = imgInput.read(input_read_pointer);
			}
			input_read_pointer++;
		}
		read_rows_count++;
	}
	int output_rows_count = 0;
	int first_row_index = 0;
	int second_row_index = 1;
	int read_row_index = 2;
	int loop_row_count = (imgOutput.rows > imgInput.rows)? imgOutput.rows : imgInput.rows;
	int loop_col_count = (imgOutput.cols > imgInput.cols)? imgOutput.cols : imgInput.cols;
	const int LOOPCOUNTROW = (INHEIGHT>OUTHEIGHT)? INHEIGHT: OUTHEIGHT;
	const int LOOPCOUNTCOL = (INWIDTH>OUTWIDTH)? INWIDTH: OUTWIDTH;
	ap_uint<INDEX_INT> indexx[XF_NPIXPERCYCLE(NPPC)];
#pragma HLS ARRAY_PARTITION variable=indexx complete dim=1
	ap_uint<INDEX_INT> indexy = 0;
	ap_uint<INDEX_INT> nextYScale = 0;
	ap_ufixed<WEIGHT_WIDTH,WEIGHT_INT> WeightX[XF_NPIXPERCYCLE(NPPC)];
#pragma HLS ARRAY_PARTITION variable=WeightX complete dim=1
	ap_ufixed<WEIGHT_WIDTH,WEIGHT_INT> WeightY = 0;
	XF_TNAME(SRC_TYPE,NPPC) P0Buf[BUFFER_DUP_FACTOR<<1];
#pragma HLS ARRAY_PARTITION variable=P0Buf complete dim=1
	XF_TNAME(SRC_TYPE,NPPC) P1Buf[BUFFER_DUP_FACTOR<<1];
#pragma HLS ARRAY_PARTITION variable=P1Buf complete dim=1

	ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT> indexx_pre_comp = 0;
	ap_fixed<COMP_INDEX_WIDTH,COMP_INDEX_INT> indexy_pre_comp = 0;

	for(int i=0; i<loop_row_count; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=LOOPCOUNTROW
		scaleCompute<INDEX_INT, COMP_INDEX_WIDTH, COMP_INDEX_INT, SCALE_WIDTH, SCALE_INT, INTERPOLATION_TYPE>(output_rows_count, scaley, indexy_pre_comp);
		for(int j=0; j<(loop_col_count>>(XF_BITSHIFT(NPPC))); j++)
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=1 max=LOOPCOUNTCOL/NPPC

			scaleCompute<INDEX_INT, COMP_INDEX_WIDTH, COMP_INDEX_INT, SCALE_WIDTH, SCALE_INT, INTERPOLATION_TYPE>(j<<(XF_BITSHIFT(NPPC)), scalex, indexx_pre_comp);
			computeInterpolation<INTERPOLATION_TYPE, COMP_INDEX_WIDTH, COMP_INDEX_INT, INDEX_INT, SCALE_WIDTH, SCALE_INT, WEIGHT_WIDTH, WEIGHT_INT, NPPC>(imgInput.rows, imgInput.cols, j<<(XF_BITSHIFT(NPPC)), output_rows_count, scalex, scaleXParallel, scaley, indexx, indexy, nextYScale, WeightX, WeightY, indexx_pre_comp, indexy_pre_comp);
			int indexstores = first_row_index;
			XF_TNAME(SRC_TYPE,NPPC) read_pixel;
			bool flag_write = 0;
			if(read_rows_count != imgInput.rows)
			{
				if((nextYScale >= read_rows_count-1)) //check if the next index y needed needs to be read.
				{
					if(j<(imgInput.cols>>(XF_BITSHIFT(NPPC))))
					{
						read_pixel = imgInput.read(input_read_pointer);
						flag_write = 1;
						input_read_pointer++;
					}
					else
					{
						flag_write = 0;
					}
				}
				else
				{
					flag_write = 0;
				}
			}
			else
			{
				flag_write = 0;
			}

			if(indexstores == 0)
			{
				for(int k=0; k<BUFFER_DUP_FACTOR; k++)
				{
#pragma HLS UNROLL
					int idx = (indexx[0]>>XF_BITSHIFT(NPPC))+(k<<1);
					int idx_nxt = idx + (indexx[0] == (imgInput.cols-1) ? 0 : 1);

					P0Buf[(k<<1)]   = line_buffer[0][k][idx];
					P0Buf[(k<<1)+1] = line_buffer[0][k][idx_nxt];
					P1Buf[(k<<1)]   = line_buffer[1][k][idx];
					P1Buf[(k<<1)+1] = line_buffer[1][k][idx_nxt];
				}
				if(flag_write)
				{
					for(int k=0; k<BUFFER_DUP_FACTOR; k++)
					{
#pragma HLS UNROLL
						line_buffer[2][k][j] = read_pixel;
					}
				}
			}
			else if(indexstores == 1)
			{
				for(int k=0; k<BUFFER_DUP_FACTOR; k++)
				{
#pragma HLS UNROLL
					int idx = (indexx[0]>>XF_BITSHIFT(NPPC))+(k<<1);
					int idx_nxt = idx + (indexx[0] == (imgInput.cols-1) ? 0 : 1);

					P0Buf[(k<<1)]   = line_buffer[1][k][idx];
					P0Buf[(k<<1)+1] = line_buffer[1][k][idx_nxt];
					P1Buf[(k<<1)]   = line_buffer[2][k][idx];
					P1Buf[(k<<1)+1] = line_buffer[2][k][idx_nxt];
				}
				if(flag_write)
				{
					for(int k=0; k<BUFFER_DUP_FACTOR; k++)
					{
#pragma HLS UNROLL
						line_buffer[0][k][j] = read_pixel;
					}
				}
			}
			else
			{
				for(int k=0; k<BUFFER_DUP_FACTOR; k++)
				{
#pragma HLS UNROLL
					int idx = (indexx[0]>>XF_BITSHIFT(NPPC))+(k<<1);
					int idx_nxt = idx + (indexx[0] == (imgInput.cols-1) ? 0 : 1);

					P0Buf[(k<<1)]   = line_buffer[2][k][idx];
					P0Buf[(k<<1)+1] = line_buffer[2][k][idx_nxt];
					P1Buf[(k<<1)]   = line_buffer[0][k][idx];
					P1Buf[(k<<1)+1] = line_buffer[0][k][idx_nxt];
				}
				if(flag_write)
				{
					for(int k=0; k<BUFFER_DUP_FACTOR; k++)
					{
#pragma HLS UNROLL
						line_buffer[1][k][j] = read_pixel;
					}
				}
			}
			if((output_rows_count <= imgOutput.rows-1) && (((indexy == read_rows_count-1) && (read_rows_count == imgInput.rows)) || (indexy == read_rows_count-2)))
			{
				if(j<(imgOutput.cols>>(XF_BITSHIFT(NPPC))))
				{
					if(indexy == read_rows_count-1)
					{
						for(int k=0; k<BUFFER_WORDS; k++)
						{
#pragma HLS UNROLL
							P0Buf[k] = P1Buf[k];
						}
					}
					XF_TNAME(SRC_TYPE,NPPC) temp_store_output;
					computeOutputPixel<SRC_TYPE,INTERPOLATION_TYPE,NPPC,INDEX_INT,BUFFER_WORDS>(P0Buf,P1Buf,((indexx[0]>>XF_BITSHIFT(NPPC))<<XF_BITSHIFT(NPPC)),indexx,WeightX,WeightY,temp_store_output);
					imgOutput.write(output_write_pointer,temp_store_output);
					output_write_pointer++;
				}
			}
		}
		if((output_rows_count <= imgOutput.rows-1) && (((indexy == read_rows_count-1) && (read_rows_count == imgInput.rows)) || (indexy == read_rows_count-2)))
		{
			output_rows_count++;
		}
		if(read_rows_count != imgInput.rows)
		{
			if((nextYScale >= read_rows_count-1)) //check if the next index y needed needs to be read.
			{
				first_row_index++;
				second_row_index++;
				read_row_index++;
				if(read_row_index == 3)
				{
					read_row_index = 0;
				}
				if(first_row_index == 3)
				{
					first_row_index = 0;
				}
				if(second_row_index == 3)
				{
					second_row_index = 0;
				}
				read_rows_count++;
			}
		}
	}
}
#endif


