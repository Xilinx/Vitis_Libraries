/* Copyright 2023 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xf_isp_accel_config.h"

static bool flag = 0;
#if (XF_TM_TYPE == 0)
static constexpr int MinMaxVArrSize = LTMTile<BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, NPPCX>::MinMaxVArrSize;
static constexpr int MinMaxHArrSize = LTMTile<BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, NPPCX>::MinMaxHArrSize;

static XF_CTUNAME(BIL_T, NPPCX) omin[2][MinMaxVArrSize][MinMaxHArrSize];
static XF_CTUNAME(BIL_T, NPPCX) omax[2][MinMaxVArrSize][MinMaxHArrSize];
#endif
/************************************************************************************
 * Function:    AXIVideo2BayerMat
 * Parameters:  Multiple bayerWindow.getval AXI Stream, User Stream, Image Resolution
 * Return:      None
 * Description: Read data from multiple pixel/clk AXI stream into user defined stream
 ************************************************************************************/
template <int TYPE, int ROWS, int COLS, int NPPC, int XFCVDEPTH_BAYER>
void AXIVideo2BayerMat(InVideoStrm_t& bayer_strm, xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_BAYER>& bayer_mat) {
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

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_IN, int XFCVDEPTH_OUT>
void fifo_copy(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& demosaic_out,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_OUT>& ltm_in,
               unsigned short height,
               unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    ap_uint<13> row, col;
    int readindex = 0, writeindex = 0;

    ap_uint<13> img_width = width >> XF_BITSHIFT(NPC);

Row_Loop:
    for (row = 0; row < height; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
    // clang-format on
    Col_Loop:
        for (col = 0; col < img_width; col++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=COLS/NPC max=COLS/NPC
#pragma HLS pipeline
            // clang-format on
            XF_TNAME(SRC_T, NPC) tmp_src;
            tmp_src = demosaic_out.read(readindex++);
            ltm_in.write(writeindex++, tmp_src);
        }
    }
}

template <int TYPE, int ROWS, int COLS, int NPPC, int XFCVDEPTH_color>
void ColorMat2AXIvideo(xf::cv::Mat<TYPE, ROWS, COLS, NPPC, XFCVDEPTH_color>& color_mat, OutVideoStrm_t& color_strm) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    OutVideoStrmBus_t axi;

    int rows = color_mat.rows;
    int cols = color_mat.cols >> XF_BITSHIFT(NPPC);
    int idx = 0;

    XF_TNAME(TYPE, NPPC) srcpixel;

    const int m_pix_width = XF_PIXELWIDTH(TYPE, NPPC) * XF_NPIXPERCYCLE(NPPC);

    int depth = XF_DTPIXELDEPTH(XF_GTM_T, XF_NPPCX);

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

static uint32_t hist0_awb[3][HIST_SIZE] = {0};
static uint32_t hist1_awb[3][HIST_SIZE] = {0};

template <int IN_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int XF_CV_DEPTH_LSC_OUT_V,
          int XF_CV_DEPTH_GAIN_OUT_V>
void function_gaincontrol(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V>& in_img,
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V>& gaincontrol_out_mat,
    uint16_t rgain,
    uint16_t bgain,
    uint16_t ggain,
    uint16_t bformat,
    uint16_t height,
    uint16_t width,
    unsigned int funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_GAIN_EN_INDEX, XF_GAIN_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V,
                  XF_CV_DEPTH_LSC_OUT_V>(in_img, gaincontrol_out_mat, height, width);
    } else {
        xf::cv::gaincontrol<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V,
                            XF_CV_DEPTH_GAIN_OUT_V>(in_img, gaincontrol_out_mat, rgain, bgain, ggain, bformat);
    }
}

template <int IN_TYPE_V,
          int OUT_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          bool XF_USE_URAM_V,
          int XF_CV_DEPTH_IN_V,
          int XF_CV_DEPTH_OUT_V>
void function_demosaicing(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V>& gaincontrol_out_mat,
    xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& demosaicing_out_mat,
    unsigned short bformat,
    uint16_t height,
    uint16_t width,
    unsigned int funcs_bypass_config) {
    bool funcs_bypass_config_bool =
        ((ap_uint32_t)funcs_bypass_config).range(XF_DEMOSAICING_EN_INDEX, XF_DEMOSAICING_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V, XF_CV_DEPTH_IN_V>(
            gaincontrol_out_mat, demosaicing_out_mat, height, width);
    } else {
        xf::cv::demosaicing<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_USE_URAM_V, XF_CV_DEPTH_IN_V,
                            XF_CV_DEPTH_OUT_V>(gaincontrol_out_mat, demosaicing_out_mat, bformat);
    }
}

