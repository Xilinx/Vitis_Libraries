/***************************************************************************
 Copyright (c) 2020, Xilinx, Inc.
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

#ifndef __XF_EXTRACT_EFRAMES_ACCEL_CPP__
#define __XF_EXTRACT_EFRAMES_ACCEL_CPP__

#include "xf_extract_eframes_config.h"

static constexpr int __XF_DEPTH =
    (XF_MAX_COLS * XF_MAX_ROWS * (XF_PIXELWIDTH(XF_SRC_T, XF_NPPC)) / 8) / (IMAGE_PTR_WIDTH / 8);
static constexpr int __XF_DEPTH_2 = __XF_DEPTH * 2;

/************************************************************************************
 * Function:    AXIVideo2BayerMat
 * Parameters:  Multiple bayerWindow.getval AXI Stream, User Stream, Image Resolution
 * Return:      None
 * Description: Read data from multiple pixel/clk AXI stream into user defined stream
 ************************************************************************************/
template <int TYPE, int ROWS, int COLS, int NPPC>
void AXIVideo2BayerMat(InVideoStrm_t_e_s& bayer_strm, xf::cv::Mat<TYPE, ROWS, COLS, NPPC>& bayer_mat) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    InVideoStrmBus_t_e_s axi;

    const int m_pix_width = XF_PIXELWIDTH(TYPE, NPPC) * XF_NPIXPERCYCLE(NPPC);

    int rows = bayer_mat.rows;
    int cols = bayer_mat.cols >> XF_BITSHIFT(NPPC);
    int idx = 0;

    bool start = false;
    bool last = false;

loop_start_hunt:
    while (!start) {
// clang-format off
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
        // clang-format on

        bayer_strm >> axi;
        start = axi.user.to_bool();
    }

loop_row_axi2mat:
    for (int i = 0; i < rows; i++) {
        last = false;
// clang-format off
#pragma HLS loop_tripcount avg=ROWS max=ROWS
    // clang-format on
    loop_col_zxi2mat:
        for (int j = 0; j < cols; j++) {
// clang-format off
#pragma HLS loop_flatten off
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=COLS/NPPC max=COLS/NPPC
            // clang-format on

            if (start || last) {
                start = false;
            } else {
                bayer_strm >> axi;
            }

            last = axi.last.to_bool();

            bayer_mat.write(idx++, axi.data(m_pix_width - 1, 0));
        }

    loop_last_hunt:
        while (!last) {
// clang-format off
#pragma HLS pipeline II=1
#pragma HLS loop_tripcount avg=0 max=0
            // clang-format on

            bayer_strm >> axi;
            last = axi.last.to_bool();
        }
    }

    return;
}

template <int TYPE, int ROWS, int COLS, int NPPC>
void GRAYMat2AXIvideo(xf::cv::Mat<TYPE, ROWS, COLS, NPPC>& gray_mat, OutVideoStrm_t_e_s& gray_strm) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    OutVideoStrmBus_t_e_s axi;

    int rows = gray_mat.rows;
    int cols = gray_mat.cols >> XF_BITSHIFT(NPPC);
    int idx = 0;

    XF_TNAME(TYPE, NPPC) srcpixel;

    const int m_pix_width = XF_PIXELWIDTH(TYPE, NPPC) * XF_NPIXPERCYCLE(NPPC);

    int depth = XF_DTPIXELDEPTH(TYPE, XF_NPPC);

    bool sof = true; // Indicates start of frame

loop_row_mat2axi:
    for (int i = 0; i < rows; i++) {
// clang-format off
#pragma HLS loop_tripcount avg=ROWS max=ROWS
    // clang-format on
    loop_col_mat2axi:
        for (int j = 0; j < cols; j++) {
// clang-format off
#pragma HLS loop_flatten off
#pragma HLS pipeline II = 1
#pragma HLS loop_tripcount avg=COLS/NPPC max=COLS/NPPC
            // clang-format on
            if (sof) {
                axi.user = 1;
            } else {
                axi.user = 0;
            }

            if (j == cols - 1) {
                axi.last = 1;
            } else {
                axi.last = 0;
            }

            axi.data = 0;

            srcpixel = gray_mat.read(idx++);

            for (int npc = 0; npc < NPPC; npc++) {
                for (int rs = 0; rs < 1; rs++) {
                    int start = (rs + npc) * depth;

                    axi.data(start + (depth - 1), start) = srcpixel.range(start + (depth - 1), start);
                }
            }

            axi.keep = -1;
            gray_strm << axi;

            sof = false;
        }
    }

    return;
}
// =======================================================================
// H/W Acclerated functions that are visible to host
// =======================================================================

void extractEFrames_accel(InVideoStrm_t_e_s& in_ptr,
                          OutVideoStrm_t_e_s& lef_ptr,
                          OutVideoStrm_t_e_s& sef_ptr,
                          ap_uint<IMAGE_SIZE_WIDTH> height,
                          ap_uint<IMAGE_SIZE_WIDTH> width) {
// -----------------------------------------------
// HLS directives for ports
// -----------------------------------------------
// clang-format off
    #pragma HLS INTERFACE axis port=&in_ptr register
	#pragma HLS INTERFACE axis port=&lef_ptr register
	#pragma HLS INTERFACE axis port=&sef_ptr register
	
    #pragma HLS INTERFACE s_axilite port=height
    #pragma HLS INTERFACE s_axilite port=width
    #pragma HLS INTERFACE s_axilite port=return
    // clang-format on

    // -----------------------------------------------
    // Internal xf::Mat objects
    // -----------------------------------------------
    xf::cv::Mat<XF_SRC_T, XF_MAX_ROWS * 2, XF_MAX_COLS, XF_NPPC> InImg(height * 2, width);
    xf::cv::Mat<XF_SRC_T, XF_MAX_ROWS, XF_MAX_COLS, XF_NPPC> LEF_Img(height, width);
    xf::cv::Mat<XF_SRC_T, XF_MAX_ROWS, XF_MAX_COLS, XF_NPPC> SEF_Img(height, width);

// -----------------------------------------------
// Actual Body
// -----------------------------------------------
// clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    AXIVideo2BayerMat<XF_SRC_T, XF_MAX_ROWS * 2, XF_MAX_COLS, XF_NPPC>(in_ptr, InImg);
    // Actual accelerator
    xf::cv::extractExposureFrames<XF_SRC_T, NUM_V_BLANK_LINES, XF_MAX_ROWS, XF_MAX_COLS, XF_NPPC, XF_USE_URAM>(
        InImg, LEF_Img, SEF_Img);

    GRAYMat2AXIvideo<XF_SRC_T, XF_MAX_ROWS, XF_MAX_COLS, XF_NPPC>(LEF_Img, lef_ptr);
    GRAYMat2AXIvideo<XF_SRC_T, XF_MAX_ROWS, XF_MAX_COLS, XF_NPPC>(SEF_Img, sef_ptr);
}

#endif // __XF_EXTRACT_EFRAMES_ACCEL_CPP__
