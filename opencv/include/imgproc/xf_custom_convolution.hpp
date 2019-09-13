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

#ifndef _XF_CUSTOM_CONVOLUTION_HPP_
#define _XF_CUSTOM_CONVOLUTION_HPP_


#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
typedef unsigned char uchar;


namespace xf{

/****************************************************************************************
 * xFApplyCustomFilter: Applies the user defined kernel to the input image.
 *
 * _lbuf	   ->  Buffer containing the input image data
 * _kernel	   ->  Kernel provided by the user of type 16S
 * shift	   ->  Fixed point format of the filter co-efficients for unity gain filter
 ****************************************************************************************/
template<int DEPTH_SRC, int DEPTH_DST, int filter_height, int filter_width,
int NPC, int buf_width,  typename buf_type>
XF_PTNAME(DEPTH_DST) xFApplyCustomFilter(buf_type _lbuf[][buf_width],
		short int _kernel[][filter_width], int ind, unsigned char shift)
		{
#pragma HLS INLINE off
	XF_PTNAME(DEPTH_DST) res = 0;
	ap_int32_t tmp_res = 0;
	ap_int24_t conv_val[filter_height][filter_width];
#pragma HLS ARRAY_PARTITION variable=conv_val complete dim=0

	ap_int32_t row_sum[filter_height], fix_res = 0, tmp_row_sum = 0;
#pragma HLS ARRAY_PARTITION variable=row_sum complete dim=1

	XF_PTNAME(DEPTH_DST) arr_ind = ind;

	// performing kernel operation and storing in the temporary buffer
	filterLoopI:
	for(uchar i = 0; i < filter_height; i++)
	{
#pragma HLS UNROLL
		arr_ind = ind;

		filterLoopJ:
		for(uchar j = 0; j < filter_width; j++)
		{
#pragma HLS UNROLL
			conv_val[i][j] = (_lbuf[i][arr_ind] * _kernel[i][j]);
			arr_ind++;
		}
	}

	// accumulating the row sum values of the temporary buffer
	addFilterLoopI:
	for(uchar i = 0; i < filter_height; i++)
	{
#pragma HLS UNROLL
		tmp_row_sum = 0;

		addFilterLoopJ:
		for(uchar j = 0; j < filter_width; j++)
		{
#pragma HLS UNROLL
			tmp_row_sum += conv_val[i][j];
		}
		row_sum[i] = tmp_row_sum;
	}

	// adding the row_sum buffer elements and storing in the result
	resultFilterLoopI:
	for(uchar i = 0; i < filter_height; i++)
	{
#pragma HLS UNROLL
		fix_res += row_sum[i];
	}

	// converting the input type from Q1.shift
	tmp_res = (fix_res >> shift);

	// overflow handling depending upon the input type
	if(DEPTH_DST == XF_8UP)
	{
		if(tmp_res > ((1 << (XF_PIXELDEPTH(DEPTH_DST))) - 1))
		{
			res = ((1 << (XF_PIXELDEPTH(DEPTH_DST))) - 1);
		}
		else if(tmp_res < 0)
		{
			res = 0;
		}
		else
		{
			res = tmp_res;
		}
	}
	else if(DEPTH_DST == XF_16SP)
	{
		//		if(tmp_res > ((1 << (XF_PIXELDEPTH(DEPTH_DST)-1)) - 1))
		//		{
		//			res = ((1 << (XF_PIXELDEPTH(DEPTH_DST)-1)) - 1);
		//		}
		//		else if(tmp_res < -(1 << (XF_PIXELDEPTH(DEPTH_DST)-1)))
		//		{
		//			res = -(1 << (XF_PIXELDEPTH(DEPTH_DST)-1));
		//		}
		//		else
		//		{
		res = tmp_res;
		//}
	}
	return res;
		}


/****************************************************************************************
 * xFComputeCustomFilter : Applies the mask and Computes the filter value for NPC
 * 					number of times.
 *
 * _lbuf	   ->  Buffer containing the input image data
 * _kernel	   ->  Kernel provided by the user of type 16S
 * _mask_value ->  The output buffer containing ouput image data
 * shift	   ->  Fixed point format of the filter co-efficients for unity gain filter
 ****************************************************************************************/
template<int filter_height, int filter_width, int buf_width, int NPC, int DEPTH_SRC, int DEPTH_DST>
void xFComputeCustomFilter(
		XF_PTNAME(DEPTH_SRC) _lbuf[][buf_width], short int _kernel[][filter_width],
		XF_PTNAME(DEPTH_DST)* _mask_value, unsigned char shift)
{
#pragma HLS inline
	// computes the filter operation depending upon the mode of parallelism
	computeFilterLoop:
	for(ap_uint<5> j = 0; j < XF_NPIXPERCYCLE(NPC); j++)
	{
#pragma HLS UNROLL
		_mask_value[j] = xFApplyCustomFilter<DEPTH_SRC,DEPTH_DST,filter_height,
				filter_width,NPC>(_lbuf,_kernel,j,shift);
	}
}

template<int SRC_T, int DST_T, int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST,int NPC,int WORDWIDTH_SRC,int WORDWIDTH_DST,int TC,int FW,int filter_height,int filter_width,int F_COUNT>
void Convolution_Process(xf::Mat<SRC_T, ROWS, COLS, NPC>& _src,xf::Mat<DST_T, ROWS, COLS, NPC>& _dst,XF_SNAME(WORDWIDTH_SRC) buf[filter_height][COLS>>XF_BITSHIFT(NPC)], XF_PTNAME(DEPTH_SRC) lbuf[filter_height][XF_NPIXPERCYCLE(NPC)+filter_width-1],
		XF_SNAME(WORDWIDTH_SRC) tmp_buf[filter_height],XF_PTNAME(DEPTH_DST) mask_value[XF_NPIXPERCYCLE(NPC)],short int _filter[][filter_width],uint16_t image_width,uchar row_ind, unsigned char shift,XF_SNAME(WORDWIDTH_DST) &P0,
		unsigned char index[filter_height],ap_uint<13> col_factor,uchar filter_width_factor,unsigned short image_height, ap_uint<13> row, int &rd_ind, int &wr_ind)
{
#pragma HLS INLINE
	uchar step = XF_PIXELDEPTH(DEPTH_DST);
	uchar max_loop = XF_WORDDEPTH(WORDWIDTH_DST);
	mainColLoop:
	for(ap_uint<13> col = 0; col < (image_width); col++)   // Width of the image
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS PIPELINE

		// reading the data from the stream to the input buffer

		if(row < image_height)
		{
			buf[row_ind][col] = _src.read(rd_ind);
			rd_ind++;
		}
		else
		{
			buf[row_ind][col] = 0;
		}

		// loading the data from the input buffer to the temporary buffer
		fillTempBuffer_1:
		for(uchar l = 0; l < filter_height; l++)
		{
#pragma HLS UNROLL
			tmp_buf[l] = buf[index[l]][col];
		}

		// extracting the pixels from the temporary buffer to the line buffer
		extractPixelsLoop_1:
		for(uchar l = 0; l < filter_height; l++)
		{
#pragma HLS UNROLL
			xfExtractPixels<NPC,WORDWIDTH_SRC,DEPTH_SRC>(
					&lbuf[l][(filter_width-1)],tmp_buf[l],0);
		}

		// computing the mask value
		xFComputeCustomFilter<filter_height,filter_width,
		(XF_NPIXPERCYCLE(NPC)+filter_width-1),NPC,DEPTH_SRC,
		DEPTH_DST>(lbuf,_filter,mask_value,shift);

		// left column border condition
		if(col <= col_factor)
		{
			ap_uint<13> ind = filter_width_factor;
			ap_uint<13> range_step = 0;

			if((XF_NPIXPERCYCLE(NPC) - filter_width_factor) >= 0)
			{
				packMaskToTempRes_1:
				for(uchar l = 0; l < (XF_NPIXPERCYCLE(NPC) - FW); l++)
				{
#pragma HLS LOOP_TRIPCOUNT min=F_COUNT max=F_COUNT
#pragma HLS UNROLL
					P0.range((range_step+(step-1)),range_step) = mask_value[ind++];
					range_step += step;
				}
			}
			else
			{
				filter_width_factor -= XF_NPIXPERCYCLE(NPC);
			}
		}

		// packing the data from the mask value to the temporary result P0 and pushing data into stream
		else
		{
			ap_uint<10> max_range_step = max_loop - (filter_width_factor * step);

			packMaskToTempRes_2:
			for(uchar l = 0; l < FW; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=FW max=FW
#pragma HLS UNROLL
				P0.range((max_range_step+(step-1)),
						(max_range_step)) = mask_value[l];
				max_range_step += step;
			}

			// writing the temporary result into the stream
			_dst.write(wr_ind,P0);
			wr_ind++;

			ap_uint<13> ind = filter_width_factor;
			ap_uint<13> range_step = 0;

			packMaskToTempRes_3:
			for(ap_uint<13> l = 0; l < (XF_NPIXPERCYCLE(NPC) -
					FW); l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=F_COUNT max=F_COUNT
#pragma HLS UNROLL
				P0.range((range_step+(step-1)),
						range_step) = mask_value[ind++];
				range_step += step;
			}
		}

		// re-initializing the line buffers
		copyEndPixelsI_1:
		for(uchar i = 0; i < filter_height; i++)
		{
#pragma HLS UNROLL
			copyEndPixelsJ_1:
			for(uchar l = 0; l < (filter_width-1); l++)
			{
#pragma HLS UNROLL
				lbuf[i][l] = lbuf[i][XF_NPIXPERCYCLE(NPC) + l];
			}
		}
	}  //  end of main column loop*/

}

