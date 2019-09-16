/***************************************************************************
Copyright (c) 2016, Xilinx, Inc.
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

#ifndef _XF_AWB_HPP_
#define _XF_AWB_HPP_

#include "hls_stream.h"
#include "common/xf_common.h"
#include "imgproc/xf_duplicateimage.hpp"

#ifndef XF_IN_STEP
#define XF_IN_STEP 8
#endif
#ifndef XF_OUT_STEP
#define XF_OUT_STEP 8
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

namespace xf {
namespace cv {

/*
 * Simple auto white balancing algorithm requires compute histogram and normalize function based on maximum and minimum
 * of the histogram.
 */

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int PLANES,
          int DEPTH_SRC,
          int DEPTH_DST,
          int WORDWIDTH_SRC,
          int WORDWIDTH_DST,
          int TC>
int balanceWhiteKernel_simple(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src1,
                              xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src2,
                              xf::cv::Mat<DST_T, ROWS, COLS, NPC>& dst,
                              float thresh,
                              uint16_t height,
                              uint16_t width) {
#if 0
	float inputMin = 0.0f;
	float inputMax = 255.0f;
	float outputMin = 0.0f;
	float outputMax = 255.0f;
	float p = 2.0f;

	XF_PTNAME(XF_DEPTH(SRC_T,NPC)) in_pix, out_pix;
	ap_uint<8> r, g, b,b1=0,g1=0,r1=0;

	//******************** Simple white balance ********************


	int depth = 2; // depth of histogram tree

	int bins = 16; // number of bins at each histogram level

	int nElements = 256; //int(pow((float)bins, (float)depth));


	int hist[3][nElements];
	int hist1[3][nElements];


	int val[3];

	// histogram initialization

	INITIALIZE_HIST:for(int k=0;k<256;k++)
	{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 256 max = 256
		INITIALIZE:for(int hi =0 ;hi<3;hi++)
		{
#pragma HLS UNROLL
			hist[hi][k] = 0;
			hist1[hi][k] = 0;
		}
	}

	for (int row = 0; row != (height); row++)// histogram filling
	{
#pragma HLS LOOP_TRIPCOUNT min = 1 max = ROWS
		for (int col = 0; col != (width); col++)// histogram filling
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 1 max = COLS
#pragma HLS DEPENDENCE variable = hist array intra false
#pragma HLS DEPENDENCE variable = hist array inter false

			in_pix = src1.read(row*(width)+col);

			for (int j = 0; j < 3; j++)
			{
#pragma HLS UNROLL
				int pos = 0;
				int currentBin=0;
				float min_vals= inputMin - 0.5f;
				float max_vals= inputMax + 0.5f;


				float minValue=min_vals;
				float maxValue=max_vals;
				float interval = float(maxValue - minValue) / bins;

				val[j] = (in_pix.range((j*8)+7, j*8));

				for (int k = 0; k < 2; ++k)
				{

#pragma HLS UNROLL

					currentBin = int((val[j] - minValue + 1e-4f) / interval);
					++hist[j][pos + currentBin];

					pos= (pos + currentBin) * bins;

					minValue = minValue + currentBin * interval;
					maxValue = minValue + interval;

					interval /= bins;
				}
			}
		}
	}

	int total = width*height;
	float min_vals= inputMin - 0.5f;
	float max_vals= inputMax + 0.5f;
	float minValue[3]={min_vals,min_vals,min_vals};
	float maxValue[3]={max_vals,max_vals,max_vals};

	for (int j = 0; j < 3; ++j)
		// searching for s1 and s2
	{
		int p1 = 0;
		int p2 = bins - 1;
		int n1 = 0;
		int n2 = total;

		const float s1 = 2.0f;
		const float s2 = 2.0f;

		float interval = (max_vals - min_vals) / float(bins);

		for (int k = 0; k < 2; ++k)
			// searching for s1 and s2
		{

			int value = hist[j][p1];
			int value1 = hist[j][p2];

			while (n1 + hist[j][p1] < s1 * total / 100.0f)
			{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false

				n1 += hist[j][p1++];
				minValue[j] += interval;

			}
			p1 *= bins;

			while (n2 - hist[j][p2] > (100.0f - s2) * total / 100.0f)
			{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false

				n2 -= hist[j][p2--];
				maxValue[j] -= interval;
			}
			p2 = (p2 + 1) * bins - 1;

			interval /= bins;
		}
	}




	//**********************Normalization using minvalue and maxvalue for each channel

	float maxmin_diff[3];

	float newmax = 255.0f;
	float newmin = 0.0f;
	maxmin_diff[0]= maxValue[0]-minValue[0];
	maxmin_diff[1]= maxValue[1]-minValue[1];
	maxmin_diff[2]= maxValue[2]-minValue[2];
	unsigned char newdiff = newmax-newmin;


	XF_PTNAME(XF_DEPTH(SRC_T,NPC)) in_buf, out_buf;

	float inv_val[3];
	inv_val[0]= (float)(1/maxmin_diff[0]);
	inv_val[1]= (float)(1/maxmin_diff[1]);
	inv_val[2]= (float)(1/maxmin_diff[2]);

	int pval=0, read_index =0,write_index=0;
	ap_uint<13> row,col;

	Row_Loop1:
	for( row = 0; row < height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min = ROWS max = ROWS

		Col_Loop1:
		for( col = 0; col < (width>>XF_BITSHIFT(NPC)); col++)
		{
#pragma HLS LOOP_TRIPCOUNT min = COLS / NPC max = COLS / NPC
#pragma HLS pipeline II = 1
#pragma HLS LOOP_FLATTEN OFF

			in_buf = src2.read(read_index++);

			float value[3]={0,0,0};
			float divval[3]={0,0,0};
			float finalmul[3]={0,0,0};
			ap_int<32> dstval[3]={0,0,0};


			for (int j = 0; j < 3; j++){


				unsigned char srcval = (in_buf.range((j*8)+7, j*8));
				/*value[j] = srcval-minValue[j];
				divval[j] = value[j]/maxmin_diff[j];
				finalmul[j] = divval[j]*newdiff;
				dstval[j] = (int)(finalmul[j] + newmin);

				if(dstval[j] > 255)
				{
					out_buf.range((j*8)+7, j*8) = 255;//(unsigned char)dstval[j];
				}else
				{
					out_buf.range((j*8)+7, j*8) = (unsigned char)dstval[j];
				}*/
				dstval[j] = (newmax - newmin) * (srcval - minValue[j]) / (maxValue[j] - minValue[j]) + newmin;

				if(dstval[j].range(31,31)==1)
				{
					dstval[j] = 0;
				}

				if(dstval[j] > 255)
				{
					out_buf.range((j*8)+7, j*8) = 255;//(unsigned char)dstval[j];
				}else
				{
					out_buf.range((j*8)+7, j*8) = (unsigned char)dstval[j];
				}
			}

			dst.write(write_index++,out_buf);


		}
	}