template <int IN_TYPE_V,
          int OUT_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          bool XF_USE_URAM_V,
          int WB_TYPE_V,
          int XF_CV_DEPTH_IN_V,
          int XF_CV_DEPTH_OUT_V>
void fifo_AWB(xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V>& demosaicing_out_mat,
              xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& AWB_out_mat,
              uint32_t hist0[3][HIST_SIZE],
              uint32_t hist1[3][HIST_SIZE],
              unsigned int awb_config,
              float inputMin,
              float inputMax,
              float outputMin,
              float outputMax,
              uint16_t height,
              uint16_t width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V> AWBhistogram_out_mat(height, width);
#pragma HLS DATAFLOW
    // clang-format on
    if (WB_TYPE_V) {
        xf::cv::AWBhistogram<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_USE_URAM_V, WB_TYPE_V,
                             HIST_SIZE, XF_CV_DEPTH_IN_V, XF_CV_DEPTH_OUT_V>(
            demosaicing_out_mat, AWBhistogram_out_mat, hist0, awb_config, inputMin, inputMax, outputMin, outputMax);

        xf::cv::AWBNormalization<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, WB_TYPE_V, HIST_SIZE,
                                 XF_CV_DEPTH_IN_V, XF_CV_DEPTH_OUT_V>(
            AWBhistogram_out_mat, AWB_out_mat, hist1, awb_config, inputMin, inputMax, outputMin, outputMax);
    }
}

template <int IN_TYPE_V,
          int OUT_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          bool XF_USE_URAM_V,
          int WB_TYPE_V,
          int XF_CV_DEPTH_IN_V,
          int XF_CV_DEPTH_OUT_V>
void function_AWB(xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V>& demosaicing_out_mat,
                  xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& AWB_out_mat,
                  uint32_t hist0[3][HIST_SIZE],
                  uint32_t hist1[3][HIST_SIZE],
                  unsigned int awb_config,
                  float inputMin,
                  float inputMax,
                  float outputMin,
                  float outputMax,
                  uint16_t height,
                  uint16_t width,
                  unsigned int funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_AWB_EN_INDEX, XF_AWB_EN_INDEX);

// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    // xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V> AWBhistogram_out_mat(height,width);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V, XF_CV_DEPTH_IN_V>(
            demosaicing_out_mat, AWB_out_mat, height, width);

    } else {
        fifo_AWB<OUT_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_USE_URAM_V, WB_TYPE_V,
                 XF_CV_DEPTH_IN_V, XF_CV_DEPTH_OUT_V>(demosaicing_out_mat, AWB_out_mat, hist0, hist1, awb_config,
                                                      inputMin, inputMax, outputMin, outputMax, height, width);

        // if (WB_TYPE_V) {
        //     xf::cv::AWBhistogram<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_USE_URAM_V,
        //     WB_TYPE_V,
        //                             HIST_SIZE, XF_CV_DEPTH_IN_V, XF_CV_DEPTH_OUT_V>(
        //         demosaicing_out_mat, AWBhistogram_out_mat, hist0, awb_config, inputMin, inputMax, outputMin,
        //         outputMax);

        //     xf::cv::AWBNormalization<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, WB_TYPE_V,
        //     HIST_SIZE,
        //                                 XF_CV_DEPTH_IN_V, XF_CV_DEPTH_OUT_V>(AWBhistogram_out_mat, AWB_out_mat,
        //                                                                     hist1, awb_config, inputMin, inputMax,
        //                                                                     outputMin,
        //                                                                     outputMax);
        // }
    }
}

template <int IN_TYPE_V,
          int OUT_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int XF_CV_DEPTH_IN_V,
          int XF_CV_DEPTH_OUT_V>
