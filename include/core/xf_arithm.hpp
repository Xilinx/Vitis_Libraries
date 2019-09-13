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

#ifndef _XF_ARITHM_HPP_
#define _XF_ARITHM_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif
#include "hls_stream.h"
#include "common/xf_common.h"
/**
 * xFAbsDiff: Computes the absolute difference between two images
 * Inputs: _src1, _src2
 * Output: _dst
 */
class kernel_add {
public:
	template<int DEPTH>
    static void apply(XF_PTNAME(DEPTH)& p, XF_PTNAME(DEPTH)& q, XF_PTNAME(DEPTH)& r,int _policytype) {
#pragma HLS inline
		// for the input type of 8U
		if((DEPTH == XF_8UP) ||(DEPTH == XF_24UP))
		{
			ap_uint<(XF_PIXELDEPTH(DEPTH)+1)> result_temp;
			result_temp = p+ q; // perform the addition operation on the input pixels
			if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp > 255)// handling the overflow
			{
				result_temp = 255;
			}
			r = (XF_PTNAME(DEPTH)) result_temp;
		}

		// for the input type of 16S
		else if((DEPTH == XF_16SP)||(DEPTH == XF_48SP))
		{
			ap_int<17> result_temp;
			result_temp = p + q; // perform the addition operation on the input pixels
			if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp > 32767)			// handling the overflow
			{
				result_temp = 32767;
			}
			else if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp < -32768)		// handling the overflow
			{
				result_temp = -32768;
			}
			r = (XF_PTNAME(DEPTH)) result_temp;
		}


    }
};
class kernel_sub {
public:
	template<int DEPTH>
    static void apply(XF_PTNAME(DEPTH)& p, XF_PTNAME(DEPTH)& q, XF_PTNAME(DEPTH)& r,int _policytype) {
#pragma HLS inline
		// for the input type of 8U
		if((DEPTH == XF_8UP) ||(DEPTH == XF_24UP))
		{
			ap_int<(XF_PIXELDEPTH(DEPTH)+1)> result_temp;
			result_temp = p - q; // perform the subtraction operation on the input pixels
			if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp < 0)// handling the overflow
			{
				result_temp = 0;
			}
			r = (XF_PTNAME(DEPTH)) result_temp;
		}

		// for the input type of 16S
		else if((DEPTH == XF_16SP)||(DEPTH == XF_48SP))
		{
			ap_int<(XF_PIXELDEPTH(DEPTH)+1)> result_temp;
			result_temp = p - q; // perform the addition operation on the input pixels
			if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp > 32767)			// handling the overflow
			{
				result_temp = 32767;
			}
			else if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp < -32768)		// handling the overflow
			{
				result_temp = -32768;
			}
			r = (XF_PTNAME(DEPTH)) result_temp;
		}


    }
};
class kernel_subRS {
public:
	template<int DEPTH>
    static void apply(XF_PTNAME(DEPTH)& p, XF_PTNAME(DEPTH)& q, XF_PTNAME(DEPTH)& r,int _policytype) {
#pragma HLS inline
		// for the input type of 8U
		if((DEPTH == XF_8UP) ||(DEPTH == XF_24UP))
		{
			ap_int<(XF_PIXELDEPTH(DEPTH)+1)> result_temp;
			result_temp = q - p; // perform the subtraction operation on the input pixels
			if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp < 0)// handling the overflow
			{
				result_temp = 0;
			}
			r = (XF_PTNAME(DEPTH)) result_temp;
		}