#else

    float inputMin = 0.0f;
    float inputMax = 255.0f;
    float outputMin = 0.0f;
    float outputMax = 255.0f;
    float p = 2.0f;

    XF_TNAME(SRC_T, NPC) in_pix, in_pix1, out_pix;
    ap_uint<8> r, g, b, b1 = 0, g1 = 0, r1 = 0;

    //******************** Simple white balance ********************

    int depth = 2; // depth of histogram tree

    int bins = 16; // number of bins at each histogram level

    int nElements = 256; // int(pow((float)bins, (float)depth));

    int hist[3][nElements];

#pragma HLS ARRAY_PARTITION variable = hist complete dim = 1

    int val[3];

    // histogram initialization

INITIALIZE_HIST:
    for (int k = 0; k < 256; k++) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 256 max = 256
    INITIALIZE:
        for (int hi = 0; hi < 3; hi++) {
#pragma HLS UNROLL
            hist[hi][k] = 0;
        }
    }

    // Temporary array used while computing histogram
    uint32_t tmp_hist[XF_NPIXPERCYCLE(NPC) * PLANES][256] = {0};
    uint32_t tmp_hist1[XF_NPIXPERCYCLE(NPC) * PLANES][256] = {0};
#pragma HLS ARRAY_PARTITION variable = tmp_hist complete dim = 1
#pragma HLS ARRAY_PARTITION variable = tmp_hist1 complete dim = 1
    XF_TNAME(SRC_T, NPC) in_buf, in_buf1, temp_buf;

    bool flag = 0;

