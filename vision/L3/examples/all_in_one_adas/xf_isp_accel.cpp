/*
 * Copyright 2022 Xilinx, Inc.
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
static uint32_t hist0_awb[3][HIST_SIZE] = {0};
static uint32_t hist1_awb[3][HIST_SIZE] = {0};

static int igain_0[3] = {0};
static int igain_1[3] = {0};

static constexpr int MinMaxVArrSize = LTMTile<BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX>::MinMaxVArrSize;
static constexpr int MinMaxHArrSize = LTMTile<BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX>::MinMaxHArrSize;
static constexpr int BILINEAR_INTERPOLATE_TYPE_C = XF_32FC3;

static XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omin[2][MinMaxVArrSize][MinMaxHArrSize];
static XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omax[2][MinMaxVArrSize][MinMaxHArrSize];

static ap_ufixed<16, 4> mean1 = 0;
static ap_ufixed<16, 4> mean2 = 0;
static ap_ufixed<16, 4> L_max1 = 0.1;
static ap_ufixed<16, 4> L_max2 = 0.1;
static ap_ufixed<16, 4> L_min1 = 10;
static ap_ufixed<16, 4> L_min2 = 10;

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_imgInput, int XFCVDEPTH_hdr_out>
void fifo_copy_hdr(xf::cv::Mat<SRC_T, ROWS * 2, COLS + NUM_H_BLANK, NPC, XFCVDEPTH_imgInput>& imgInput1,
                   xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_hdr_out>& hdr_out,
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
            tmp_src = imgInput1.read(readindex++);
            hdr_out.write(writeindex++, tmp_src);
        }
    }
}
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_imgInput, int XFCVDEPTH_hdr_out>
void function_extract_merge(xf::cv::Mat<SRC_T, ROWS * 2, COLS + NUM_H_BLANK, NPC, XFCVDEPTH_imgInput>& imgInput1,
                            xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_hdr_out>& hdr_out,
                            short wr_hls[NO_EXPS * XF_NPPCX * W_B_SIZE],
                            unsigned short height,
                            unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_imgInput> LEF_Img(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_imgInput> SEF_Img(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    xf::cv::extractExposureFrames<IN_TYPE, NUM_V_BLANK_LINES, NUM_H_BLANK, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM,
                                  XFCVDEPTH_imgInput, XFCVDEPTH_imgInput, XFCVDEPTH_imgInput>(imgInput1, LEF_Img,
                                                                                              SEF_Img);
    xf::cv::Hdrmerge_bayer<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM, NO_EXPS, W_B_SIZE,
                           XFCVDEPTH_imgInput, XFCVDEPTH_imgInput, XFCVDEPTH_hdr_out>(LEF_Img, SEF_Img, hdr_out,
                                                                                      wr_hls);
}

template <int SRC_T,
          int DST_T,
          int MAX_ROWS,
          int MAX_COLS,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_imgInput,
          int XFCVDEPTH_hdr_out>
void function_HDR(xf::cv::Mat<SRC_T, MAX_ROWS, MAX_COLS, NPC, XFCVDEPTH_imgInput>& imgInput1,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_hdr_out>& hdr_out,
                  short* wr_hls,
                  unsigned short height,
                  unsigned short width,
                  unsigned short mode_reg) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    ap_uint<16> mode = (ap_uint<16>)mode_reg;
    ap_uint<1> mode_hdr = mode.range(HDR_EN_LSB, HDR_EN_LSB);
    if (mode_hdr) {
        function_extract_merge<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_imgInput, XFCVDEPTH_hdr_out>(
            imgInput1, hdr_out, wr_hls, height, width);
    } else {
        fifo_copy_hdr<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_imgInput, XFCVDEPTH_hdr_out>(
            imgInput1, hdr_out, height, width);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_demosaic_out, int XFCVDEPTH_ltm_in>
void fifo_copy(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_demosaic_out>& demosaic_out,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_ltm_in>& ltm_in,
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
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_demosaic_out, int XFCVDEPTH_ltm_in>
void fifo_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_demosaic_out>& demosaic_out,
              xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_ltm_in>& ltm_in,
              uint32_t hist0[3][HIST_SIZE],
              uint32_t hist1[3][HIST_SIZE],
              int gain0[3],
              int gain1[3],
              unsigned short height,
              unsigned short width,
              float thresh) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_ltm_in> impop(height, width);
    uint32_t awb_config = (int)(thresh * 256); // thresh_awb int Q24_8 format change to Q16_16 format

    float inputMin = 0.0f;
    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;
    float outputMin = 0.0f;
    float outputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;
                                                                       // clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    if (WB_TYPE) {
        xf::cv::AWBhistogram<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM, WB_TYPE, HIST_SIZE,
                             XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(demosaic_out, impop, hist0, awb_config, inputMin,
                                                                       inputMax, outputMin, outputMax);
        xf::cv::AWBNormalization<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, WB_TYPE, HIST_SIZE,
                                 XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(impop, ltm_in, hist1, awb_config, inputMin,
                                                                           inputMax, outputMin, outputMax);
    } else {
        xf::cv::AWBChannelGain<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XFCVDEPTH_demosaic_out,
                               XFCVDEPTH_ltm_in>(demosaic_out, impop, awb_config, gain0);
        xf::cv::AWBGainUpdate<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XFCVDEPTH_demosaic_out,
                              XFCVDEPTH_ltm_in>(impop, ltm_in, awb_config, gain1);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_demosaic_out, int XFCVDEPTH_ltm_in>
void function_awb(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_demosaic_out>& demosaic_out,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_ltm_in>& ltm_in,
                  uint32_t hist0[3][HIST_SIZE],
                  uint32_t hist1[3][HIST_SIZE],
                  int gain0[3],
                  int gain1[3],
                  unsigned short height,
                  unsigned short width,
                  unsigned short mode_reg,
                  float thresh) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    ap_uint<16> mode = (ap_uint<16>)mode_reg;
    ap_uint<1> mode_flg = mode.range(AWB_EN_LSB, AWB_EN_LSB);

    if (mode_flg) {
        fifo_awb<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(
            demosaic_out, ltm_in, hist0, hist1, gain0, gain1, height, width, thresh);
    } else {
        fifo_copy<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(
            demosaic_out, ltm_in, height, width);
    }
}

template <int DST_T, int GTM_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_ltm_in, int XFCVDEPTH_aecin>
void function_tm(xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_ltm_in>& ltm_in,
                 xf::cv::Mat<GTM_T, ROWS, COLS, NPC, XFCVDEPTH_aecin>& aecin,
                 XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omin_r[MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omax_r[MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omin_w[MinMaxVArrSize][MinMaxHArrSize],
                 XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omax_w[MinMaxVArrSize][MinMaxHArrSize],
                 int blk_height,
                 int blk_width,
                 ap_ufixed<16, 4>& mean1,
                 ap_ufixed<16, 4>& mean2,
                 ap_ufixed<16, 4>& L_max1,
                 ap_ufixed<16, 4>& L_max2,
                 ap_ufixed<16, 4>& L_min1,
                 ap_ufixed<16, 4>& L_min2,
                 uint32_t c1,
                 uint32_t c2,
                 unsigned short mode_reg,
                 unsigned short height,
                 unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    ap_uint<16> mode = (ap_uint<16>)mode_reg;
    ap_uint<1> mode_ltm = mode.range(LTM_EN_LSB, LTM_EN_LSB);
    ap_uint<1> mode_gtm = mode.range(GTM_EN_LSB, GTM_EN_LSB);
    ap_uint<1> mode_qnd = mode.range(QnD_EN_LSB, QnD_EN_LSB);

    if (mode_ltm) {
        xf::cv::LTM<OUT_TYPE, XF_GTM_T, BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_ltm_in,
                    XFCVDEPTH_aecin>::process(ltm_in, blk_height, blk_width, omin_r, omax_r, omin_w, omax_w, aecin);
    } else if (mode_gtm) {
        xf::cv::gtm<OUT_TYPE, XF_GTM_T, IN_TYPE, SIN_CHANNEL_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_ltm_in,
                    XFCVDEPTH_aecin>(ltm_in, aecin, mean1, mean2, L_max1, L_max2, L_min1, L_min2, c1, c2);
    } else if (mode_qnd) {
        xf::cv::xf_QuatizationDithering<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, 256, Q_VAL, XF_NPPCX, XF_USE_URAM,
                                        XFCVDEPTH_ltm_in, XFCVDEPTH_aecin>(ltm_in, aecin);

    } else {
        fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_ltm_in, XFCVDEPTH_aecin>(ltm_in, aecin,
                                                                                                        height, width);
    }
}

template <int SRC_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_hdr_out,
          int XFCVDEPTH_rggb_out,
          int XFCVDEPTH_fullir_out,
          int XFCVDEPTH_3XWIDTH>
void function_rgbir(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_hdr_out>& hdr_out,
                    xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_rggb_out>& rggb_out,
                    ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,
                    char R_IR_C1_wgts[25],
                    char R_IR_C2_wgts[25],
                    char B_at_R_wgts[25],
                    char IR_at_R_wgts[9],
                    char IR_at_B_wgts[9],
                    char sub_wgts[4],
                    unsigned short height,
                    unsigned short width) {
// clang-format off
 #pragma HLS INLINE OFF
    // clang-format on

    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_fullir_out> fullir_out(height, width);
// clang-format off
 #pragma HLS DATAFLOW
    // clang-format on

    xf::cv::rgbir2bayer<FILTERSIZE1, FILTERSIZE2, XF_BAYER_PATTERN, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX,
                        XF_BORDER_CONSTANT, XF_USE_URAM, XFCVDEPTH_hdr_out, XFCVDEPTH_rggb_out, XFCVDEPTH_fullir_out,
                        XFCVDEPTH_3XWIDTH>(hdr_out, R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts,
                                           sub_wgts, rggb_out, fullir_out);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_fullir_out>(fullir_out,
                                                                                                         img_out_ir);
}

template <int SRC_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_hdr_out,
          int XFCVDEPTH_rggb_out,
          int XFCVDEPTH_fullir_out,
          int XFCVDEPTH_3XWIDTH>
void function_rgbir_or_fifo(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_hdr_out>& hdr_out,
                            xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_rggb_out>& rggb_out,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,
                            char R_IR_C1_wgts[25],
                            char R_IR_C2_wgts[25],
                            char B_at_R_wgts[25],
                            char IR_at_R_wgts[9],
                            char IR_at_B_wgts[9],
                            char sub_wgts[4],
                            unsigned short mode_reg,
                            unsigned short height,
                            unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    ap_uint<16> mode = (ap_uint<16>)mode_reg;
    ap_uint<1> mode_rgbir = mode.range(RGBIR_EN_LSB, RGBIR_EN_LSB);

    if (mode_rgbir) {
        function_rgbir<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_hdr_out, XFCVDEPTH_rggb_out,
                       XFCVDEPTH_fullir_out, XFCVDEPTH_3XWIDTH>(hdr_out, rggb_out, img_out_ir, R_IR_C1_wgts,
                                                                R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts,
                                                                sub_wgts, height, width);
    } else {
        fifo_copy<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_hdr_out, XFCVDEPTH_rggb_out>(
            hdr_out, rggb_out, height, width);
    }
}

template <int DST_T,
          int GTM_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_dst,
          int XFCVDEPTH_lutout,
          int XFCVDEPTH_3dlut>
void function_3dlut(xf::cv::Mat<GTM_T, ROWS, COLS, NPC, XFCVDEPTH_dst>& _dst,
                    xf::cv::Mat<GTM_T, ROWS, COLS, NPC, XFCVDEPTH_lutout>& lut_out,
                    ap_uint<INPUT_PTR_WIDTH>* lut,
                    int lutDim) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    xf::cv::Mat<XF_32FC3, SQ_LUTDIM, LUT_DIM, XF_NPPCX, XFCVDEPTH_3dlut> lutMat(lutDim * lutDim, lutDim);

#pragma HLS DATAFLOW
    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, XF_32FC3, SQ_LUTDIM, LUT_DIM, XF_NPPCX, XFCVDEPTH_3dlut>(lut, lutMat);
    xf::cv::lut3d<LUT_DIM, SQ_LUTDIM, XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM, XFCVDEPTH_dst,
                  XFCVDEPTH_3dlut, XFCVDEPTH_lutout>(_dst, lutMat, lut_out, lutDim);
}

template <int DST_T,
          int GTM_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_dst,
          int XFCVDEPTH_lutout,
          int XFCVDEPTH_3dlut>
void function_3dlut_fifo(xf::cv::Mat<GTM_T, ROWS, COLS, NPC, XFCVDEPTH_dst>& _dst,
                         xf::cv::Mat<GTM_T, ROWS, COLS, NPC, XFCVDEPTH_lutout>& lut_out,
                         ap_uint<INPUT_PTR_WIDTH>* lut,
                         int lutDim,
                         unsigned short mode_reg,
                         unsigned short height,
                         unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    ap_uint<16> mode = (ap_uint<16>)mode_reg;
    ap_uint<1> mode_lut3d = mode.range(LUT3D_EN_LSB, LUT3D_EN_LSB);

    if (mode_lut3d) {
        function_3dlut<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_dst, XFCVDEPTH_lutout,
                       XFCVDEPTH_3dlut>(_dst, lut_out, lut, lutDim);

    } else {
        fifo_copy<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_dst, XFCVDEPTH_lutout>(_dst, lut_out,
                                                                                                      height, width);
    }
}

template <int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_dst, int XFCVDEPTH_ccm>
void function_ccm_fifo(xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_dst>& awb_out,
                       xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_ccm>& ccm_out,
                       unsigned short mode_reg,
                       signed int ccm_matrix[3][3],
                       signed int offsetarray[3],
                       unsigned short height,
                       unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    ap_uint<16> mode = (ap_uint<16>)mode_reg;
    ap_uint<1> mode_ccm = mode.range(CCM_EN_LSB, CCM_EN_LSB);

    if (mode_ccm) {
        xf::cv::colorcorrectionmatrix<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_dst, XFCVDEPTH_ccm>(
            awb_out, ccm_out, ccm_matrix, offsetarray);
    } else {
        fifo_copy<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_dst, XFCVDEPTH_ccm>(awb_out, ccm_out,
                                                                                                   height, width);
    }
}

template <int GTM_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_csc>
void function_csc(xf::cv::Mat<GTM_T, ROWS, COLS, NPC, XFCVDEPTH_csc>& csc_out,
                  ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                  unsigned short height,
                  unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    xf::cv::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc> _imgOutput(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    xf::cv::rgb2yuyv<XF_GTM_T, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc>(csc_out, _imgOutput);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc>(_imgOutput, img_out);
}

template <int GTM_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_csc>
void function_csc_or_mat_array(xf::cv::Mat<GTM_T, ROWS, COLS, NPC, XFCVDEPTH_csc>& csc_out,
                               ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                               unsigned short mode_reg,
                               unsigned short height,
                               unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    ap_uint<16> mode = (ap_uint<16>)mode_reg;
    ap_uint<1> mode_csc = mode.range(CSC_EN_LSB, CSC_EN_LSB);

    if (mode_csc) {
        function_csc<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc>(csc_out, img_out, height, width);
    } else {
        xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc>(csc_out, img_out);
    }
}

static constexpr int MAX_HEIGHT = XF_HEIGHT * 2;
static constexpr int MAX_WIDTH = XF_WIDTH + NUM_H_BLANK;

void ISPpipeline(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                 ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                 ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,
                 unsigned short mode_reg,
                 unsigned short height,
                 unsigned short width,
                 short* wr_hls,
                 char R_IR_C1_wgts[25],
                 char R_IR_C2_wgts[25],
                 char B_at_R_wgts[25],
                 char IR_at_R_wgts[9],
                 char IR_at_B_wgts[9],
                 char sub_wgts[4],
                 uint16_t rgain,
                 uint16_t bgain,
                 uint32_t hist0[3][HIST_SIZE], /* function_awb */
                 uint32_t hist1[3][HIST_SIZE], /* function_awb */
                 int gain0[3],                 /* function_awb */
                 int gain1[3],                 /* function_awb */
                 uint16_t pawb,
                 unsigned char gamma_lut[256 * 3],
                 XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omin_r[MinMaxVArrSize][MinMaxHArrSize], /* LTM */
                 XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omax_r[MinMaxVArrSize][MinMaxHArrSize], /* LTM */
                 XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omin_w[MinMaxVArrSize][MinMaxHArrSize], /* LTM */
                 XF_CTUNAME(BILINEAR_INTERPOLATE_TYPE_C, XF_NPPCX) omax_w[MinMaxVArrSize][MinMaxHArrSize], /* LTM */
                 int blk_height,                                                                           /* LTM */
                 int blk_width,                                                                            /* LTM */
                 ap_ufixed<16, 4>& mean1,                                                                  /* gtm */
                 ap_ufixed<16, 4>& mean2,                                                                  /* gtm */
                 ap_ufixed<16, 4>& L_max1,                                                                 /* gtm */
                 ap_ufixed<16, 4>& L_max2,                                                                 /* gtm */
                 ap_ufixed<16, 4>& L_min1,                                                                 /* gtm */
                 ap_ufixed<16, 4>& L_min2,                                                                 /* gtm */
                 uint32_t c1,                                                                              /* gtm */
                 uint32_t c2,                                                                              /* gtm */
                 ap_uint<INPUT_PTR_WIDTH>* lut,
                 int lutDim,
                 signed int ccm_config_1[3][3],
                 signed int ccm_config_2[3],
                 unsigned short bformat,
                 unsigned short ggain) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    int mat_height, mat_width;
    ap_uint<16> mode = (ap_uint<16>)mode_reg;
    ap_uint<1> mode_hdr = mode.range(HDR_EN_LSB, HDR_EN_LSB);

    if (mode_hdr) {
        mat_height = height * 2;
        mat_width = width + NUM_H_BLANK;
    } else {
        mat_height = height;
        mat_width = width;
    }

    xf::cv::Mat<IN_TYPE, MAX_HEIGHT, MAX_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput> imgInput1(mat_height, mat_width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_hdr_out> hdr_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_rggb_out> rggb_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_bpc_out> bpc_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_blc_out> blc_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_lsc_out> LscOut(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_gain_out> gain_out(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_demosaic_out> demosaic_out(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_awb_out> awb_out(height, width);
    xf::cv::Mat<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_ccm> ccm_out(height, width);
    xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_aecin> aecin(height, width);
    xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_dst> _dst(height, width);
    xf::cv::Mat<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_lut_out> lut_out(height, width);

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on

    float thresh = (float)pawb / 256;
    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;

    float mul_fact = (inputMax / (inputMax - BLACK_LEVEL));
    unsigned int blc_config_1 = (int)(mul_fact * 65536); // mul_fact int Q16_16 format
    unsigned int blc_config_2 = BLACK_LEVEL;

    xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, MAX_HEIGHT, MAX_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput>(img_inp,
                                                                                                         imgInput1);

    function_HDR<IN_TYPE, IN_TYPE, MAX_HEIGHT, MAX_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput,
                 XF_CV_DEPTH_hdr_out>(imgInput1, hdr_out, wr_hls, height, width, mode_reg);

    function_rgbir_or_fifo<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_hdr_out, XF_CV_DEPTH_rggb_out,
                           XF_CV_DEPTH_fullir_out, XF_CV_DEPTH_3XWIDTH>(
        hdr_out, rggb_out, img_out_ir, R_IR_C1_wgts, R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts,
        mode_reg, height, width);

    xf::cv::blackLevelCorrection<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 16, 15, 1, XF_CV_DEPTH_rggb_out,
                                 XF_CV_DEPTH_blc_out>(rggb_out, blc_out, blc_config_2, blc_config_1);

    xf::cv::badpixelcorrection<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, 0, XF_CV_DEPTH_blc_out, XF_CV_DEPTH_bpc_out>(
        blc_out, bpc_out);

    xf::cv::Lscdistancebased<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_bpc_out, XF_CV_DEPTH_lsc_out>(
        bpc_out, LscOut);

    xf::cv::gaincontrol<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_lsc_out, XF_CV_DEPTH_gain_out>(
        LscOut, gain_out, rgain, bgain, ggain, bformat);

    xf::cv::demosaicing<IN_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XF_CV_DEPTH_gain_out,
                        XF_CV_DEPTH_demosaic_out>(gain_out, demosaic_out, bformat);

    function_awb<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_demosaic_out, XF_CV_DEPTH_awb_out>(
        demosaic_out, awb_out, hist0, hist1, gain0, gain1, height, width, mode_reg, thresh);

    function_ccm_fifo<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_awb_out, XF_CV_DEPTH_ccm>(
        awb_out, ccm_out, mode_reg, ccm_config_1, ccm_config_2, height, width);

    if (OUT_TYPE == XF_8UC3) {
        fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_ccm, XF_CV_DEPTH_aecin>(ccm_out, aecin,
                                                                                                         height, width);
    } else {
        function_tm<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_ccm, XF_CV_DEPTH_aecin>(
            ccm_out, aecin, omin_r, omax_r, omin_w, omax_w, blk_height, blk_width, mean1, mean2, L_max1, L_max2, L_min1,
            L_min2, c1, c2, mode_reg, height, width);
    }
    xf::cv::gammacorrection<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_aecin, XF_CV_DEPTH_dst>(
        aecin, _dst, gamma_lut);

    function_3dlut_fifo<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_dst, XF_CV_DEPTH_lut_out,
                        XF_CV_DEPTH_3dlut>(_dst, lut_out, lut, lutDim, mode_reg, height, width);

    function_csc_or_mat_array<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_lut_out>(lut_out, img_out, mode_reg,
                                                                                            height, width);
}
/*********************************************************************************
 * Function:    ISPPipeline_accel
 * Parameters:  input and output image pointers, image resolution
 * Return:
 * Description:
 **********************************************************************************/
extern "C" {
void ISPPipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,           /* Array2xfMat */
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out,          /* xfMat2Array */
                       ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,       /* xfMat2Array */
                       int height,                                  /* HDR, rgbir2bayer, fifo_copy */
                       int width,                                   /* HDR, rgbir2bayer, fifo_copy */
                       short wr_hls[NO_EXPS * XF_NPPCX * W_B_SIZE], /* HDR */
                       uint16_t rgain,                              /* gaincontrol */
                       uint16_t bgain,                              /* gaincontrol */
                       char R_IR_C1_wgts[25],                       /* rgbir2bayer */
                       char R_IR_C2_wgts[25],                       /* rgbir2bayer */
                       char B_at_R_wgts[25],                        /* rgbir2bayer */
                       char IR_at_R_wgts[9],                        /* rgbir2bayer */
                       char IR_at_B_wgts[9],                        /* rgbir2bayer */
                       char sub_wgts[4],                            /* rgbir2bayer */
                       int blk_height,                              /* LTM */
                       int blk_width,                               /* LTM */
                       uint32_t c1,                                 /* gtm */
                       uint32_t c2,                                 /* gtm */
                       unsigned char gamma_lut[256 * 3],            /* gammacorrection */
                       unsigned short mode_reg,
                       ap_uint<INPUT_PTR_WIDTH>* lut, /* lut3d */
                       int lutDim,                    /* lut3d */
                       uint16_t pawb,                 /* used to calculate thresh which is used in function_awb */
                       signed int ccm_config_1[3][3],
                       signed int ccm_config_2[3],
                       unsigned short bformat,
                       unsigned short ggain) {
// clang-format off

#pragma HLS INTERFACE m_axi port=img_inp      offset=slave bundle=gmem1 
#pragma HLS INTERFACE m_axi port=img_out      offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=img_out_ir   offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=R_IR_C1_wgts offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi port=R_IR_C2_wgts offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi port=B_at_R_wgts  offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi port=IR_at_R_wgts offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi port=IR_at_B_wgts offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi port=sub_wgts     offset=slave bundle=gmem5
#pragma HLS INTERFACE m_axi port=gamma_lut    offset=slave bundle=gmem6
#pragma HLS INTERFACE m_axi port=wr_hls       offset=slave bundle=gmem7
#pragma HLS INTERFACE m_axi port=lut          offset=slave bundle=gmem8

#pragma HLS INTERFACE m_axi port=ccm_config_1     bundle=gmem9 offset=slave
#pragma HLS INTERFACE m_axi port=ccm_config_2     bundle=gmem10 offset=slave

#pragma HLS ARRAY_PARTITION variable=hist0_awb    complete dim=1
#pragma HLS ARRAY_PARTITION variable=hist1_awb    complete dim=1
#pragma HLS ARRAY_PARTITION variable=omin dim=1   complete
#pragma HLS ARRAY_PARTITION variable=omin dim=2   cyclic factor=2
#pragma HLS ARRAY_PARTITION variable=omin dim=3   cyclic factor=2
#pragma HLS ARRAY_PARTITION variable=omax dim=1   complete
#pragma HLS ARRAY_PARTITION variable=omax dim=2   cyclic factor=2
#pragma HLS ARRAY_PARTITION variable=omax dim=3   cyclic factor=2
    // clang-format on

    static short wr_hls_tmp[NO_EXPS * XF_NPPCX * W_B_SIZE];

WR_HLS_INIT_LOOP:
    for (int k = 0; k < XF_NPPCX; k++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=XF_NPPCX max=XF_NPPCX
        // clang-format on
        for (int i = 0; i < NO_EXPS; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=NO_EXPS max=NO_EXPS
            // clang-format on
            for (int j = 0; j < (W_B_SIZE); j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=W_B_SIZE max=W_B_SIZE
                // clang-format on
                wr_hls_tmp[(i + k * NO_EXPS) * W_B_SIZE + j] = wr_hls[(i + k * NO_EXPS) * W_B_SIZE + j];
            }
        }
    }

    if (!flag) {
        ISPpipeline(img_inp, img_out, img_out_ir, mode_reg, height, width, wr_hls_tmp, R_IR_C1_wgts, R_IR_C2_wgts,
                    B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, rgain, bgain, hist0_awb, hist1_awb, igain_0,
                    igain_1, pawb, gamma_lut, omin[0], omax[0], omin[1], omax[1], blk_height, blk_width, mean2, mean1,
                    L_max2, L_max1, L_min2, L_min1, c1, c2, lut, lutDim, ccm_config_1, ccm_config_2, bformat, ggain);
        flag = 1;

    } else {
        ISPpipeline(img_inp, img_out, img_out_ir, mode_reg, height, width, wr_hls_tmp, R_IR_C1_wgts, R_IR_C2_wgts,
                    B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, rgain, bgain, hist1_awb, hist0_awb, igain_1,
                    igain_0, pawb, gamma_lut, omin[1], omax[1], omin[0], omax[0], blk_height, blk_width, mean1, mean2,
                    L_max1, L_max2, L_min1, L_min2, c1, c2, lut, lutDim, ccm_config_1, ccm_config_2, bformat, ggain);
        flag = 0;
    }
}
}