		// for the input type of 16S
		else if((DEPTH == XF_16SP)||(DEPTH == XF_48SP))
		{
			ap_int<17> result_temp;
			result_temp = q - p; // perform the subtraction operation on the input pixels
			if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp < 32767)			// handling the overflow
			{
				result_temp = 32767;
			}
			else if(_policytype == XF_CONVERT_POLICY_SATURATE && result_temp < -32768)		// handling the overflow
			{
				result_temp = -32768;
			}
			r = (XF_PTNAME(DEPTH)) result_temp;
		}


    }
};
/* Finding the maximum intensity pixel between two input pixels*/
class kernel_max{
public:
	template<int DEPTH>
    static void apply(XF_PTNAME(DEPTH)& p, XF_PTNAME(DEPTH)& q, XF_PTNAME(DEPTH)& r,int _policytype) {
#pragma HLS inline
		XF_PTNAME(DEPTH) Max=0;
		if(p > q)
		{
			Max=p;
		}
		else
		{
			Max=q;
		}
			r = (XF_PTNAME(DEPTH)) Max;
   }
};
/* Finding the minimum intensity pixel between two input pixels*/

class kernel_min{
public:
	template<int DEPTH>
    static void apply(XF_PTNAME(DEPTH)& p, XF_PTNAME(DEPTH)& q, XF_PTNAME(DEPTH)& r,int _policytype) {
#pragma HLS inline
		XF_PTNAME(DEPTH) Min=0;
		if(p < q)
		{
			Min=p;
		}
		else
		{
			Min=q;
		}
			r = (XF_PTNAME(DEPTH)) Min;
   }
};
/* performing comparision between two pixels*/

class kernel_compare{
public:
	template<int DEPTH>
	static void apply(XF_PTNAME(DEPTH)& p, XF_PTNAME(DEPTH)& q, XF_PTNAME(DEPTH)& r,int comp_op) {
#pragma HLS inline
		XF_PTNAME(DEPTH) Min=0;
		switch(comp_op)
		{
		case XF_CMP_EQ: r = (p==q ? 255 : 0);	//equal
		break;	
		case XF_CMP_GT: r = (p >q ? 255 : 0);	//greater than
		break;
		case XF_CMP_GE: r = (p>=q ? 255 : 0);	//greater than or equal
		break;
		case XF_CMP_LT: r = (p <q ? 255 : 0);	//less than 
		break;
		case XF_CMP_LE: r = (p<=q ? 255 : 0);	//less than or equal
		break;
		case XF_CMP_NE: r = (p!=q ? 255 : 0);	//not equal
		break;
		default:
			break;
		}
	}
};
class kernel_set{
public:
	template<int DEPTH>
	static void apply(XF_PTNAME(DEPTH)& p, XF_PTNAME(DEPTH)& q, XF_PTNAME(DEPTH)& r,int comp_op) {
#pragma HLS inline

		r = (XF_PTNAME(DEPTH))q;
	}
};
class kernel_zero{
public:
	template<int DEPTH>
	static void apply(XF_PTNAME(DEPTH)& p, XF_PTNAME(DEPTH)& q, XF_PTNAME(DEPTH)& r,int comp_op) {
#pragma HLS inline

		r = 0;
	}
};
namespace xf {

template<int SRC_T, int ROWS, int COLS,int PLANES, int DEPTH, int NPC, int WORDWIDTH_SRC,
		int WORDWIDTH_DST, int COLS_TRIP>
void xFAbsDiffKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst,
		uint16_t image_height, uint16_t image_width)
{
//	image_width=image_width>>XF_BITSHIFT(NPC);
	ap_uint <13> i,j,k;

	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	uchar_t result, p, q;
int STEP= XF_PIXELDEPTH(DEPTH)/PLANES;
	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read(i*image_width+j)); // reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read(i*image_width+j));// reading the data from the second stream


			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src1.range(k + (STEP-1), k); // Get bits from certain range of positions.
				q = val_src2.range(k + (STEP-1), k);// Get bits from certain range of positions.
				result = __ABS(p - q);// performing absolute difference for the input pixels
				val_dst.range(k + (STEP-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(i*image_width+j,(val_dst));    // writing data to the output stream
		}
	}
}

/**
 * xFBitwiseAND: Performs bitwise AND between two XF_8UP images
 * Inputs: _src1, _src2
 * Output: _dst
 */