HIST_INITIALIZE_LOOP:
    for (ap_uint<10> i = 0; i < 256; i++) //
    {
#pragma HLS PIPELINE
        for (ap_uint<5> j = 0; j < XF_NPIXPERCYCLE(NPC) * PLANES; j++) {
#pragma HLS LOOP_TRIPCOUNT min = 256 max = 256
            tmp_hist[j][i] = 0;
            tmp_hist1[j][i] = 0;
        }
    }

ROW_LOOP:
    for (int row = 0; row != (height); row++) // histogram filling
    {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = ROWS
    COL_LOOP:
        for (int col = 0; col < (width); col = col + 2) // histogram filling
        {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 1 max = TC / 2
#pragma HLS DEPENDENCE variable = hist array intra false
#pragma HLS DEPENDENCE variable = hist array inter false

            in_pix = src1.read(row * (width) + col);
            in_pix1 = src1.read((row * (width) + col) + 1);

        PLANES_LOOP:
            for (ap_uint<9> j = 0; j < XF_NPIXPERCYCLE(NPC) * PLANES; j++) {
#pragma HLS DEPENDENCE variable = tmp_hist array intra false
#pragma HLS DEPENDENCE variable = tmp_hist1 array intra false
#pragma HLS UNROLL

                ap_uint<8> val = 0, val1 = 0;
                val = in_pix.range(j * 8 + 7, j * 8);
                val1 = in_pix1.range(j * 8 + 7, j * 8);

                int pos = 0, pos1 = 0;
                int currentBin = 0, currentBin1 = 0;
                ap_fixed<24, 12> min_vals = inputMin - 0.5f;
                ap_fixed<24, 12> max_vals = inputMax + 0.5f;

                ap_fixed<24, 12> minValue = min_vals, minValue1 = min_vals;
                ap_fixed<24, 12> maxValue = max_vals, maxValue1 = max_vals;

                ap_fixed<24, 12> interval = ap_fixed<24, 12>(maxValue - minValue) / bins;

                for (int k = 0; k < 2; ++k) {

#pragma HLS UNROLL

                    currentBin = int((val - minValue + (ap_fixed<24, 12>)(1e-4f)) / interval);
                    currentBin1 = int((val1 - minValue1 + (ap_fixed<24, 12>)(1e-4f)) / interval);

                    ++tmp_hist[j][pos + currentBin];
                    ++tmp_hist1[j][pos1 + currentBin1];

                    pos = (pos + currentBin) << 4;
                    pos1 = (pos1 + currentBin1) << 4;

                    minValue = minValue + currentBin * interval;
                    minValue1 = minValue1 + currentBin1 * interval;
                    maxValue = minValue + interval;
                    maxValue1 = minValue1 + interval;

                    interval /= bins;
                }
            }
        }
    }

    uint32_t cnt, p1;
    uint32_t plane[PLANES];
COPY_LOOP:
    for (ap_uint<10> i = 0; i < 256; i++) {
#pragma HLS pipeline
        cnt = 0;
        p1 = 0;
        for (ap_uint<5> j = 0, k = 0; j < XF_NPIXPERCYCLE(NPC) * PLANES; j++, k++) {
#pragma HLS UNROLL

            uint32_t value = tmp_hist[j][i] + tmp_hist1[j][i];
            cnt = cnt + value;
            if (PLANES != 1) {
                plane[p1] = cnt;
                p1++;
                cnt = 0;
                value = 0;
            }
        }
        if (PLANES == 1) {
            hist[0][i] = cnt;
        } else {
            hist[0][i] = plane[0];
            hist[1][i] = plane[1];
            hist[2][i] = plane[2];
        }
    }

    int total = width * height;
    ap_fixed<24, 12> min_vals = inputMin - 0.5f;
    ap_fixed<24, 12> max_vals = inputMax + 0.5f;
    ap_fixed<24, 12> minValue[3] = {min_vals, min_vals, min_vals};
    ap_fixed<24, 12> maxValue[3] = {max_vals, max_vals, max_vals};

    for (int j = 0; j < 3; ++j)
    // searching for s1 and s2
    {
        int p1 = 0;
        int p2 = bins - 1;
        int n1 = 0;
        int n2 = total;

        ap_fixed<24, 12> s1 = 2.0f;
        ap_fixed<24, 12> s2 = 2.0f;

        ap_fixed<24, 12> interval = (max_vals - min_vals) / bins;

        for (int k = 0; k < 2; ++k)
        // searching for s1 and s2
        {
            int value = hist[j][p1];
            int value1 = hist[j][p2];

            int rval = (s1 * total) / 100;

            int rval1 = (100 - s2) * total / 100;

            while (n1 + hist[j][p1] < rval) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false

                n1 += hist[j][p1++];
                minValue[j] += interval;
            }
            p1 *= bins;

            while (n2 - hist[j][p2] > rval1) {
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min = 255 max = 255
#pragma HLS DEPENDENCE variable = hist array intra false

                n2 -= hist[j][p2--];
                maxValue[j] -= interval;
            }
            p2 = (p2 + 1) * bins - 1;

            interval /= bins;
        }
    }

    //**********************Normalization using minvalue and maxvalue for each channel