/************************************************************************************
 * xFCustomConvKernel : Convolutes the input filter over the input image and writes
 * 					onto the output image.
 *
 * _src		->  Input image of type 8U
 * _filter	->  Kernel provided by the user of type 16S
 * _dst		->  Output image after applying the filter operation, of type 8U or 16S
 * shift	->  Fixed point format of the filter co-efficients for unity gain filter
 ************************************************************************************/
template<int SRC_T, int DST_T, int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC,
int WORDWIDTH_DST, int COLS_COUNT,  int filter_height,
int filter_width, int F_COUNT, int FW, int COL_FACTOR_COUNT>
void xFCustomConvolutionKernel(
		xf::Mat<SRC_T, ROWS, COLS, NPC>& _src,
		short int _filter[][filter_width],
		xf::Mat<DST_T, ROWS, COLS, NPC>& _dst,
		unsigned char shift,unsigned short img_width,unsigned short img_height)
{
	uchar step = XF_PIXELDEPTH(DEPTH_DST);
	uchar max_loop = XF_WORDDEPTH(WORDWIDTH_DST);
	uchar buf_size = (XF_NPIXPERCYCLE(NPC)+filter_width-1);

	uchar row_ind = 0, row_ptr = 0;
	unsigned char index[filter_height];
#pragma HLS ARRAY_PARTITION variable=index complete dim=1

	XF_SNAME(WORDWIDTH_DST) P0;
	XF_SNAME(WORDWIDTH_SRC) buf[filter_height][COLS>>XF_BITSHIFT(NPC)];
#pragma HLS ARRAY_PARTITION variable=buf complete dim=1

	XF_PTNAME(DEPTH_SRC) lbuf[filter_height][XF_NPIXPERCYCLE(NPC)+filter_width-1];
#pragma HLS ARRAY_PARTITION variable=lbuf complete dim=0

	XF_SNAME(WORDWIDTH_SRC) tmp_buf[filter_height];
#pragma HLS ARRAY_PARTITION variable=tmp_buf complete dim=1

	XF_PTNAME(DEPTH_DST) mask_value[XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=mask_value complete dim=1

	XF_PTNAME(DEPTH_DST) col_border_mask[(filter_width >> 1)];
#pragma HLS ARRAY_PARTITION variable=col_border_mask complete dim=1

	ap_uint<13> col_factor = 0;
	uchar filter_width_factor = (filter_width >> 1);
	int rd_ind = 0, wr_ind = 0;

	// setting the column factor depending upon the filter dimensions
	colFactorLoop:
	for(uchar f = (filter_width >> 1); f > (XF_NPIXPERCYCLE(NPC));
			f = (f - XF_NPIXPERCYCLE(NPC)))
	{
		col_factor++;
	}

	// initializing the first two rows to zeros
	fillBufZerosI:
	for(uchar i = 0; i < (filter_height>>1); i++)
	{
#pragma HLS UNROLL
		fillBufZerosJ:
		for(ap_uint<13> j = 0; j < (img_width); j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_COUNT max=COLS_COUNT
#pragma HLS UNROLL
			buf[row_ind][j] = 0;
		}
		row_ind++;
	}

	// reading the first two rows from the input stream
	readTopBorderI:
	for(uchar i = 0; i < (filter_height>>1); i++)
	{
#pragma HLS UNROLL
		readTopBorderJ:
		for(ap_uint<13> j = 0; j < (img_width); j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_COUNT max=COLS_COUNT
#pragma HLS PIPELINE
			buf[row_ind][j] = _src.read(rd_ind);
			rd_ind++;
		}
		row_ind++;
	}

	// row loop from 1 to the end of the image
	mainRowLoop:
	for(ap_uint<13> row = (filter_height >> 1); row < (img_height + ((filter_height >> 1))); row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		row_ptr = row_ind+1;

		// index calculation
		settingIndex_1:
		for(int l = 0; l < filter_height; l++)
		{
#pragma HLS UNROLL
			if(row_ptr >= filter_height)
				row_ptr = 0;

			index[l] = row_ptr++;
		}

		// initializing the line buffer to zero
		fillingLineBufferZerosI_1:
		for(uchar i = 0; i < filter_height; i++)
		{
#pragma HLS UNROLL
			fillingLineBufferZerosJ_1:
			for(uchar j = 0; j < (filter_width-1); j++)
			{
#pragma HLS UNROLL
				lbuf[i][j] = 0;
			}
		}
		// initializing the temporary result value to zero
		P0 = 0;

		Convolution_Process<SRC_T, DST_T, ROWS,COLS,DEPTH_SRC,DEPTH_DST,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,COLS_COUNT,FW,filter_height,filter_width,F_COUNT>
		(_src,_dst,buf,lbuf,tmp_buf,mask_value,_filter,img_width,row_ind,shift,P0,index,col_factor,filter_width_factor,img_height,row,rd_ind,wr_ind);

		/////////  Column right border  /////////

		// initializing the line buffers to zero
		fillingLineBufferZerosI_2:
		for(uchar i = 0; i < filter_height; i++)
		{
#pragma HLS UNROLL
			fillingLineBufferZerosJ_2:
			for(ap_uint<13> l = (filter_width-1); l < buf_size; l++)
			{
#pragma HLS UNROLL
				lbuf[i][l] = 0;
			}
		}

		// applying the filter and computing the mask_value
		if((filter_width >> 1) > 0)
		{
			getMaskValue_1:
			for(uchar i = 0; i < (filter_width >> 1); i++)
			{
#pragma HLS UNROLL
				col_border_mask[i] = xFApplyCustomFilter<DEPTH_SRC,DEPTH_DST,
						filter_height,filter_width,NPC>(lbuf,_filter,i,shift);
			}
		}

		int max_range_step = max_loop - (FW * step);

		packMaskToTempRes_4:
		for(uchar l = 0; l < FW; l++)
		{
#pragma HLS LOOP_TRIPCOUNT min=FW max=FW
#pragma HLS UNROLL
			P0.range((max_range_step+step-1),
					(max_range_step)) = col_border_mask[l];
			max_range_step += step;
		}

		// writing the temporary result into the stream
		_dst.write(wr_ind,P0);
		wr_ind++;

		colFactorLoopBorder:
		for(ap_uint<13> c = 0; c < col_factor; c++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COL_FACTOR_COUNT max=COL_FACTOR_COUNT

			max_range_step = 0;
			widthFactorLoopBorder:
			for(int l = FW;l < (XF_NPIXPERCYCLE(NPC) + FW); l++)
			{
				P0.range((max_range_step+(step-1)),
						(max_range_step)) = col_border_mask[l];
				max_range_step += step;
			}
			_dst.write(wr_ind,P0);
			wr_ind++;
		}

		// incrementing the row_ind for each iteration of row
		row_ind++;
		if(row_ind == filter_height)
		{
			row_ind = 0;
		}
	}  // end of main row loop
}  // end of xFCustomConvKernel

template<int DEPTH_SRC, int DEPTH_DST,int F_HEIGHT, int F_WIDTH,int PLANES>
void xFApplyFilter2D(XF_PTNAME(DEPTH_SRC) _kernel_pixel[F_HEIGHT][F_WIDTH],
		short int	_kernel_filter[F_HEIGHT][F_WIDTH],
		XF_PTNAME(DEPTH_DST)  &out, unsigned char shift)
{
#pragma HLS INLINE off

	ap_int<32> sum=0,in_step=0,out_step=0,p=0;
	ap_int<32> temp=0;
	ap_int<32> tmp_sum=0;
	FILTER_LOOP_HEIGHT:
	ap_uint<24> bgr_val;
if ((DEPTH_DST == XF_8UP) || (DEPTH_DST == XF_24UP))
{
	 in_step=8;
	 out_step=8;
}
else
{
	in_step=8;
	out_step=16;
}
for(ap_uint<8> c=0,k=0;c<PLANES;c++,k+=out_step)
{
	sum=0;temp=0;tmp_sum=0;
	for(ap_int<8> m = 0; m < F_HEIGHT; m++)
	{

		FILTER_LOOP_WIDTH:
		for(ap_int<8> n = 0; n < F_WIDTH; n++)
		{
			XF_PTNAME(DEPTH_SRC) src_v = _kernel_pixel[F_HEIGHT-m-1][F_WIDTH-1-n];

			short int filter_v = _kernel_filter[m][n];
			temp = src_v.range(p+(in_step-1),p) * filter_v;
			sum = sum + temp;

		}
	}
	p=p+8;
	tmp_sum = sum >> shift;

	if((DEPTH_DST == XF_8UP)||(DEPTH_DST == XF_24UP))
	{
		if(tmp_sum > ((1 << (8)) - 1))
		{
			out.range(k+7,k) = ((1 << (8)) - 1);
		}
		else if(tmp_sum < 0)
		{
			out.range(k+7,k) = 0;
		}
		else
		{
			out.range(k+7,k) = tmp_sum;
		}
	}
	else if((DEPTH_DST == XF_16SP)||(DEPTH_DST == XF_48SP))
	{
		if(tmp_sum > ((1 << (16)) - 1))
		{
			out.range(k+15,k) = ((1 << (16)) - 1);
		}
		else if(tmp_sum < -(1 << (16)))
		{
			out.range(k+15,k) = -(1 << (16));
		}
		else
		{
			out.range(k+15,k) = tmp_sum;
		}
	}

}

}
static int borderInterpolate( int p, int len, int borderType )
{
#pragma HLS INLINE

	if( p >= 0 && p < len )
		return p;
	else
		p = -1;
	return p;
}


template<int SRC_T, int DST_T, int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int K_HEIGHT, int K_WIDTH, int PLANES>
static void xFFilter2Dkernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat,
		short int _filter_kernel[K_HEIGHT][K_WIDTH],
		unsigned char shift, uint16_t rows, uint16_t cols)