template<int SRC_T, int ROWS, int COLS,int PLANES, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST, int COLS_TRIP>
void xFBitwiseANDKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst,
		uint16_t image_height,uint16_t image_width)
{

	ap_uint <13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p, q;
int STEP=XF_PIXELDEPTH(DEPTH)/PLANES;
	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read(i*image_width+j));//(_src1.data[i*image_width+j]); // reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read(i*image_width+j));// reading the data from the second stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += STEP)
			{
#pragma HLS unroll
				p = val_src1.range(k + (STEP-1), k); // Get bits from certain range of positions.
				q = val_src2.range(k + (STEP-1), k);// Get bits from certain range of positions.
				result = p & q;// performing the bitwiseAND operation
				val_dst.range(k + (STEP-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(i*image_width+j, (val_dst));		// writing data to the stream
		}
	}
}

///**
// * xFBitwiseOR: Performs bitwise OR between two XF_8UP images
// * Inputs: _src1, _src2
// * Output: _dst
// */
template<int SRC_T, int ROWS, int COLS,int PLANES, int DEPTH, int NPC, int WORDWIDTH_SRC,
		int WORDWIDTH_DST, int COLS_TRIP>
void xFBitwiseORKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst,
		uint16_t image_height,uint16_t image_width)
{

	ap_uint<13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p, q;
int STEP=XF_PIXELDEPTH(DEPTH)/PLANES;
	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read(i*image_width+j));//(_src1.data[i*image_width+j]);// reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read(i*image_width+j));//(_src2.data[i*image_width+j]);// reading the data from the second stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));k += STEP)
			{
#pragma HLS unroll
				p = val_src1.range(k + (STEP-1), k);// Get bits from certain range of positions.
				q = val_src2.range(k + (STEP-1), k);// Get bits from certain range of positions.
				result = p | q;// performing the bitwiseOR operation
				val_dst.range(k + (STEP-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(i*image_width+j,(val_dst));  		// write data to the stream
		}
	}
}

/**
 * xFBitwiseNOT: Performs bitwise NOT for a XF_8UP image
 * Inputs: _src
 * Output: _dst
 */
template<int SRC_T, int ROWS, int COLS, int PLANES, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST, int COLS_TRIP>
void xFBitwiseNOTKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst,
		uint16_t image_height,uint16_t image_width)
{

	ap_uint<13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p;
int STEP=XF_PIXELDEPTH(DEPTH)/PLANES;
	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src = (XF_SNAME(WORDWIDTH_SRC)) (_src.read(i*image_width+j)); // reading the data from the stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += STEP)
			{
#pragma HLS unroll
				p = val_src.range(k + (STEP-1), k);	// Get bits from certain range of positions.
				result = ~p;// performing the bitwiseNOT operation
				val_dst.range(k + (STEP-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(i*image_width+j, (val_dst));			// write data to the stream
		}
	}
}

/**
 * xFBitwiseXOR: Performs bitwise XOR between two XF_8UP images
 * Inputs: _src1, _src2
 * Output: _dst
 */
template<int SRC_T, int ROWS, int COLS, int PLANES,int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST, int COLS_TRIP>
void xFBitwiseXORKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2, xf::Mat<SRC_T, ROWS, COLS, NPC> &_dst,
		uint16_t image_height,uint16_t image_width)
{

	ap_uint <13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p, q;
int STEP=XF_PIXELDEPTH(DEPTH)/PLANES;
	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read(i*image_width+j));// reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read(i*image_width+j));// reading the data from the second stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += STEP)
			{
#pragma HLS unroll
				p = val_src1.range(k + (STEP-1), k);// Get bits from certain range of positions.
				q = val_src2.range(k + (STEP-1), k);// Get bits from certain range of positions.
				result = p ^ q;// performing the bitwise XOR operation
				val_dst.range(k + (STEP-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write((i*image_width+j),(val_dst));  	  	// write data to the stream
		}
	}
}

/**
 * xFMul : Performs element-wise multiplication between two images and a scalar value
 * Inputs: _src1, _src2, _policytype, _scale_val
 * Output: _dst
 */