#if 0
	float maxmin_diff[3];

	float newmax = 255.0f;
	float newmin = 0.0f;
	maxmin_diff[0]= maxValue[0]-minValue[0];
	maxmin_diff[1]= maxValue[1]-minValue[1];
	maxmin_diff[2]= maxValue[2]-minValue[2];
	unsigned char newdiff = newmax-newmin;


	XF_TNAME(SRC_T,NPC) in_buf_n,out_buf_n;

	float inv_val[3];
	inv_val[0]= (float)(1/maxmin_diff[0]);
	inv_val[1]= (float)(1/maxmin_diff[1]);
	inv_val[2]= (float)(1/maxmin_diff[2]);

	int pval=0, read_index =0,write_index=0;
	ap_uint<13> row,col;

	Row_Loop1:
	for( row = 0; row < height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min = ROWS max = ROWS

		Col_Loop1:
		for( col = 0; col < width; col++)
		{
#pragma HLS LOOP_TRIPCOUNT min = COLS / NPC max = COLS / NPC
#pragma HLS pipeline II = 1
#pragma HLS LOOP_FLATTEN OFF

			in_buf_n = src2.read(read_index++);

			float value[3]={0,0,0};
			float divval[3]={0,0,0};
			float finalmul[3]={0,0,0};
			ap_int<32> dstval[3]={0,0,0};

			for(int p=0; p < XF_NPIXPERCYCLE(NPC)*PLANES;p++)
			{
#pragma HLS unroll

				ap_uint<8> val = in_buf_n.range(p*8+7, p*8);
				//dstval[p%3] = (newmax - newmin) * (val - minValue[p%3]) / (maxValue[p%3] - minValue[p%3]) + newmin;

				value[p%3] = val-minValue[p%3];
				divval[p%3] = value[p%3]/maxmin_diff[p%3];
				finalmul[p%3] = divval[p%3]*newdiff;
				dstval[p%3] = (int)(finalmul[p%3] + newmin);

				if(dstval[p%3].range(31,31)==1)
				{
					dstval[p%3] = 0;
				}

				if(dstval[p%3] > 255)
				{
					out_buf_n.range((p*8)+7, p*8) = 255;//(unsigned char)dstval[j];
				}else
				{
					out_buf_n.range((p*8)+7, p*8) = (unsigned char)dstval[p%3];
				}


			}

			dst.write(row*width+col,out_buf_n);

		}
	}

#else
    ap_fixed<24, 12> maxmin_diff[3];

    ap_fixed<24, 12> newmax = 255.0f;
    ap_fixed<24, 12> newmin = 0.0f;
    maxmin_diff[0] = maxValue[0] - minValue[0];
    maxmin_diff[1] = maxValue[1] - minValue[1];
    maxmin_diff[2] = maxValue[2] - minValue[2];
    ap_fixed<24, 12> newdiff = newmax - newmin;

    XF_TNAME(SRC_T, NPC) in_buf_n, out_buf_n;

    ap_fixed<24, 6> inv_val[3];
    inv_val[0] = ((ap_fixed<24, 12>)1 / maxmin_diff[0]);
    inv_val[1] = ((ap_fixed<24, 12>)1 / maxmin_diff[1]);
    inv_val[2] = ((ap_fixed<24, 12>)1 / maxmin_diff[2]);

    int pval = 0, read_index = 0, write_index = 0;
    ap_uint<13> row, col;

