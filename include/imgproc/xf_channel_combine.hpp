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

#ifndef _XF_CHANNEL_COMBINE_HPP_
#define _XF_CHANNEL_COMBINE_HPP_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{

#define XF_STEP_NEXT 8


/********************************************************************
 * 	ChannelCombine: combine multiple 8-bit planes into one
 *******************************************************************/
template<int ROWS, int COLS, int SRC_T, int DST_T, int NPC, int TC>
void xfChannelCombineKernel(
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in1,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in2,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in3,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in4,
		hls::stream< XF_TNAME(DST_T,NPC) >& _out,uint16_t height,uint16_t width)
{

	XF_TNAME(SRC_T,NPC) val1, val2, val3, val4;

	uchar_t channel1, channel2, channel3, channel4;

const int noofbits = XF_DTPIXELDEPTH(SRC_T, NPC);
ap_uint<13> i,j,k;
	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			XF_TNAME(DST_T,NPC) res;

			val1 = (XF_TNAME(SRC_T,NPC))(_in1.read());
			val2 = (XF_TNAME(SRC_T,NPC))(_in2.read());
			val3 = (XF_TNAME(SRC_T,NPC))(_in3.read());
			val4 = (XF_TNAME(SRC_T,NPC))(_in4.read());

			ProcLoop:
			for (k = 0; k < (noofbits<<XF_BITSHIFT(NPC)); k += noofbits)
			{
#pragma HLS UNROLL
				int y = k * XF_CHANNELS(DST_T,NPC);
				channel1 = val1.range(k+(noofbits-1), k);	// B
				channel2 = val2.range(k+(noofbits-1), k);	// G
				channel3 = val3.range(k+(noofbits-1), k);	// R
				channel4 = val4.range(k+(noofbits-1), k);	// A

				uint32_t result = ((uint32_t)channel3 << 0)  |
						((uint32_t)channel2 << noofbits) |
						((uint32_t)channel1 << noofbits*2)|
						((uint32_t)channel4 << noofbits*3);

				res.range(y+(XF_PIXELWIDTH(DST_T,NPC)-1), y) = result;
			}//ProcLoop
			_out.write((XF_TNAME(DST_T,NPC))res);
		}//ColLoop
	}//RowLoop
}

template<int ROWS, int COLS, int SRC_T, int DST_T, int NPC, int TC>
void xfChannelCombineKernel(
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in1,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in2,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in3,
		hls::stream< XF_TNAME(DST_T,NPC) >& _out,uint16_t height,uint16_t width)
{
	XF_TNAME(SRC_T,NPC) val1, val2, val3;
	uchar_t channel1, channel2, channel3;
	const int noofbits = XF_DTPIXELDEPTH(SRC_T, NPC);
	int rows = height, cols = width;

	RowLoop:
	for(int i = 0; i < rows; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < cols; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			XF_TNAME(DST_T,NPC) res;

			val1 = (XF_TNAME(SRC_T,NPC))(_in1.read());
			val2 = (XF_TNAME(SRC_T,NPC))(_in2.read());
			val3 = (XF_TNAME(SRC_T,NPC))(_in3.read());

			ProcLoop:
			for (int k = 0; k < (noofbits<<XF_BITSHIFT(NPC)); k += noofbits)
			{
#pragma HLS UNROLL
				int y = k * XF_CHANNELS(DST_T,NPC);
				channel1 = val1.range(k+(noofbits-1), k);	// B
				channel2 = val2.range(k+(noofbits-1), k);	// G
				channel3 = val3.range(k+(noofbits-1), k);	// R

				uint32_t result = (((uint32_t)channel3 << 0)  | ((uint32_t)channel2 << noofbits) |
						((uint32_t)channel1 << noofbits*2) );

				res.range(y+(XF_PIXELWIDTH(DST_T,NPC)-1), y) = result;
			}
			_out.write((XF_TNAME(DST_T,NPC))res);
		}
	}
}