template<int SRC_T, int ROWS, int COLS, int PLANES,int DEPTH, int NPC, int WORDWIDTH_SRC,
		int WORDWIDTH_DST, int COLS_TRIP>
void xFMulKernel(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst,
int _policytype, float _scale_val,uint16_t image_height,uint16_t image_width)
{

	int STEP= XF_PIXELDEPTH(DEPTH)/PLANES;
	ap_uint <13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;

	XF_PTNAME(DEPTH) result, p ,q;
	int64_t result_temp;
	uint16_t scale_value_8;
	uint32_t scale_value_16;
	if((DEPTH == XF_8UP) ||(DEPTH == XF_24UP))
	{
		scale_value_8 = (_scale_val * ( (1 << 15) -1 )); // floating point value taken in fixed point format (Q1.15)
	}
	else if((DEPTH == XF_16SP)||(DEPTH == XF_48SP))
	{
		scale_value_16 = (_scale_val * ( (1 << 24) -1 )); // floating point value taken in fixed point format (Q1.24)
	}
unsigned long long int idx=0;
	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (src1.read(i*image_width+j)); // reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (src2.read(i*image_width+j));// reading the data from the second stream
			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += STEP)
			{
#pragma HLS unroll
				p = val_src1.range(k + (STEP - 1), k); // Get bits from certain range of positions.
				q = val_src2.range(k + (STEP - 1), k);// Get bits from certain range of positions.

				// for the input type of 8U
				if((DEPTH == XF_8UP)||(DEPTH == XF_24UP))
				{
					result_temp = (scale_value_8 * p * q ) >> 15; // performing pixel-wise multiplication with scale value
					if(_policytype == XF_CONVERT_POLICY_SATURATE &&
					result_temp > 255)// handling the overflow
					{
						result_temp = 255;
					}
					result = (uchar_t) result_temp;
				}

				// for the input type of 16S
				else
				{
					result_temp = (scale_value_16 * p * q ) >> 24; // performing pixel-wise multiplication with scale value
					if(_policytype==XF_CONVERT_POLICY_SATURATE &&
					result_temp > 32767)// handling the overflow
					{
						result_temp = 32767;
					}
					else if(_policytype==XF_CONVERT_POLICY_SATURATE && result_temp < -32768)
					{
						result_temp = -32768;
					}
					result = (int16_t) result_temp;
				}
				val_dst.range(k + (STEP - 1), k) = result; // Set bits in a range of positions.
			}
			dst.write(i*image_width+j, val_dst);		// write data to the stream
		}
	}
}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC =1>
void absdiff(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst) {

#pragma HLS inline off

	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

//	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
//			"NPC must be XF_NPPC1 or XF_NPPC8 ");
//	assert(((SRC_T == XF_8UC1) ) &&
//			"TYPE must be XF_8UC1 ");
//	assert(((_src1.rows <= ROWS ) && (_src1.cols <= COLS) && (_src2.rows <= ROWS ) && (_src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");

	xFAbsDiffKernel<SRC_T,ROWS,COLS,XF_CHANNELS(SRC_T,NPC),XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC))>
	(_src1,_src2,_dst,_src1.rows,image_width);

}







#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void bitwise_and(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2, xf::Mat<SRC_T, ROWS, COLS, NPC> &_dst)
{

#pragma HLS inline off

//	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
//			"NPC must be XF_NPPC1 or XF_NPPC8 ");
//	assert(((SRC_T == XF_8UC1) ) &&
//			"Image type must be XF_8UC1 ");
//	assert(((_src1.rows <= ROWS ) && (_src1.cols <= COLS) && (_src2.rows <= ROWS ) && (_src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");

	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	xFBitwiseANDKernel<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC),XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC))>
	(_src1,_src2,_dst,_src1.rows,image_width);
}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void bitwise_or(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
//	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
//			"NPC must be XF_NPPC1 or XF_NPPC8 ");
//	assert(((SRC_T == XF_8UC1) ) &&
//			"Image type must be XF_8UC1 ");
//	assert(((_src1.rows <= ROWS ) && (_src1.cols <= COLS) && (_src2.rows <= ROWS ) && (_src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");

	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	xFBitwiseORKernel<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC))>
	(_src1,_src2,_dst,_src1.rows,image_width);

}