Row_Loop1:
    for (row = 0; row < height; row++) {
#pragma HLS LOOP_TRIPCOUNT min = ROWS max = ROWS

    Col_Loop1:
        for (col = 0; col < width; col++) {
#pragma HLS LOOP_TRIPCOUNT min = COLS / NPC max = COLS / NPC
#pragma HLS pipeline II = 1
#pragma HLS LOOP_FLATTEN OFF

            in_buf_n = src2.read(read_index++);

            ap_fixed<24, 12> value[3] = {0, 0, 0};
            ap_fixed<24, 12> divval[3] = {0, 0, 0};
            ap_fixed<24, 12> finalmul[3] = {0, 0, 0};
            ap_int<32> dstval[3] = {0, 0, 0};

            for (int p = 0, bit = 0; p < XF_NPIXPERCYCLE(NPC) * PLANES; p++, bit = p % 3) {
#pragma HLS unroll

                ap_uint<8> val = in_buf_n.range(p * 8 + 7, p * 8);
                // dstval[p%3] = (newmax - newmin) * (val - minValue[p%3]) / (maxValue[p%3] - minValue[p%3]) + newmin;

                value[bit] = val - minValue[bit];
                divval[bit] = value[bit] / maxmin_diff[p % 3];
                finalmul[bit] = divval[bit] * newdiff;
                dstval[bit] = (int)(finalmul[bit] + newmin);

                if (dstval[bit].range(31, 31) == 1) {
                    dstval[bit] = 0;
                }

                if (dstval[bit] > 255) {
                    out_buf_n.range((p * 8) + 7, p * 8) = 255; //(unsigned char)dstval[j];
                } else {
                    out_buf_n.range((p * 8) + 7, p * 8) = (unsigned char)dstval[bit];
                }
            }

            dst.write(row * width + col, out_buf_n);
        }
    }

#endif
#endif
}

template <int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC,
          int PLANES,
          int DEPTH_SRC,
          int DEPTH_DST,
          int WORDWIDTH_SRC,
          int WORDWIDTH_DST,
          int TC>
int balanceWhiteKernel(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src1,
                       xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src2,
                       xf::cv::Mat<DST_T, ROWS, COLS, NPC>& dst,
                       float thresh,
                       uint16_t height,
                       uint16_t width) {
#if 0
	ap_uint<13> i,j,k,l;

	XF_PTNAME(XF_DEPTH(SRC_T,NPC)) in_pix, out_pix;
	ap_uint<8> r, g, b,b1=0,g1=0,r1=0;

	XF_SNAME(WORDWIDTH_DST) pxl_pack_out;
	XF_SNAME(WORDWIDTH_SRC)  pxl_pack1, pxl_pack2;

	int thresh255 = int(thresh*255);

	int minRGB,maxRGB;

	int sumB=0,sumG=0,sumR=0;

	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min = ROWS max = ROWS
#pragma HLS LOOP_FLATTEN OFF
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min = TC max = TC
#pragma HLS pipeline
			in_pix = src1.read(i*src1.cols+j);
			b = in_pix.range(7, 0);
			g = in_pix.range(15, 8);
			r = in_pix.range(23, 16);

			minRGB = MIN(b, MIN(g, r));
			maxRGB = MAX(b, MAX(g, r));


			if ((maxRGB - minRGB) * 255 > thresh255 * maxRGB)
				continue;


			sumB += b;
			sumG += g;
			sumR += r;
		}
	}

	int max_sum = MAX(sumB,MAX(sumG,sumR));

	float dinB = sumB < 0.1 ? 0.0f: (float)(max_sum/(float)sumB);
	float dinG = sumG < 0.1 ? 0.0f: (float)(max_sum/(float)sumG);
	float dinR = sumR < 0.1 ? 0.0f: (float)(max_sum/(float)sumR);
	float gain_max = MAX(dinB, MAX(dinG, dinR));


	if (gain_max > 0)
	{
		dinB /= gain_max;
		dinG /= gain_max;
		dinR /= gain_max;
	}


	int i_gainB = (dinB * (1 << 8)), i_gainG = (dinG * (1 << 8)),i_gainR = (dinR * (1 << 8));

	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min = ROWS max = ROWS