{
	XF_SNAME(WORDWIDTH_SRC) fillvalue = 0;
#pragma HLS INLINE off

	// The main processing window
	XF_PTNAME(DEPTH_SRC) src_kernel_win[K_HEIGHT][K_WIDTH];
	// The main line buffer
	XF_SNAME(WORDWIDTH_SRC) k_buf[K_HEIGHT][COLS>>XF_BITSHIFT(NPC)];
	// A small buffer keeping a few pixels from the line
	// buffer, so that we can complete right borders correctly.
	XF_SNAME(WORDWIDTH_SRC) right_border_buf[K_HEIGHT][K_WIDTH];
	// Temporary storage for reading from the line buffers.
	XF_SNAME(WORDWIDTH_SRC) col_buf[K_HEIGHT];

	assert(rows >= 8);
	assert(cols >= 8);
	assert(rows <= ROWS);
	assert(cols <= COLS);

#pragma HLS ARRAY_PARTITION variable=col_buf complete dim=0
#pragma HLS ARRAY_PARTITION variable=&_filter_kernel complete dim=0
#pragma HLS ARRAY_PARTITION variable=src_kernel_win complete dim=0
#pragma HLS ARRAY_PARTITION variable=k_buf complete dim=1
#pragma HLS ARRAY_PARTITION variable=right_border_buf complete dim=0

	int heightloop= rows+K_HEIGHT-1+K_HEIGHT;
	int widthloop = cols+K_WIDTH-1;//one pixel overlap, so it should minus one
	/*ap_uint<13> i,j;
	ap_uint<13> anchorx=K_WIDTH/2,anchory=K_HEIGHT/2;
	ap_uint<13> ImagLocx=0,ImagLocy =0;*/

	uint16_t i,j;
	int rd_ind=0, wr_ind=0;
	uint16_t anchorx = K_WIDTH >> 1,anchory = K_HEIGHT >> 1;
	int16_t ImagLocx=0,ImagLocy =0;

	ROW_LOOP:
	for( i = 0; i < heightloop; i++) {
		COL_LOOP:
		for ( j = 0; j < widthloop;j++) {
			// This DEPENDENCE pragma is necessary because the border mode handling is not affine.
#pragma HLS DEPENDENCE array inter false
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS PIPELINE

			//fill data x,y are the coordinate in the image, it could be negative. For example (-1,-1) represents the interpolation pixel.
			ImagLocx=j-anchorx;
			ImagLocy=i-K_HEIGHT-anchory;
			int16_t x = borderInterpolate(ImagLocx, cols, 0);

			// column left shift
			for(ap_int<8> row = 0;row < K_HEIGHT; row++)
				for(ap_int<8> col = K_WIDTH-1; col >= 1; col--)
					src_kernel_win[row][col] = src_kernel_win[row][col-1];

			for(ap_int<8> buf_row = 0; buf_row < K_HEIGHT; buf_row++) {
				// Fetch the column from the line buffer to shift into the window.
				assert((x < COLS));
				col_buf[buf_row]  = ((x < 0)) ? fillvalue : k_buf[buf_row][x];
			}

			if((ImagLocy < (-anchory)) || (ImagLocy >= K_HEIGHT-1 && ImagLocy < rows-1 ))
			{
				//Advance load and body process
				if(ImagLocx >= 0 && ImagLocx < cols) {
					XF_SNAME(WORDWIDTH_SRC) Toppixel = col_buf[K_HEIGHT-1]; //k_buf[k](K_HEIGHT-1,ImagLocx);
					src_kernel_win[K_HEIGHT-1][0] = Toppixel;
					if(ImagLocx >= cols - K_WIDTH) {
						right_border_buf[0][ImagLocx-(cols-K_WIDTH)] = Toppixel;
					}
					for(ap_int<8> buf_row= K_HEIGHT-1;buf_row >= 1;buf_row--)
					{
						XF_SNAME(WORDWIDTH_SRC) temp = col_buf[buf_row-1]; //k_buf[k](buf_row-1,ImagLocx);
						src_kernel_win[buf_row-1][0]=temp;
						k_buf[buf_row][x]=temp;
						if(ImagLocx >= cols - K_WIDTH) {
							right_border_buf[K_HEIGHT-buf_row][ImagLocx-(cols-K_WIDTH)] = temp;
						}
					}
					XF_SNAME(WORDWIDTH_SRC) temp=0;
					temp = (_src_mat.read(rd_ind));
					rd_ind++;

					k_buf[0][x]=temp;
				}
				else if(ImagLocx < 0)
				{
					for(int buf_row = 0;buf_row < K_HEIGHT; buf_row++)
					{
						src_kernel_win[buf_row][0] = fillvalue;
					}
				}
				else if (ImagLocx >= cols) {
					for(int buf_row= 0;buf_row < K_HEIGHT; buf_row++)
					{
						src_kernel_win[buf_row][0] = fillvalue;
					}
				}
			} else
				if(ImagLocy >= 0) { //   && ImagLocy < K_HEIGHT-1) ||
					// (ImagLocy >= rows-1      && ImagLocy < heightloop)) {
					//top extend pixel bottom keep the buffer 0 with the data rows-1 content.
					int ref = K_HEIGHT-1;
					if(ImagLocy >= rows-1)
						ref = rows-1;
					int y = ImagLocy;
					for(int buf_row = 0;buf_row < K_HEIGHT; buf_row++) {
						int t = borderInterpolate(y, rows, 0);
						int locy = ref - t;
						assert(t < 0 || (locy >= 0 && locy < K_HEIGHT));
						if(y >= rows)
							src_kernel_win[buf_row][0] = fillvalue;
						else if( y < 0)
							src_kernel_win[buf_row][0] = fillvalue;
						else
							src_kernel_win[buf_row][0] = col_buf[locy];
						y--;
					}
				}

			// figure out the output image pixel value
			if(i >= (K_HEIGHT + K_HEIGHT - 1) && j >= (K_WIDTH-1))
			{
				XF_PTNAME(DEPTH_DST) temp;
				xFApplyFilter2D<DEPTH_SRC,DEPTH_DST,K_HEIGHT,K_WIDTH,PLANES>(src_kernel_win,_filter_kernel,temp,shift);
				XF_SNAME(WORDWIDTH_DST) temp1 = temp;
				_dst_mat.write(wr_ind,temp1);
				wr_ind++;

			}
		}
	}
}