#pragma SDS data access_pattern("src.data":SEQUENTIAL)
#pragma SDS data copy("src.data"[0:"src.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void bitwise_not(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)
{

//	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
//			"NPC must be XF_NPPC1 or XF_NPPC8 ");
//	assert(((SRC_T == XF_8UC1) ) &&
//			"Image type must be XF_8UC1 ");
//	assert(((src.rows <= ROWS ) && (src.cols <= COLS) ) && "ROWS and COLS should be greater than input image");

	uint16_t image_width = src.cols >> XF_BITSHIFT(NPC);
#pragma HLS inline off

	xFBitwiseNOTKernel<SRC_T,ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC))>
	(src,dst,src.rows,image_width);

}


#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void bitwise_xor(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)
{

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) ) &&
			"Image type must be XF_8UC1 ");
	assert(((src1.rows <= ROWS ) && (src1.cols <= COLS) && (src2.rows <= ROWS ) && (src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");

#pragma HLS inline off

	uint16_t image_width = src1.cols >> XF_BITSHIFT(NPC);

	xFBitwiseXORKernel<SRC_T,ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC),
		(COLS>>XF_BITSHIFT(NPC))>(src1,src2,dst,src1.rows,image_width);

}

#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC = 1>
void multiply(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst,float scale)
{

#pragma HLS inline off

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) || (SRC_T == XF_16SC1) || (SRC_T == XF_8UC3) || (SRC_T == XF_16SC3) || (SRC_T == XF_8UC4) || (SRC_T == XF_16UC4) ) &&
			"TYPE must be XF_8UC1 or XF_16SC1 ");
	assert((POLICY_TYPE == XF_CONVERT_POLICY_SATURATE ||
			POLICY_TYPE == XF_CONVERT_POLICY_TRUNCATE)
			&& "_policytype must be 'XF_CONVERT_POLICY_SATURATE' or 'XF_CONVERT_POLICY_TRUNCATE'");
	assert(((scale >= 0) && (scale <= 1)) &&
			"_scale_val must be within the range of 0 to 1");
	assert(((src1.rows <= ROWS ) && (src1.cols <= COLS) && (src2.rows <= ROWS ) && (src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");

	uint16_t image_width = src1.cols >> XF_BITSHIFT(NPC);

	xFMulKernel<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC),(COLS>>XF_BITSHIFT(NPC))>
	(src1,src2,dst,POLICY_TYPE,scale,src1.rows,image_width);

}


/**
 * xFAdd: Adds the pixels of two input XF_8UP or XF_16SP images and generates the
 * 		  resultant image.
 * Inputs: _src1, _src2, _policytype
 * Output: _dst
 */


template<int SRC_T, int ROWS, int COLS, int PLANES,int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST, int COLS_TRIP,typename KERNEL, int USE_SRC2=0>
void xFarithm_proc(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2, xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char> scl, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst,
int _policytype,uint16_t image_height,uint16_t image_width)
{
	KERNEL opr;
	int STEP;
	STEP=XF_PIXELDEPTH(DEPTH)/PLANES;
	ap_uint<13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1=0, val_src2=0;
	XF_SNAME(WORDWIDTH_DST) val_dst=0;
	XF_PTNAME(DEPTH) result, p=0 ,q=0;

	rowLoop:
	for(i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read(i*image_width+j)); // reading the data from the first stream
			if (USE_SRC2) {
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read(i*image_width+j));
			}
			procLoop:
			for(k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));k += STEP)
			{
#pragma HLS unroll
				p = val_src1.range(k + (STEP - 1), k); // Get bits from certain range of positions.
				if (USE_SRC2) {
					q = val_src2.range(k + (STEP - 1), k);
				} else {
				   q = scl.val[0];
				}

				opr.template apply<DEPTH>(p, q, result, _policytype);

				val_dst.range(k + (STEP - 1), k) = result; // Set bits in a range of positions.
			}
			_dst.write(i*image_width+j, val_dst);			// writing data to the output stream
		}
	}
}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>
void add(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) || (SRC_T == XF_16SC1) || (SRC_T == XF_8UC3) || (SRC_T == XF_8UC4) ) &&
			"TYPE must be XF_8UC1 or XF_16SC1");
	assert((POLICY_TYPE == XF_CONVERT_POLICY_SATURATE ||
			POLICY_TYPE == XF_CONVERT_POLICY_TRUNCATE)
			&& "_policytype must be 'XF_CONVERT_POLICY_SATURATE' or 'XF_CONVERT_POLICY_TRUNCATE'");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");


	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_add,1>
	(_src1,_src2,0,_dst,POLICY_TYPE,_src1.rows,image_width);

}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  addS API call*/
template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>
void addS(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, unsigned char _scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) || (SRC_T == XF_16SC1) ) &&
			"TYPE must be XF_8UC1 or XF_16SC1 ");
	assert((POLICY_TYPE == XF_CONVERT_POLICY_SATURATE ||
			POLICY_TYPE == XF_CONVERT_POLICY_TRUNCATE)
			&& "_policytype must be 'XF_CONVERT_POLICY_SATURATE' or 'XF_CONVERT_POLICY_TRUNCATE'");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");

	xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>  scl;
	for(int i=0; i<XF_CHANNELS(SRC_T,NPC); i++)
	{
		scl.val[i]=_scl[i];
	}

	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_add,0>
	(_src1,_src1,scl,_dst,POLICY_TYPE,_src1.rows,image_width);

}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>
void SubS(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,  unsigned char _scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) || (SRC_T == XF_16SC1) ) &&
			"TYPE must be XF_8UC1 or XF_16SC1 ");
	assert((POLICY_TYPE == XF_CONVERT_POLICY_SATURATE ||
			POLICY_TYPE == XF_CONVERT_POLICY_TRUNCATE)
			&& "_policytype must be 'XF_CONVERT_POLICY_SATURATE' or 'XF_CONVERT_POLICY_TRUNCATE'");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");
	xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>  scl;
	for(int i=0; i<XF_CHANNELS(SRC_T,NPC); i++)
	{
		scl.val[i]=_scl[i];
	}

	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_sub, 0>
	(_src1,_src1,scl,_dst,POLICY_TYPE,_src1.rows,image_width);

}
#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  subRS API call*/
template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>
void SubRS(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, unsigned char _scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) || (SRC_T == XF_16SC1) ) &&
			"TYPE must be XF_8UC1 or XF_16SC1 ");
	assert((POLICY_TYPE == XF_CONVERT_POLICY_SATURATE ||
			POLICY_TYPE == XF_CONVERT_POLICY_TRUNCATE)
			&& "_policytype must be 'XF_CONVERT_POLICY_SATURATE' or 'XF_CONVERT_POLICY_TRUNCATE'");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");

	xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>  scl;
	for(int i=0; i<XF_CHANNELS(SRC_T,NPC); i++)
	{
		scl.val[i]=_scl[i];
	}

	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_subRS, 0>
	(_src1,_src1,scl,_dst,POLICY_TYPE,_src1.rows,image_width);

}
#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  subtract API call*/
template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>
void subtract(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) || (SRC_T == XF_16SC1) || (SRC_T == XF_8UC3) || (SRC_T == XF_16SC3) || (SRC_T == XF_8UC4) || (SRC_T == XF_16UC4)) &&
			"TYPE must be XF_8UC1 or 16SC1 ");
	assert((POLICY_TYPE == XF_CONVERT_POLICY_SATURATE ||
			POLICY_TYPE == XF_CONVERT_POLICY_TRUNCATE)
			&& "_policytype must be 'XF_CONVERT_POLICY_SATURATE' or 'XF_CONVERT_POLICY_TRUNCATE'");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");


	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_sub, 1>
	(_src1,_src2,0,_dst,POLICY_TYPE,_src1.rows,image_width);

}
#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  MaxS API call*/
template< int SRC_T, int ROWS, int COLS, int NPC =1>
void max(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,  unsigned char _scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) ) &&
			"TYPE must be XF_8UC1 ");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");
	xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>  scl;
	for(int i=0; i<XF_CHANNELS(SRC_T,NPC); i++)
	{
		scl.val[i]=_scl[i];
	}

	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_max,0>
	(_src1,_src1,scl,_dst,0,_src1.rows,image_width);

}