#pragma HLS LOOP_FLATTEN OFF
		ColLoop1:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min = TC max = TC
#pragma HLS pipeline
			in_pix = src2.read(i*src2.cols+j);
			b = in_pix.range(7, 0);
			g = in_pix.range(15, 8);
			r = in_pix.range(23, 16);

			unsigned char b1 = (unsigned char)((b*i_gainB)>>8);
			unsigned char g1 = (unsigned char)((g*i_gainG)>>8);
			unsigned char r1 = (unsigned char)((r*i_gainR)>>8);

			out_pix.range(7, 0) = (unsigned char)b1;
			out_pix.range(15, 8) = (unsigned char)g1;
			out_pix.range(23, 16) =  (unsigned char)r1;

			dst.write(i*src2.cols+j,out_pix);
		}
	}
#else
    ap_uint<13> i = 0, j = 0;

    XF_TNAME(SRC_T, NPC) in_pix, out_pix;
    ap_uint<8> r, g, b, b1 = 0, g1 = 0, r1 = 0;

    XF_SNAME(WORDWIDTH_DST) pxl_pack_out;
    XF_SNAME(WORDWIDTH_SRC) pxl_pack1, pxl_pack2;

    int thresh255 = int(thresh * 255);

    int minRGB, maxRGB;

    ap_ufixed<32, 32> tmpsum_vals[(1 << XF_BITSHIFT(NPC)) * PLANES];

    ap_ufixed<32, 32> sum[PLANES];

#pragma HLS ARRAY_PARTITION variable = tmpsum_vals complete dim = 0
#pragma HLS ARRAY_PARTITION variable = sum complete dim = 0

    for (j = 0; j < ((1 << XF_BITSHIFT(NPC)) * PLANES); j++) {
#pragma HLS UNROLL
        tmpsum_vals[j] = 0;
    }
    for (j = 0; j < PLANES; j++) {
#pragma HLS UNROLL
        sum[j] = 0;
    }

    int p = 0, read_index = 0;

