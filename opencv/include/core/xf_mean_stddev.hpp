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

#ifndef _XF_MEAN_STDDEV_HPP_
#define _XF_MEAN_STDDEV_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#define POW32 2147483648

#include "hls_stream.h"
#include "common/xf_common.h"
#include "core/xf_math.h"

namespace xf{

template<int TYPE, int ROWS, int COLS,int PLANES, int NPC>
void xFStddevkernel(xf::Mat<TYPE, ROWS, COLS, NPC>&  _src_mat1, unsigned short _mean[XF_CHANNELS(TYPE, NPC)], unsigned short  _dst_stddev[XF_CHANNELS(TYPE, NPC)],uint16_t height,uint16_t width)
{
#pragma HLS inline
	ap_uint<4> j;
	ap_uint<45> tmp_var_vals[(1<<XF_BITSHIFT(NPC))*PLANES];//={0};
	ap_uint<64> var[PLANES];//={0};
	uint32_t tmp_sum_vals[(1<<XF_BITSHIFT(NPC))*PLANES];//={0};
	uint64_t sum[PLANES];//={0};
	//ap_uint<8> val[(1<<XF_BITSHIFT(NPC))*PLANES];

#pragma HLS ARRAY_PARTITION variable=tmp_var_vals complete dim=0
#pragma HLS ARRAY_PARTITION variable=tmp_sum_vals complete dim=0
#pragma HLS ARRAY_PARTITION variable=sum complete dim=0
#pragma HLS ARRAY_PARTITION variable=var complete dim=0
	//#pragma HLS ARRAY_PARTITION variable=val complete dim=0


	for ( j = 0; j<((1<<XF_BITSHIFT(NPC))*PLANES);j++)
	{
#pragma HLS UNROLL
		tmp_var_vals[j] = 0;
		tmp_sum_vals[j] = 0;
	}
	for(j=0;j<PLANES;j++)
	{
		sum[j]=0;
		var[j]=0;
	}
	int p=0, read_index =0;
	ap_uint<13> row,col;
	Row_Loop1:
	for( row = 0; row < height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		Col_Loop1:
		for( col = 0; col < (width>>XF_BITSHIFT(NPC)); col++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPC max=COLS/NPC
#pragma HLS pipeline II=1
#pragma HLS LOOP_FLATTEN OFF

			XF_TNAME(TYPE,NPC) in_buf;
			in_buf = _src_mat1.read(read_index++);

			Extract1:
			for(int p=0; p < XF_NPIXPERCYCLE(NPC)*PLANES;p++)
			{
#pragma HLS unroll
#pragma HLS DEPENDENCE variable=tmp_var_vals intra false

				ap_uint<8> val = in_buf.range(p*8+7, p*8);
				tmp_sum_vals[p] = tmp_sum_vals[p] + val;
				unsigned short int  temp=((unsigned short)val * (unsigned short)val);

				tmp_var_vals[p] += temp;
			}
		}
	}

	for(int c=0;c<PLANES;c++) {
		for ( j = 0; j<(1<<XF_BITSHIFT(NPC));j++) {
#pragma HLS UNROLL
			sum[c] = (sum[c] + tmp_sum_vals[j*PLANES+c]);
			var[c] =(ap_uint<64>)((ap_uint<64>) var[c] + (ap_uint<64>)tmp_var_vals[j*PLANES+c]);
		}
	}

	ap_uint<16*PLANES> mean_acc = 0, stddev_acc = 0;

	for(int c=0;c<PLANES;c++)
	{
#pragma HLS UNROLL
		unsigned int tempmean=0;

		tempmean = (unsigned short)((ap_uint<64>)(256*(ap_uint<64>)sum[c]) / (width * height));
		mean_acc.range(c*16+15, c*16)  = tempmean;

		/* Variance Computation */

		uint32_t temp = (ap_uint<32>)((ap_uint<64>)(65536 * (ap_uint<64>)var[c])/(width * height));

		uint32_t Varstddev = temp - (tempmean*tempmean);

		uint32_t t1 = (uint32_t)((Varstddev >> 16) << 16);

		stddev_acc.range(c*16+15, c*16) = (unsigned short)xf::Sqrt(t1);//StdDev;//(StdDev >> 4);
	}

	for(int i =0;i<PLANES;++i){
		_mean[i] = mean_acc.range(i*16+15, i*16);
		_dst_stddev[i] = stddev_acc.range(i*16+15, i*16);
	}

}

#pragma SDS data data_mover("_src.data":FASTDMA)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern(_mean:SEQUENTIAL)
#pragma SDS data access_pattern(_stddev:SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy(_mean[0:"_src.channels()"])
#pragma SDS data copy(_stddev[0:"_src.channels()"])

template<int SRC_T,int ROWS, int COLS,int NPC=1>
void meanStdDev(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, unsigned short _mean[XF_CHANNELS(SRC_T, NPC)], unsigned short _stddev[XF_CHANNELS(SRC_T, NPC)])
{
#pragma HLS inline off
//#pragma HLS dataflow


	assert((SRC_T == XF_8UC1 || SRC_T == XF_8UC3 || SRC_T == XF_8UC4)  && "Input image type should be XF_8UC1, XF_8UC3 or XF_8UC4");

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8))
			&& " NPC must be XF_NPPC1, XF_NPPC8");

	assert(((_src.rows <= ROWS ) && (_src.cols <= COLS)) && "ROWS and COLS should be greater than input image");

	xFStddevkernel<SRC_T, ROWS, COLS, XF_CHANNELS(SRC_T,NPC), NPC>(_src, _mean, _stddev, _src.rows, _src.cols);
}
}

#endif // _XF_MEAN_STDDEV_HPP_