#pragma SDS data data_mover("_src_mat.data":FASTDMA,"_dst_mat.data":FASTDMA)
#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL,filter:RANDOM,"_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])

template<int BORDER_TYPE,int FILTER_WIDTH,int FILTER_HEIGHT, int SRC_T,int DST_T, int ROWS, int COLS,int NPC>
void filter2D(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat,short int filter[FILTER_HEIGHT*FILTER_WIDTH],unsigned char _shift)
{

#pragma HLS INLINE OFF

	assert(((_src_mat.rows <= ROWS ) && (_src_mat.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	unsigned short img_width = _src_mat.cols >>XF_BITSHIFT(NPC);
	unsigned short img_height = _src_mat.rows;

	short int lfilter[FILTER_HEIGHT][FILTER_WIDTH];
	#pragma HLS ARRAY_PARTITION variable=lfilter complete dim=0
		for(unsigned char i = 0; i < FILTER_HEIGHT; i++)
		{
			for(unsigned char j = 0; j < FILTER_WIDTH; j++)
			{
				lfilter[i][j]=filter[i*FILTER_WIDTH+j];
			}
		}


	if(NPC==XF_NPPC8){

		xFCustomConvolutionKernel<SRC_T, DST_T,ROWS,COLS,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),(COLS >> XF_BITSHIFT(NPC)),
				FILTER_HEIGHT, FILTER_WIDTH, (XF_NPIXPERCYCLE(NPC) - ((FILTER_WIDTH >> 1) % XF_NPIXPERCYCLE(NPC))),
				(( FILTER_WIDTH >> 1) % XF_NPIXPERCYCLE(NPC)), (((FILTER_WIDTH >> 1) - 1) >> XF_BITSHIFT(NPC))>
		(_src_mat,lfilter,_dst_mat,_shift,img_width,img_height);

	}

	else if(NPC==XF_NPPC1)
	{
		xFFilter2Dkernel<SRC_T, DST_T, ROWS,COLS,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),COLS,FILTER_HEIGHT,FILTER_WIDTH, XF_CHANNELS(SRC_T,NPC)>
		(_src_mat,_dst_mat,lfilter,_shift,img_height,img_width);
	}


	}
}
#endif // _XF_CUSTOM_CONVOLUTION_HPP_


