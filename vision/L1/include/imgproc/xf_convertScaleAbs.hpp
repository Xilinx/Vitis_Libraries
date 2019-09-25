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
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _XF_CONVERT_SCALE_ABS_HPP_
#define _XF_CONVERT_SCALE_ABS_HPP_

#include "common/xf_common.h"
#include "hls_stream.h"

#ifndef XF_IN_STEP
#define XF_IN_STEP 8
#endif
#ifndef XF_OUT_STEP
#define XF_OUT_STEP 8
#endif
namespace xf {
namespace cv {
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
int convertScaleAbsKernel(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src1,
                          xf::cv::Mat<DST_T, ROWS, COLS, NPC>& dst,
                          float scale,
                          float shift,
                          uint16_t height,
                          uint16_t width) {
    ap_uint<13> i, j, k, l;
    ap_uint<64> temp = (scale * (1 << 23));
    ap_uint<64> temp2 = (shift * (1 << 23));

    int STEP = XF_PIXELDEPTH(DEPTH_SRC) / PLANES;

    XF_SNAME(WORDWIDTH_DST) pxl_pack_out;
    XF_SNAME(WORDWIDTH_SRC) pxl_pack1, pxl_pack2;
RowLoop:
    for (i = 0; i < height; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min = ROWS max = ROWS
#pragma HLS LOOP_FLATTEN OFF
        // clang-format on
    ColLoop:
        for (j = 0; j < width; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min = TC max = TC
#pragma HLS pipeline
            // clang-format on

            pxl_pack1 = (XF_SNAME(WORDWIDTH_SRC))(src1.read(i * width + j));
        ProcLoop:
            for (k = 0, l = 0; k < ((8 << XF_BITSHIFT(NPC)) * PLANES); k += XF_IN_STEP, l += XF_OUT_STEP) {
                XF_PTNAME(DEPTH_SRC) pxl1 = pxl_pack1.range(k + 7, k);

                ap_uint<64> firstcmp = pxl1 * temp;

                ap_uint<64> t = (firstcmp + temp2) >> 23;

                if (t > 255) {
                    t = 255;
                }

                pxl_pack_out.range(l + XF_OUT_STEP - 1, l) = (XF_PTNAME(DEPTH_DST))t;
            }

            dst.write(i * width + j, (XF_SNAME(WORDWIDTH_DST))pxl_pack_out);
        }
    }
    return 0;
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1>
void convertScaleAbs(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src1,
                     xf::cv::Mat<DST_T, ROWS, COLS, NPC>& dst,
                     float scale,
                     float shift) {
#ifndef __SYNTHESIS__
    assert(((SRC_T == XF_8UC1)) && "Input TYPE must be XF_8UC1 for 1-channel ");

    assert(((src1.rows == dst.rows) && (src1.cols == dst.cols)) && "Input and output image should be of same size");
    assert(((src1.rows <= ROWS) && (src1.cols <= COLS)) && "ROWS and COLS should be greater than input image");
    assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) && "NPC must be XF_NPPC1, XF_NPPC8 ");
#endif
    short width = src1.cols >> XF_BITSHIFT(NPC);

    convertScaleAbsKernel<SRC_T, DST_T, ROWS, COLS, NPC, XF_CHANNELS(SRC_T, NPC), XF_DEPTH(SRC_T, NPC),
                          XF_DEPTH(DST_T, NPC), XF_WORDWIDTH(SRC_T, NPC), XF_WORDWIDTH(DST_T, NPC),
                          (COLS >> XF_BITSHIFT(NPC))>(src1, dst, scale, shift, src1.rows, width);
}
} // namespace cv
} // namespace xf
#endif //_XF_CONVERT_SCALE_ABS_HPP_