void function_colorcorrectionmatrix(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V>& AWBNormalization_out_mat,
    xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& colorcorrectionmatrix_out_mat,
    signed int ccm_matrix[3][3],
    signed int offsetarray[3],
    uint16_t height,
    uint16_t width,
    unsigned int funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_CCM_EN_INDEX, XF_CCM_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V, XF_CV_DEPTH_IN_V>(
            AWBNormalization_out_mat, colorcorrectionmatrix_out_mat, height, width);
    } else {
        xf::cv::colorcorrectionmatrix<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V,
                                      XF_CV_DEPTH_OUT_V>(AWBNormalization_out_mat, colorcorrectionmatrix_out_mat,
                                                         ccm_matrix, offsetarray);
    }
}

template <int OUT_TYPE_V,
          int XF_GTM_T_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int SCALE_FACTOR_V,
          int MAX_REPRESENTED_VALUE_V,
          int XF_NPPCX_V,
          bool XF_USE_URAM_V,
          int XF_CV_DEPTH_IN_V,
          int XF_CV_DEPTH_OUT_V>
void function_xf_QuatizationDithering(
    xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V>& colorcorrectionmatrix_out_mat,
    xf::cv::Mat<XF_GTM_T_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& xf_QuatizationDithering_out_mat,
    uint16_t height,
    uint16_t width,
    uint32_t funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_TM_EN_INDEX, XF_TM_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<OUT_TYPE_V, XF_GTM_T_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_IN_V, XF_CV_DEPTH_IN_V>(
            colorcorrectionmatrix_out_mat, xf_QuatizationDithering_out_mat, height, width);
    } else {
        xf::cv::xf_QuatizationDithering<OUT_TYPE_V, XF_GTM_T_V, XF_HEIGHT_V, XF_WIDTH_V, SCALE_FACTOR_V,
                                        MAX_REPRESENTED_VALUE_V, XF_NPPCX_V, XF_USE_URAM_V, XF_CV_DEPTH_IN_V,
                                        XF_CV_DEPTH_OUT_V>(colorcorrectionmatrix_out_mat,
                                                           xf_QuatizationDithering_out_mat);
    }
}

template <int IN_TYPE_V,
          int OUT_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int XF_CV_DEPTH_GAIN_OUT_V,
          int XF_CV_DEPTH_OUT_V>
void function_gammacorrection(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V>&
        xf_QuatizationDithering_out_mat,
    xf::cv::Mat<OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_OUT_V>& gammacorrection_out_mat,
    unsigned char gamma_lut[256 * 3],
    uint16_t height,
    uint16_t width,
    uint32_t funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_GAMMA_EN_INDEX, XF_GAMMA_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V,
                  XF_CV_DEPTH_GAIN_OUT_V>(xf_QuatizationDithering_out_mat, gammacorrection_out_mat, height, width);
    } else {
        xf::cv::gammacorrection<IN_TYPE_V, OUT_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V,
                                XF_CV_DEPTH_OUT_V>(xf_QuatizationDithering_out_mat, gammacorrection_out_mat, gamma_lut);
    }
}

template <int IN_TYPE_V,
          int XF_HEIGHT_V,
          int XF_WIDTH_V,
          int XF_NPPCX_V,
          int MUL_VALUE_WIDTH_V,
          int FL_POS_V,
          int USE_DSP_V,
          int XF_CV_DEPTH_LSC_OUT_V,
          int XF_CV_DEPTH_GAIN_OUT_V>
void function_blackLevelCorrection(
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V>& in_img,
    xf::cv::Mat<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_GAIN_OUT_V>& blackLevelCorrection_out_mat,
    unsigned int blc_config_1,
    unsigned int blc_config_2,
    uint16_t height,
    uint16_t width,
    unsigned int funcs_bypass_config) {
    bool funcs_bypass_config_bool = ((ap_uint32_t)funcs_bypass_config).range(XF_BLC_EN_INDEX, XF_BLC_EN_INDEX);

    if (funcs_bypass_config_bool) {
        fifo_copy<IN_TYPE_V, IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, XF_CV_DEPTH_LSC_OUT_V,
                  XF_CV_DEPTH_LSC_OUT_V>(in_img, blackLevelCorrection_out_mat, height, width);
    } else {
        xf::cv::blackLevelCorrection<IN_TYPE_V, XF_HEIGHT_V, XF_WIDTH_V, XF_NPPCX_V, MUL_VALUE_WIDTH_V, FL_POS_V,
                                     USE_DSP_V, XF_CV_DEPTH_GAIN_OUT_V, XF_CV_DEPTH_GAIN_OUT_V>(
            in_img, blackLevelCorrection_out_mat, blc_config_2, blc_config_1);
    }
}