#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  Max API call*/
template<int SRC_T, int ROWS, int COLS, int NPC =1>
void max(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) ) &&
			"TYPE must be XF_8UC1 ");

	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");


	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_max,1>
	(_src1,_src2,0,_dst,0,_src1.rows,image_width);

}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  MinS API call*/
template< int SRC_T, int ROWS, int COLS, int NPC =1>
void min(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,  unsigned char _scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1)) &&
			"TYPE must be XF_8UC1 ");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");

	xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>  scl;
	for(int i=0; i<XF_CHANNELS(SRC_T,NPC); i++)
	{
		scl.val[i]=_scl[i];
	}

	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_min,0>
	(_src1,_src1,scl,_dst,0,_src1.rows,image_width);

}


#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  Min API call*/
template< int SRC_T, int ROWS, int COLS, int NPC =1>
void min(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1)) &&
			"TYPE must be XF_8UC1");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");


	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_min,1>
	(_src1,_src2,0,_dst,0,_src1.rows,image_width);

}
#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  CompareS API call*/
template<int CMP_OP, int SRC_T, int ROWS, int COLS, int NPC =1>
void compare(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, unsigned char _scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) ) &&
			"TYPE must be XF_8UC1");

	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");

	xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>  scl;
	for(int i=0; i<XF_CHANNELS(SRC_T,NPC); i++)
	{
		scl.val[i]=_scl[i];
	}

	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_compare,0>
	(_src1,_src1,scl,_dst,CMP_OP,_src1.rows,image_width);

}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/*  Compare API call*/
template<int CMP_OP, int SRC_T, int ROWS, int COLS, int NPC =1>
void compare(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & _src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1)) &&
			"TYPE must be XF_8UC1");

	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");


	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_compare,1>
	(_src1,_src2,0,_dst,CMP_OP,_src1.rows,image_width);

}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/* set API call*/
template< int SRC_T, int ROWS, int COLS, int NPC =1>
void set(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,  unsigned char _scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1)) &&
			"TYPE must be XF_8UC1  ");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");

	xf::Scalar<XF_CHANNELS(SRC_T,NPC), unsigned char>  scl;
	for(int i=0; i<XF_CHANNELS(SRC_T,NPC); i++)
	{
		scl.val[i]=_scl[i];
	}

	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_set,0>
	(_src1,_src1,scl,_dst,0,_src1.rows,image_width);

}
#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
/* Zero API call*/
template< int SRC_T, int ROWS, int COLS, int NPC =1>
void zero(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src1,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

#pragma HLS inline off
	uint16_t image_width = _src1.cols >> XF_BITSHIFT(NPC);

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((SRC_T == XF_8UC1) ) &&
			"TYPE must be XF_8UC1");
	assert((_src1.rows <= ROWS ) && "ROWS and COLS should be greater than input image");


	xFarithm_proc<SRC_T, ROWS,COLS,XF_CHANNELS(SRC_T,NPC), XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC), (COLS>>XF_BITSHIFT(NPC)),kernel_zero,0>
	(_src1,_src1,0,_dst,0,_src1.rows,image_width);

}
}//namespace
#endif