template<int ROWS, int COLS, int SRC_T, int DST_T, int NPC, int TC>
void xfChannelCombineKernel(
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in1,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in2,
		hls::stream< XF_TNAME(DST_T,NPC) >& _out,uint16_t height,uint16_t width)
{

	XF_TNAME(SRC_T,NPC) val1, val2;
	uchar_t channel1, channel2;
	const int noofbits = XF_DTPIXELDEPTH(SRC_T, NPC);
	int rows = height, cols = width;

	RowLoop:
	for(int i = 0; i < rows; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < cols; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			XF_TNAME(DST_T,NPC) res;

			val1 = (XF_TNAME(SRC_T,NPC))(_in1.read());
			val2 = (XF_TNAME(SRC_T,NPC))(_in2.read());

			ProcLoop:
			for (int k = 0; k < (noofbits<<XF_BITSHIFT(NPC)); k += noofbits)
			{
#pragma HLS UNROLL
				int y = k * XF_CHANNELS(DST_T,NPC);
				channel1 = val1.range(k+(noofbits-1), k);	// B
				channel2 = val2.range(k+(noofbits-1), k);	// G

				uint32_t result = ((uint32_t)channel1 << 0)  | ((uint32_t)channel2 << noofbits);
				res.range(y+(XF_PIXELWIDTH(DST_T,NPC)-1), y) = result;
			}
			_out.write((XF_TNAME(DST_T,NPC))res);
		}
	}
}

template<int ROWS, int COLS, int SRC_T, int DST_T, int NPC>
void xfChannelCombine(
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in1,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in2,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in3,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in4,
		hls::stream< XF_TNAME(DST_T,NPC) >& _out,uint16_t height,uint16_t width)
{

	width=width>>(XF_BITSHIFT(NPC));


		 xfChannelCombineKernel<ROWS,COLS,SRC_T,DST_T,NPC,(COLS>>(XF_BITSHIFT(NPC)))>
		 	 	 (_in1, _in2, _in3, _in4, _out, height, width);


}

template<int ROWS, int COLS, int SRC_T, int DST_T, int NPC>
void xfChannelCombine(
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in1,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in2,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in3,
		hls::stream< XF_TNAME(DST_T,NPC) >& _out,uint16_t height,uint16_t width)
{

	width=width>>(XF_BITSHIFT(NPC));


		 xfChannelCombineKernel<ROWS,COLS,SRC_T,DST_T,NPC,(COLS>>(XF_BITSHIFT(NPC)))>
		 	 	 (_in1, _in2, _in3, _out, height, width);


}

