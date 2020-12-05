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

#include "xf_isp_types.h"

static bool flag = 0;

static uint32_t hist0[3][HIST_SIZE] = {0};
static uint32_t hist1[3][HIST_SIZE] = {0};
static uint32_t histogram0[1][256] = {0};
static uint32_t histogram1[1][256] = {0};
static int igain_0[3] = {0};
static int igain_1[3] = {0};

/************************************************************************************
 * Function:    AXIVideo2BayerMat
 * Parameters:  Multiple bayerWindow.getval AXI Stream, User Stream, Image Resolution
 * Return:      None
 * Description: Read data from multiple pixel/clk AXI stream into user defined stream
 ************************************************************************************/
template <int TYPE, int ROWS, int COLS, int NPPC>
void AXIVideo2BayerMat(InVideoStrm_t& bayer_strm, xf::cv::Mat<TYPE, ROWS, COLS, NPPC>& bayer_mat) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    InVideoStrmBus_t axi;

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
void ColorMat2AXIvideo(xf::cv::Mat<TYPE, ROWS, COLS, NPPC>& color_mat, OutVideoStrm_t& color_strm) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    OutVideoStrmBus_t axi;

    int rows = color_mat.rows;
    int cols = color_mat.cols >> XF_BITSHIFT(NPPC);
    int idx = 0;

    XF_TNAME(TYPE, NPPC) srcpixel;

    const int m_pix_width = XF_PIXELWIDTH(TYPE, NPPC) * XF_NPIXPERCYCLE(NPPC);

    int depth = XF_DTPIXELDEPTH(XF_LTM_T, XF_NPPC);

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

            srcpixel = color_mat.read(idx++);

            for (int npc = 0; npc < NPPC; npc++) {
                for (int rs = 0; rs < 3; rs++) {
#if XF_AXI_GBR == 1
                    int kmap[3] = {1, 0, 2}; // GBR format
#else
                    int kmap[3] = {0, 1, 2}; // GBR format
#endif

                    int start = (rs + npc * 3) * depth;

                    int start_format = (kmap[rs] + npc * 3) * depth;

                    axi.data(start + (depth - 1), start) = srcpixel.range(start_format + (depth - 1), start_format);
                }
            }

            axi.keep = -1;
            color_strm << axi;

            sof = false;
        }
    }

    return;
}

void ISPpipeline(InVideoStrm_t& s_axis_video,
                 OutVideoStrm_t& m_axis_video,
                 unsigned short height,
                 unsigned short width,
                 uint32_t hist0[3][HIST_SIZE],
                 uint32_t hist1[3][HIST_SIZE],
                 uint32_t hist_aec1[1][256],
                 uint32_t hist_aec2[1][256],
                 int gain0[3],
                 int gain1[3]) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> imgInput1(height, width);
    xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> imgInput2(height, width);
    xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> bpc_out(height, width);
    xf::cv::Mat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> gain_out(height, width);
    xf::cv::Mat<XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> demosaic_out(height, width);
    xf::cv::Mat<XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> impop(height, width);
    xf::cv::Mat<XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, S_DEPTH> ltm_in(height, width);
    xf::cv::Mat<XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> lsc_out(height, width);
    xf::cv::Mat<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> _dst(height, width);
    xf::cv::Mat<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC> aecin(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    float inputMin = 0.0f;
    float inputMax = (1 << (XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC))) - 1; // 65535.0f;
    float outputMin = 0.0f;
    float outputMax = (1 << (XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC))) - 1; // 65535.0f;
    float p = 0.2f;
    float thresh = 2.0f;

    float mul_fact = (inputMax / (inputMax - BLACK_LEVEL));

    AXIVideo2BayerMat<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC>(s_axis_video, imgInput1);

    xf::cv::blackLevelCorrection<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 16, 15, 1>(imgInput1, imgInput2, BLACK_LEVEL,
                                                                                    mul_fact);

    xf::cv::badpixelcorrection<XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 0, 0>(imgInput2, bpc_out);
    xf::cv::gaincontrol<XF_BAYER_PATTERN, XF_SRC_T, XF_HEIGHT, XF_WIDTH, XF_NPPC>(bpc_out, gain_out);
    xf::cv::demosaicing<XF_BAYER_PATTERN, XF_SRC_T, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 0>(gain_out, demosaic_out);

    if (WB_TYPE) {
        xf::cv::AWBhistogram<XF_DST_T, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, WB_TYPE, HIST_SIZE>(
            demosaic_out, impop, hist0, thresh, inputMin, inputMax, outputMin, outputMax);
        xf::cv::AWBNormalization<XF_DST_T, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, WB_TYPE, HIST_SIZE, S_DEPTH>(
            impop, ltm_in, hist1, thresh, inputMin, inputMax, outputMin, outputMax);
    } else {
        xf::cv::AWBChannelGain<XF_DST_T, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 0>(demosaic_out, impop, p, gain0);
        xf::cv::AWBGainUpdate<XF_DST_T, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, 0, S_DEPTH>(impop, ltm_in, p, gain1);
    }

    xf::cv::colorcorrectionmatrix<XF_CCM_TYPE, XF_DST_T, XF_DST_T, XF_HEIGHT, XF_WIDTH, XF_NPPC, S_DEPTH>(ltm_in,
                                                                                                          lsc_out);

    xf::cv::xf_QuatizationDithering<XF_DST_T, XF_LTM_T, XF_HEIGHT, XF_WIDTH, 256, 1024, XF_NPPC>(lsc_out, aecin);

    if (AEC_EN) {
        xf::cv::autoexposurecorrection<XF_LTM_T, XF_LTM_T, SIN_CHANNEL_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC>(
            aecin, _dst, hist_aec1, hist_aec2);

        ColorMat2AXIvideo<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC>(_dst, m_axis_video);
    } else {
        ColorMat2AXIvideo<XF_LTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPC>(aecin, m_axis_video);
    }
}

/*********************************************************************************
 * Function:    ISPPipeline_accel
 * Parameters:  Stream of input/output pixels, image resolution
 * Return:
 * Description:
 **********************************************************************************/
// void ISPPipeline_accel(HW_STRUCT_REG HwReg, InVideoStrm_t& s_axis_video, OutVideoStrm_t& m_axis_video) {
void ISPPipeline_accel(
    uint16_t width, uint16_t height, uint16_t bayer_phase, InVideoStrm_t& s_axis_video, OutVideoStrm_t& m_axis_video) {
// Create AXI Streaming Interfaces for the core
// clang-format off
#pragma HLS INTERFACE axis port=&s_axis_video register
#pragma HLS INTERFACE axis port=&m_axis_video register

#pragma HLS INTERFACE s_axilite port=width bundle=CTRL offset=0x0010
#pragma HLS INTERFACE s_axilite port=height bundle=CTRL offset=0x0018
#pragma HLS INTERFACE s_axilite port=bayer_phase bundle=CTRL offset=0x0030
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL


	// clang-format off
#pragma HLS ARRAY_PARTITION variable=hist0 complete dim=1
#pragma HLS ARRAY_PARTITION variable=hist1 complete dim=1

    // clang-format on
    if (!flag) {
        ISPpipeline(s_axis_video, m_axis_video, height, width, hist0, hist1, histogram0, histogram1, igain_0, igain_1);
        flag = 1;

    } else {
        ISPpipeline(s_axis_video, m_axis_video, height, width, hist1, hist0, histogram1, histogram0, igain_1, igain_0);
        flag = 0;
    }
}
