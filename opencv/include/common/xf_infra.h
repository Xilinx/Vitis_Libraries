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

#ifndef _XF_INFRA_H_
#define _XF_INFRA_H_

#ifndef __cplusplus
#error C++ is needed to use this file!
#endif

#include <stdio.h>
#include <assert.h>
#include "xf_types.h"
#include "hls_stream.h"
#include "xf_axi_sdata.h"
#include "common/xf_axi_io.h"

namespace xf
		{
template<int SRC_T, int ROWS, int COLS, int NPC>
void write(xf::Mat<SRC_T, ROWS, COLS, NPC> &img,xf::Scalar<XF_CHANNELS(SRC_T,NPC), XF_TNAME(SRC_T,NPC)> s,int ind) {
#pragma HLS inline

	img.write(ind,s.val[0]);


}


template<int SRC_T, int ROWS, int COLS, int NPC>
void fetchingmatdata(xf::Mat<SRC_T, ROWS, COLS, NPC> &img,xf::Scalar<XF_CHANNELS(SRC_T,NPC), XF_TNAME(SRC_T,NPC)> s,int val)
{
	#pragma HLS inline
	write(img,s,val);
}
template<int SRC_T, int ROWS, int COLS, int NPC>
xf::Scalar<XF_CHANNELS(SRC_T,NPC), XF_TNAME(SRC_T,NPC)>read(xf::Mat<SRC_T, ROWS, COLS, NPC> &img,int index)
{

	#pragma HLS inline

	xf::Scalar<XF_CHANNELS(SRC_T,NPC), XF_TNAME(SRC_T,NPC) >scl;
	scl.val[0]=img.read(index);

	return scl;


}


template<int SRC_T, int ROWS, int COLS, int NPC>
void fillingdata(xf::Mat<SRC_T, ROWS, COLS, NPC> &img,xf::Scalar<XF_CHANNELS(SRC_T,NPC), XF_TNAME(SRC_T,NPC)>& s, int index)
{
#pragma HLS inline

	s=read(img,index);
}


#define HLS_CN_MAX     512
#define HLS_CN_SHIFT   11
#define HLS_DEPTH_MAX  (1 << HLS_CN_SHIFT)


#define HLS_MAT_CN_MASK          ((HLS_CN_MAX - 1) << HLS_CN_SHIFT)
#define HLS_MAT_CN(flags)        ((((flags) & HLS_MAT_CN_MASK) >> HLS_CN_SHIFT) + 1)
#define HLS_MAT_TYPE_MASK        (HLS_DEPTH_MAX*HLS_CN_MAX - 1)
#define HLS_MAT_TYPE(flags)      ((flags) & HLS_MAT_TYPE_MASK)


#define ERROR_IO_EOL_EARLY  (1 << 0)
#define ERROR_IO_EOL_LATE   (1 << 1)
#define ERROR_IO_SOF_EARLY  (1 << 0)
#define ERROR_IO_SOF_LATE   (1 << 1)

// Unpack a AXI video stream into a xf::Mat<> object

template<int W,int T,int ROWS, int COLS,int NPC>
int AXIvideo2xfMat(hls::stream< ap_axiu<W,1,1,1> >& AXI_video_strm, xf::Mat<T,ROWS, COLS, NPC>& img)
{
#pragma hls inline

    int res = 0,val=0,depth;
    ap_axiu<W,1,1,1> axi;
    xf::Scalar<XF_CHANNELS(T,NPC), XF_TNAME(T,NPC)> pix;
     depth = XF_WORDDEPTH(XF_WORDWIDTH(T,NPC));
//    HLS_SIZE_T rows = img.rows;
//    HLS_SIZE_T cols = img.cols;
     int rows = img.rows;
     int cols = img.cols;
    assert(rows <= ROWS);
    assert(cols <= COLS);
    bool sof = 0;
 loop_wait_for_start: while (!sof) {
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
        AXI_video_strm >> axi;
        sof = axi.user.to_int();
    }
 loop_height: for (int i = 0; i < rows; i++) {
        bool eol = 0;
    loop_width: for (int j = 0; j < (cols/NPC); j++) {
#pragma HLS loop_flatten off
#pragma HLS pipeline II=1
            if (sof || eol) {
                sof = 0;
                eol = axi.last.to_int();
            } else {
                // If we didn't reach EOL, then read the next pixel
                AXI_video_strm >> axi;
                eol = axi.last.to_int();
                bool user = axi.user.to_int();
                if(user) {
                    res |= ERROR_IO_SOF_EARLY;
                }
            }
            if (eol && (j != cols-1)) {
                res |= ERROR_IO_EOL_EARLY;
            }
            // All channels are merged in cvMat2AXIVideoxf function
           xf:: AXIGetBitFields(axi, 0, depth, pix.val[0]);

        	fetchingmatdata< T, ROWS,  COLS, NPC>(img, pix,val);
            val++;
        }
    loop_wait_for_eol: while (!eol) {
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
            // Keep reading until we get to EOL
            AXI_video_strm >> axi;
            eol = axi.last.to_int();
            res |= ERROR_IO_EOL_LATE;
        }
    }
    return res;
}


//Pack the data of a xf::Mat<> object into an AXI Video stream

template<int W, int T, int ROWS, int COLS,int NPC>
int xfMat2AXIvideo(xf::Mat<T,ROWS, COLS,NPC>& img,hls::stream<ap_axiu<W,1,1,1> >& AXI_video_strm)
{
#pragma hls inline
    int res = 0,index=0,depth;
    xf::Scalar<XF_CHANNELS(T,NPC), XF_TNAME(T,NPC)> pix;
    ap_axiu<W,1,1,1> axi;
     depth =XF_WORDDEPTH(XF_WORDWIDTH(T,NPC));//8;// HLS_TBITDEPTH(T);

    // std::cout << W << " " << depth << " " << HLS_MAT_CN(T) << "\n";
    assert(W >= depth*HLS_MAT_CN(T) && "Bit-Width of AXI stream must be greater than the total number of bits in a pixel");
//    HLS_SIZE_T rows = img.rows;
//    HLS_SIZE_T cols = img.cols;
    int rows = img.rows;
    int cols = img.cols;
    assert(rows <= ROWS);
    assert(cols <= COLS);
    bool sof = 1;
 loop_height: for (int i = 0; i < rows; i++) {
    loop_width: for (int j = 0; j < (cols/NPC); j++) {
#pragma HLS loop_flatten off
#pragma HLS pipeline II=1
            if (sof) {
                axi.user = 1;
                sof = 0;

            } else {
                axi.user = 0;
            }
            if (j == (cols-1)) {
                axi.last = 1;
            } else {
                axi.last = 0;
            }
            fillingdata< T, ROWS,  COLS, NPC>(img, pix,index);
            index++;
            axi.data = -1;

                xf::AXISetBitFields(axi, 0, depth, pix.val[0]);
            axi.keep = -1;
            AXI_video_strm << axi;
        }
    }
    return res;
}
}

#endif