template<int ROWS, int COLS, int SRC_T, int DST_T, int NPC>
void xfChannelCombine(
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in1,
		hls::stream< XF_TNAME(SRC_T,NPC) >& _in2,
		hls::stream< XF_TNAME(DST_T,NPC) >& _out,uint16_t height,uint16_t width)
{

	width=width>>(XF_BITSHIFT(NPC));


		 xfChannelCombineKernel<ROWS,COLS,SRC_T,DST_T,NPC,(COLS>>(XF_BITSHIFT(NPC)))>
		 	 	 (_in1, _in2, _out, height, width);


}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void merge(xf::Mat<SRC_T, ROWS, COLS, NPC> &_src1, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src2, xf::Mat<DST_T, ROWS, COLS, NPC> &_dst){

	assert(((_src1.rows <= ROWS ) && (_src1.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_src2.rows <= ROWS ) && (_src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_dst.rows <= ROWS ) && (_dst.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert((SRC_T == XF_8UC1) && (DST_T == XF_8UC2) && "Source image should be of 1 channel and destination image of 2 channels");
//	assert(((NPC == XF_NPPC1)) && "NPC must be XF_NPPC1");

		hls::stream< XF_TNAME(SRC_T, NPC) > _in1;
		hls::stream< XF_TNAME(SRC_T, NPC) > _in2;
		hls::stream< XF_TNAME(DST_T, NPC) > _out;

#pragma HLS inline off
#pragma HLS DATAFLOW

	Read_Mat_Loop:
	for(int i=0; i<_src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			_in1.write( (_src1.read( i*(_src1.cols>>(XF_BITSHIFT(NPC))) +j) ));
			_in2.write( (_src2.read( i*(_src2.cols>>(XF_BITSHIFT(NPC))) +j) ) );
		}
	}

	xfChannelCombine<ROWS, COLS, SRC_T, DST_T, NPC>(_in1, _in2, _out, _src1.rows, _src1.cols);

	Write_Mat_Loop:
	for(int i=0; i<_dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			XF_TNAME(DST_T,NPC) outpix = _out.read();
			_dst.write( i*(_dst.cols>>(XF_BITSHIFT(NPC))) +j, outpix);
		}
	}
}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src3.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data copy("_src3.data"[0:"_src3.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void merge(xf::Mat<SRC_T, ROWS, COLS, NPC> &_src1, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src2, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src3, xf::Mat<DST_T, ROWS, COLS, NPC> &_dst){

	assert(((_src1.rows <= ROWS ) && (_src1.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_src2.rows <= ROWS ) && (_src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_src3.rows <= ROWS ) && (_src3.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_dst.rows <= ROWS ) && (_dst.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert((SRC_T == XF_8UC1) && (DST_T == XF_8UC3) && "Source image should be of 1 channel and destination image of 3 channels");
//	assert(((NPC == XF_NPPC1)) && "NPC must be XF_NPPC1");

		hls::stream< XF_TNAME(SRC_T, NPC) > _in1;
		hls::stream< XF_TNAME(SRC_T, NPC) > _in2;
		hls::stream< XF_TNAME(SRC_T, NPC) > _in3;
		hls::stream< XF_TNAME(DST_T, NPC) > _out;

#pragma HLS inline off
#pragma HLS DATAFLOW

	Read_Mat_Loop:
	for(int i=0; i<_src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			_in1.write( (_src1.read( i*(_src1.cols>>(XF_BITSHIFT(NPC))) +j) ));
			_in2.write( (_src2.read( i*(_src2.cols>>(XF_BITSHIFT(NPC))) +j) ) );
			_in3.write( (_src3.read( i*(_src3.cols>>(XF_BITSHIFT(NPC))) +j) ) );
		}
	}

	xfChannelCombine<ROWS, COLS, SRC_T, DST_T, NPC>(_in1, _in2, _in3, _out, _src1.rows, _src1.cols);

	Write_Mat_Loop:
	for(int i=0; i<_dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			XF_TNAME(DST_T,NPC) outpix = _out.read();
			_dst.write( i*(_dst.cols>>(XF_BITSHIFT(NPC))) +j, outpix);
		}
	}
}

#pragma SDS data access_pattern("_src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src2.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src3.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src4.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src1.data"[0:"_src1.size"])
#pragma SDS data copy("_src2.data"[0:"_src2.size"])
#pragma SDS data copy("_src3.data"[0:"_src3.size"])
#pragma SDS data copy("_src4.data"[0:"_src4.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void merge(xf::Mat<SRC_T, ROWS, COLS, NPC> &_src1, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src2, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src3, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src4, xf::Mat<DST_T, ROWS, COLS, NPC> &_dst){

	assert(((_src1.rows <= ROWS ) && (_src1.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_src2.rows <= ROWS ) && (_src2.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_src3.rows <= ROWS ) && (_src3.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_src4.rows <= ROWS ) && (_src4.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert(((_dst.rows <= ROWS ) && (_dst.cols <= COLS)) && "ROWS and COLS should be greater than input image");
	assert((SRC_T == XF_8UC1) && (DST_T == XF_8UC4) && "Source image should be of 1 channel and destination image of 4 channels");
//	assert(((NPC == XF_NPPC1)) && "NPC must be XF_NPPC1");

		hls::stream< XF_TNAME(SRC_T, NPC) > _in1;
		hls::stream< XF_TNAME(SRC_T, NPC) > _in2;
		hls::stream< XF_TNAME(SRC_T, NPC) > _in3;
		hls::stream< XF_TNAME(SRC_T, NPC) > _in4;
		hls::stream< XF_TNAME(DST_T, NPC) > _out;

#pragma HLS inline off
#pragma HLS DATAFLOW

	Read_Mat_Loop:
	for(int i=0; i<_src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			_in1.write( (_src1.read( i*(_src1.cols>>(XF_BITSHIFT(NPC))) +j) ));
			_in2.write( (_src2.read( i*(_src2.cols>>(XF_BITSHIFT(NPC))) +j) ) );
			_in3.write( (_src3.read( i*(_src3.cols>>(XF_BITSHIFT(NPC))) +j) ) );
			_in4.write( (_src4.read( i*(_src4.cols>>(XF_BITSHIFT(NPC))) +j) ) );
		}
	}

	xfChannelCombine<ROWS, COLS, SRC_T, DST_T, NPC>(_in1, _in2, _in3, _in4, _out, _src1.rows, _src1.cols);

	Write_Mat_Loop:
	for(int i=0; i<_dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			XF_TNAME(DST_T,NPC) outpix = _out.read();
			_dst.write( i*(_dst.cols>>(XF_BITSHIFT(NPC))) +j, outpix);
		}
	}
}

}



#endif//_XF_CHANNEL_COMBINE_HPP_