void ISPpipeline(InVideoStrm_t& s_axis_video,
                 OutVideoStrm_t& m_axis_video,
                 uint16_t height,
                 uint16_t width,

                 uint16_t new_height,
                 uint16_t new_width,
#if XF_BLC_EN
                 unsigned int blc_config_1,
                 unsigned int blc_config_2,
#endif
#if XF_GAIN_EN
                 uint16_t rgain,
                 uint16_t bgain,
                 uint16_t ggain,
                 uint16_t bformat,
#endif
#if XF_AWB_EN
                 uint32_t hist0[3][HIST_SIZE],
                 unsigned int awb_config,
                 float inputMin,
                 float inputMax,
                 float outputMin,
                 float outputMax,
                 uint32_t hist1[3][HIST_SIZE],
#endif
#if XF_GAMMA_EN
                 unsigned char gamma_lut[256 * 3],
#endif
#if XF_CCM_EN
                 signed int ccm_matrix[3][3],
                 signed int offsetarray[3],
#endif
#if (XF_TM_TYPE == 0)
                 uint32_t blk_height,
                 uint32_t blk_width,
                 XF_CTUNAME(BIL_T, NPPCX) omin_r[MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(BIL_T, NPPCX) omax_r[MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(BIL_T, NPPCX) omin_w[MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(BIL_T, NPPCX) omax_w[MinMaxVArrSize][MinMaxHArrSize],
#endif
                 unsigned int funcs_bypass_config) {
// clang-format off
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_LSC_OUT> in_img(height, width);

    AXIVideo2BayerMat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT>(s_axis_video, in_img);

// blacklevel functioncall
#if XF_BLC_EN
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT> blackLevelCorrection_out_mat(height,
                                                                                                           width);

    function_blackLevelCorrection<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, MUL_VALUE_WIDTH, FL_POS, USE_DSP,
                                  XF_CV_DEPTH_GAIN_OUT, XF_CV_DEPTH_GAIN_OUT>(
        in_img, blackLevelCorrection_out_mat, blc_config_1, blc_config_2, height, width, funcs_bypass_config);
#endif

// gaincontrol functioncall
#if XF_GAIN_EN
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT> gaincontrol_out_mat(height, width);
#if XF_BLC_EN
    function_gaincontrol<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_LSC_OUT, XF_CV_DEPTH_GAIN_OUT>(
        blackLevelCorrection_out_mat, gaincontrol_out_mat, rgain, bgain, ggain, bformat, height, width,
        funcs_bypass_config);
#else
    function_gaincontrol<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_LSC_OUT, XF_CV_DEPTH_GAIN_OUT>(
        in_img, gaincontrol_out_mat, rgain, bgain, ggain, bformat, height, width, funcs_bypass_config);
#endif
#endif

    // demosaicing functioncall

    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> demosaicing_out_mat(height, width);
#if (XF_GAIN_EN == 1)
    function_demosaicing<IN_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM, XF_CV_DEPTH_IN,
                         XF_CV_DEPTH_OUT>(gaincontrol_out_mat, demosaicing_out_mat, bformat, height, width,
                                          funcs_bypass_config);

#elif (XF_BLC_EN == 1)
    function_demosaicing<IN_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM, XF_CV_DEPTH_IN,
                         XF_CV_DEPTH_OUT>(blackLevelCorrection_out_mat, demosaicing_out_mat, bformat, height, width,
                                          funcs_bypass_config);
#else
    function_demosaicing<IN_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM, XF_CV_DEPTH_IN,
                         XF_CV_DEPTH_OUT>(in_img, demosaicing_out_mat, bformat, height, width, funcs_bypass_config);

#endif

// awb functioncall

#if XF_AWB_EN
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> AWB_out_mat(height, width);
    function_AWB<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, WB_TYPE, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        demosaicing_out_mat, AWB_out_mat, hist0, hist1, awb_config, inputMin, inputMax, outputMin, outputMax, height,
        width, funcs_bypass_config);
#endif

// ccm functioncall
#if XF_CCM_EN
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> colorcorrectionmatrix_out_mat(height, width);
#if XF_AWB_EN
    function_colorcorrectionmatrix<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        AWB_out_mat, colorcorrectionmatrix_out_mat, ccm_matrix, offsetarray, height, width, funcs_bypass_config);
#else
    function_colorcorrectionmatrix<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        demosaicing_out_mat, colorcorrectionmatrix_out_mat, ccm_matrix, offsetarray, height, width,
        funcs_bypass_config);
#endif
#endif

    // TM functioncall

    xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> xf_QuatizationDithering_out_mat(height,
                                                                                                          width);
#if XF_CCM_EN
#if (T_8U)
    fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        colorcorrectionmatrix_out_mat, xf_QuatizationDithering_out_mat, height, width);