Row_Loop:
    for (i = 0; i < height; i++) {
#pragma HLS LOOP_TRIPCOUNT min = ROWS max = ROWS

    Col_Loop:
        for (j = 0; j < (width); j++) {
#pragma HLS LOOP_TRIPCOUNT min = COLS / NPC max = COLS / NPC
#pragma HLS pipeline II = 1
#pragma HLS LOOP_FLATTEN OFF

            XF_TNAME(SRC_T, NPC) in_buf;
            in_buf = src1.read(i * width + j);

        PLANES_LOOP:
            for (int p = 0; p < XF_NPIXPERCYCLE(NPC) * PLANES; p = p + PLANES) {
#pragma HLS unroll

                ap_uint<8> val1 = in_buf.range(p * 8 + 7, p * 8);
                ap_uint<8> val2 = in_buf.range(p * 8 + 15, p * 8 + 8);
                ap_uint<8> val3 = in_buf.range(p * 8 + 23, p * 8 + 16);

                minRGB = MIN(val1, MIN(val2, val3));
                maxRGB = MAX(val1, MAX(val2, val3));

                if ((maxRGB - minRGB) * 255 > thresh255 * maxRGB) continue;

                tmpsum_vals[p] = tmpsum_vals[p] + val1;
                tmpsum_vals[(p) + 1] = tmpsum_vals[(p) + 1] + val2;
                tmpsum_vals[(p) + 2] = tmpsum_vals[(p) + 2] + val3;
            }
        }
    }

    for (int c = 0; c < PLANES; c++) {
        for (j = 0; j < (1 << XF_BITSHIFT(NPC)); j++) {
#pragma HLS UNROLL
            sum[c] = (sum[c] + tmpsum_vals[j * PLANES + c]);
        }
    }

    ap_ufixed<32, 32> max_sum_fixed = MAX(sum[0], MAX(sum[1], sum[2]));

    ap_ufixed<32, 2> bval = (float)0.1;
    ap_ufixed<32, 32> zero = 0;

    ap_ufixed<40, 32> dinB1;
    ap_ufixed<40, 32> dinG1;
    ap_ufixed<40, 32> dinR1;

    if (sum[0] < bval) {
        dinB1 = 0;
    } else {
        dinB1 = (ap_ufixed<40, 32>)((ap_ufixed<40, 32>)max_sum_fixed / sum[0]);
    }

    if (sum[1] < bval) {
        dinG1 = 0;
    } else {
        dinG1 = (ap_ufixed<40, 32>)((ap_ufixed<40, 32>)max_sum_fixed / sum[1]);
    }
    if (sum[2] < bval) {
        dinR1 = 0;
    } else {
        dinR1 = (ap_ufixed<40, 32>)((ap_ufixed<40, 32>)max_sum_fixed / sum[2]);
    }

    ap_ufixed<40, 32> gain_max1 = MAX(dinB1, MAX(dinG1, dinR1));

    if (gain_max1 > 0) {
        dinB1 /= gain_max1;
        dinG1 /= gain_max1;
        dinR1 /= gain_max1;
    }

    float a1 = dinB1;
    float a2 = dinG1;
    float a3 = dinR1;

    int i_gain[3] = {0, 0, 0};
    i_gain[0] = (dinB1 * (1 << 8));
    i_gain[1] = (dinG1 * (1 << 8));
    i_gain[2] = (dinR1 * (1 << 8));

    for (i = 0; i < height; i++) {
#pragma HLS LOOP_TRIPCOUNT min = ROWS max = ROWS
#pragma HLS LOOP_FLATTEN OFF
    ColLoop1:
        for (j = 0; j < width; j++) {
#pragma HLS LOOP_TRIPCOUNT min = TC max = TC
#pragma HLS pipeline
            in_pix = src2.read(i * width + j);

            for (int p = 0; p < XF_NPIXPERCYCLE(NPC) * PLANES; p++) {
#pragma HLS unroll

                ap_uint<8> val = in_pix.range(p * 8 + 7, p * 8);
                ap_uint<8> outval = (unsigned char)((val * i_gain[p % 3]) >> 8);

                out_pix.range(p * 8 + 7, p * 8) = outval;
            }

            dst.write(i * width + j, out_pix);
        }
    }
#endif
}

#pragma SDS data access_pattern("src1.data" : SEQUENTIAL)
#pragma SDS data copy("src1.data" [0:"src1.size"])
#pragma SDS data access_pattern("dst.data" : SEQUENTIAL)
#pragma SDS data copy("dst.data" [0:"dst.size"])

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, bool WB_TYPE>
void balanceWhite(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src1,
                  xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src2,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC>& dst,
                  float thresh) {
    assert(((src1.rows == dst.rows) && (src1.cols == dst.cols)) && "Input and output image should be of same size");
    assert(((src1.rows <= ROWS) && (src1.cols <= COLS)) && "ROWS and COLS should be greater than input image");
    assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) && "NPC must be XF_NPPC1, XF_NPPC8 ");

    short width = src1.cols >> XF_BITSHIFT(NPC);

    if (WB_TYPE == 0) {
        balanceWhiteKernel<SRC_T, DST_T, ROWS, COLS, NPC, XF_CHANNELS(SRC_T, NPC), XF_DEPTH(SRC_T, NPC),
                           XF_DEPTH(DST_T, NPC), XF_WORDWIDTH(SRC_T, NPC), XF_WORDWIDTH(DST_T, NPC),
                           (COLS >> XF_BITSHIFT(NPC))>(src1, src2, dst, thresh, src1.rows, width);
    } else {
        balanceWhiteKernel_simple<SRC_T, DST_T, ROWS, COLS, NPC, XF_CHANNELS(SRC_T, NPC), XF_DEPTH(SRC_T, NPC),
                                  XF_DEPTH(DST_T, NPC), XF_WORDWIDTH(SRC_T, NPC), XF_WORDWIDTH(DST_T, NPC),
                                  (COLS >> XF_BITSHIFT(NPC))>(src1, src2, dst, thresh, src1.rows, width);
    }
}
} // namespace cv
} // namespace xf
#endif //_XF_AWB_HPP_