#else
#if (XF_TM_TYPE == 0)

    xf::cv::LTM<OUT_TYPE, XF_GTM_T, BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN,
                XF_CV_DEPTH_OUT>::process(colorcorrectionmatrix_out_mat, blk_height, blk_width, omin_r, omax_r, omin_w,
                                          omax_w, xf_QuatizationDithering_out_mat);
#elif (XF_TM_TYPE == 2)
    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    function_xf_QuatizationDithering<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, SCALE_FACTOR, Q_VAL, XF_NPPCX,
                                     XF_USE_URAM, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        colorcorrectionmatrix_out_mat, xf_QuatizationDithering_out_mat, height, width, funcs_bypass_config);
#endif
#endif
#elif (XF_AWB_EN == 1)
#if (T_8U)
    fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        AWB_out_mat, xf_QuatizationDithering_out_mat, height, width);

#else
#if (XF_TM_TYPE == 0)

    xf::cv::LTM<OUT_TYPE, XF_GTM_T, BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN,
                XF_CV_DEPTH_OUT>::process(AWB_out_mat, blk_height, blk_width, omin_r, omax_r, omin_w, omax_w,
                                          xf_QuatizationDithering_out_mat);
#elif (XF_TM_TYPE == 2)
    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    function_xf_QuatizationDithering<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, SCALE_FACTOR, Q_VAL, XF_NPPCX,
                                     XF_USE_URAM, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        AWB_out_mat, xf_QuatizationDithering_out_mat, height, width, funcs_bypass_config);
#endif
#endif
#else
#if (T_8U)
    fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        demosaicing_out_mat, xf_QuatizationDithering_out_mat, height, width);

#else
#if (XF_TM_TYPE == 0)

    xf::cv::LTM<OUT_TYPE, XF_GTM_T, BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_IN,
                XF_CV_DEPTH_OUT>::process(demosaicing_out_mat, blk_height, blk_width, omin_r, omax_r, omin_w, omax_w,
                                          xf_QuatizationDithering_out_mat);
#elif (XF_TM_TYPE == 2)
    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    function_xf_QuatizationDithering<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, SCALE_FACTOR, Q_VAL, XF_NPPCX,
                                     XF_USE_URAM, XF_CV_DEPTH_IN, XF_CV_DEPTH_OUT>(
        demosaicing_out_mat, xf_QuatizationDithering_out_mat, height, width, funcs_bypass_config);
#endif
#endif
#endif

// gammcorrection functioncall

#if XF_GAMMA_EN
    xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_OUT> gammacorrection_out_mat(height, width);
    function_gammacorrection<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_GAIN_OUT, XF_CV_DEPTH_OUT>(
        xf_QuatizationDithering_out_mat, gammacorrection_out_mat, gamma_lut, height, width, funcs_bypass_config);
#endif

    // resize functioncall
    xf::cv::Mat<XF_GTM_T, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_CV_DEPTH_LUT_OUT> resize_out_mat(new_height,
                                                                                                   new_width);
#if XF_GAMMA_EN
    xf::cv::resize<INTERPOLATION, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_USE_URAM,
                   MAXDOWNSCALE, XF_CV_DEPTH_LUT_IN, XF_CV_DEPTH_LUT_OUT>(gammacorrection_out_mat, resize_out_mat);
#else
    xf::cv::resize<INTERPOLATION, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_USE_URAM,
                   MAXDOWNSCALE, XF_CV_DEPTH_LUT_IN, XF_CV_DEPTH_LUT_OUT>(xf_QuatizationDithering_out_mat,
                                                                          resize_out_mat);
#endif

    ColorMat2AXIvideo<XF_GTM_T, XF_NEWHEIGHT, XF_NEWWIDTH, XF_NPPCX, XF_CV_DEPTH_LUT_OUT>(resize_out_mat, m_axis_video);
}

void ISPPipeline_accel(InVideoStrm_t& s_axis_video,
                       OutVideoStrm_t& m_axis_video,
                       unsigned int common_config,
#if XF_BLC_EN
                       unsigned int blc_config_1,
                       unsigned int blc_config_2,
#endif

                       unsigned int resize_config,

#if XF_GAIN_EN
                       unsigned int gain_control_config_1,
                       unsigned int gain_control_config_2,
#endif
#if XF_AWB_EN
                       unsigned int awb_config,
#endif
#if XF_GAMMA_EN
                       unsigned char gamma_lut[256 * 3],
#endif
#if XF_CCM_EN
                       signed int ccm_config_1[3][3],
                       signed int ccm_config_2[3],
#endif
#if (XF_TM_TYPE == 0)
                       uint32_t ltm_config,
#endif
                       unsigned int& pipeline_config_info,
                       unsigned int& max_supported_size,
                       unsigned int& funcs_available,
                       unsigned int& funcs_bypassable,
                       unsigned int funcs_bypass_config) {
// clang-format off
#pragma HLS INTERFACE axis port=s_axis_video register
#pragma HLS INTERFACE axis port=m_axis_video register
#pragma HLS INTERFACE s_axilite port=common_config bundle=CTRL offset=0x00010

#pragma HLS INTERFACE s_axilite port=resize_config bundle=CTRL offset=0x00078 

#if XF_GAIN_EN
#pragma HLS INTERFACE s_axilite port=gain_control_config_1 bundle=CTRL offset=0x00038
#pragma HLS INTERFACE s_axilite port=gain_control_config_2 bundle=CTRL offset=0x00040
#endif
#if XF_AWB_EN
#pragma HLS INTERFACE s_axilite port=awb_config	bundle=CTRL offset=0x00020
#endif
#if XF_GAMMA_EN
#pragma HLS INTERFACE s_axilite port=gamma_lut bundle=CTRL offset=0x02000
#endif
#if XF_CCM_EN
#pragma HLS INTERFACE s_axilite port=ccm_config_1 bundle=CTRL offset=0x04000
#pragma HLS INTERFACE s_axilite port=ccm_config_2 bundle=CTRL offset=0x04100
#endif
#if XF_BLC_EN
#pragma HLS INTERFACE s_axilite port=blc_config_1 bundle=CTRL offset=0x00028 
#pragma HLS INTERFACE s_axilite port=blc_config_2 bundle=CTRL offset=0x00030 
#endif
#if (XF_TM_TYPE == 0)
#pragma HLS INTERFACE s_axilite port=ltm_config bundle=CTRL offset=0x00058 
#endif


#pragma HLS INTERFACE s_axilite port=pipeline_config_info bundle=CTRL offset=0x00080 
#pragma HLS INTERFACE mode=ap_none port=pipeline_config_info 
#pragma HLS INTERFACE s_axilite port=max_supported_size bundle=CTRL offset=0x00090 
#pragma HLS INTERFACE mode=ap_none port=max_supported_size 
#pragma HLS INTERFACE s_axilite port=funcs_available bundle=CTRL offset=0x000a0 
#pragma HLS INTERFACE mode=ap_none port=funcs_available 
#pragma HLS INTERFACE s_axilite port=funcs_bypassable bundle=CTRL offset=0x000b0 
#pragma HLS INTERFACE mode=ap_none port=funcs_bypassable 
#pragma HLS INTERFACE s_axilite port=funcs_bypass_config bundle=CTRL offset=0x000c0
#pragma HLS INTERFACE s_axilite port=return bundle=CTRL
    // clang-format on
    // extracting height and width  //
    uint16_t height, width;
    height = (uint16_t)(common_config >> 16);
    width = (uint16_t)(common_config);

    // extracting new_height and new_width  //
    uint16_t new_height, new_width;
    new_height = (uint16_t)(resize_config >> 16);
    new_width = (uint16_t)(resize_config);

#if XF_GAIN_EN
    // extracting rgain and bgain   //
    uint16_t rgain, bgain;
    bgain = (uint16_t)(gain_control_config_1 >> 16);
    rgain = (uint16_t)(gain_control_config_1);

    uint16_t bformat;
    bformat = (uint16_t)(gain_control_config_2 >> 16);

    uint16_t ggain;
    ggain = (uint16_t)(gain_control_config_2);
#endif

#if XF_AWB_EN
    //  awb specific data   //
    float inputMin = 0.0f;
    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;
    float outputMin = 0.0f;
    float outputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;
// clang-format off

#pragma HLS ARRAY_PARTITION variable=hist0_awb    complete dim=1
#pragma HLS ARRAY_PARTITION variable=hist1_awb    complete dim=1
#endif

#if (XF_TM_TYPE == 0)
uint32_t block_height = (uint16_t)(ltm_config >> 16);;
uint32_t block_width= (uint16_t)(ltm_config);
#pragma HLS ARRAY_PARTITION variable=omin dim=1 complete
#pragma HLS ARRAY_PARTITION variable=omin dim=2 cyclic factor=2
#pragma HLS ARRAY_PARTITION variable=omin dim=3 cyclic factor=2

#pragma HLS ARRAY_PARTITION variable=omax dim=1 complete
#pragma HLS ARRAY_PARTITION variable=omax dim=2 cyclic factor=2
#pragma HLS ARRAY_PARTITION variable=omax dim=3 cyclic factor=2

#endif
    // clang-format on

    max_supported_size = (XF_WIDTH << MAX_WIDTH_INDEX) | (XF_HEIGHT << MAX_HEIGHT_INDEX);

    pipeline_config_info = (IN_C_TYPE << IN_C_TYPE_INDEX) | (IN_BW_MODE << IN_BW_MODE_INDEX) |
                           (OUT_C_TYPE << OUT_C_TYPE_INDEX) | (OUT_BW_MODE << OUT_BW_MODE_INDEX) |
                           (NPPCX << NPPCX_INDEX) | (NUM_STREAMS << NUM_STREAMS_INDEX);

    funcs_available = (XF_BLC_EN << XF_BLC_EN_INDEX) | (XF_GAIN_EN << XF_GAIN_EN_INDEX) |
                      (XF_AWB_EN << XF_AWB_EN_INDEX) | (XF_CCM_EN << XF_CCM_EN_INDEX) | (XF_TM_EN << XF_TM_EN_INDEX) |
                      (XF_TM_TYPE << XF_TM_TYPE_INDEX) | (XF_GAMMA_EN << XF_GAMMA_EN_INDEX);

    funcs_bypassable = (XF_BLC_BYPASS_EN << XF_BLC_EN_INDEX) | (XF_GAIN_BYPASS_EN << XF_GAIN_EN_INDEX) |
                       (XF_AWB_BYPASS_EN << XF_AWB_EN_INDEX) | (XF_CCM_BYPASS_EN << XF_CCM_EN_INDEX) |
                       (XF_GAMMA_BYPASS_EN << XF_GAMMA_EN_INDEX);
    if (!flag) {
        ISPpipeline(s_axis_video, m_axis_video, height, width,

                    new_height, new_width,

#if XF_BLC_EN
                    blc_config_1, blc_config_2,
#endif
#if XF_GAIN_EN
                    rgain, bgain, ggain, bformat,
#endif
#if XF_AWB_EN
                    hist0_awb, awb_config, inputMin, inputMax, outputMin, outputMax, hist1_awb,
#endif
#if XF_GAMMA_EN
                    gamma_lut,
#endif
#if XF_CCM_EN
                    ccm_config_1, ccm_config_2,
#endif
#if (XF_TM_TYPE == 0)
                    block_height, block_width, omin[0], omax[0], omin[1], omax[1],
#endif
                    funcs_bypass_config);
        flag = 1;
    } else {
        ISPpipeline(s_axis_video, m_axis_video, height, width,

                    new_height, new_width,

#if XF_BLC_EN
                    blc_config_1, blc_config_2,
#endif
#if XF_GAIN_EN
                    rgain, bgain, ggain, bformat,
#endif
#if XF_AWB_EN
                    hist1_awb, awb_config, inputMin, inputMax, outputMin, outputMax, hist0_awb,
#endif
#if XF_GAMMA_EN
                    gamma_lut,
#endif
#if XF_CCM_EN
                    ccm_config_1, ccm_config_2,
#endif
#if (XF_TM_TYPE == 0)
                    block_height, block_width, omin[0], omax[0], omin[1], omax[1],
#endif
                    funcs_bypass_config);
        flag = 0;
    }
}