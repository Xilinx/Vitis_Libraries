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

static uint32_t hist0_awb[3][HIST_SIZE] = {0};
static uint32_t hist1_awb[3][HIST_SIZE] = {0};

static uint32_t hist0_aec[AEC_HIST_SIZE] = {0};
static uint32_t hist1_aec[AEC_HIST_SIZE] = {0};

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
template <int SRC_T,
          int DST_T,
          int MAX_ROWS_HEIGHT,
          int MAX_COLS_WIDTH,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_imgInput,
          int XFCVDEPTH_hdr_out>
void HDR_extract_merge(xf::cv::Mat<SRC_T, MAX_ROWS_HEIGHT, MAX_COLS_WIDTH, NPC, XFCVDEPTH_imgInput>& imgInput1,
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

template <int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_demosaic_out, int XFCVDEPTH_ltm_in>
void function_awb(xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_demosaic_out>& demosaic_out,
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

    if (USE_AWB) {
        fifo_awb<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(
            demosaic_out, ltm_in, hist0, hist1, gain0, gain1, height, width, thresh);
    } else {
        fifo_copy<OUT_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_demosaic_out, XFCVDEPTH_ltm_in>(
            demosaic_out, ltm_in, height, width);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_rggb_out_aec, int XFCVDEPTH_aec_out>
void function_aec(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_rggb_out_aec>& rggb_out,
                  xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_aec_out>& aec_out,
                  unsigned short height,
                  unsigned short width,
                  float aec_pawb,
                  uint32_t aec_hist0[AEC_HIST_SIZE],
                  uint32_t aec_hist1[AEC_HIST_SIZE]) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    uint32_t aec_config = (int)(aec_pawb * 256); // thresh_aec int Q24_8 format change to Q16_16 format

    float aec_inputMin = 0.0f;
    float aec_inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;
    float aec_outputMin = 0.0f;
    float aec_outputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;

// clang-format off
#pragma HLS DATAFLOW
    // clang-format on
    if (USE_AEC) {
        xf::cv::autoexposurecorrection_sin<IN_TYPE, IN_TYPE, AEC_SIN_CHANNEL_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX,
                                           XF_USE_URAM, XFCVDEPTH_rggb_out_aec, XFCVDEPTH_aec_out, AEC_HIST_SIZE>(
            rggb_out, aec_out, aec_hist0, aec_hist1, aec_config, aec_inputMin, aec_inputMax, aec_outputMin,
            aec_outputMax);

    } else {
        fifo_copy<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_rggb_out_aec, XFCVDEPTH_aec_out>(
            rggb_out, aec_out, height, width);
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
                 unsigned int c1,
                 unsigned int c2,
                 unsigned short height,
                 unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    constexpr int Q_VAL = 1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX)); /* Used in xf_QuatizationDithering */

    if (USE_LTM) {
        xf::cv::LTM<OUT_TYPE, XF_GTM_T, BLOCK_HEIGHT, BLOCK_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_ltm_in,
                    XFCVDEPTH_aecin>::process(ltm_in, blk_height, blk_width, omin_r, omax_r, omin_w, omax_w, aecin);
    } else if (USE_GTM) {
        xf::cv::gtm<OUT_TYPE, XF_GTM_T, IN_TYPE, SIN_CHANNEL_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_ltm_in,
                    XFCVDEPTH_aecin>(ltm_in, aecin, mean1, mean2, L_max1, L_max2, L_min1, L_min2, c1, c2);
    } else if (USE_QND) {
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
                            unsigned short height,
                            unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    if (USE_RGBIR) {
        function_rgbir<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_hdr_out, XFCVDEPTH_rggb_out,
                       XFCVDEPTH_fullir_out, XFCVDEPTH_3XWIDTH>(hdr_out, rggb_out, img_out_ir, R_IR_C1_wgts,
                                                                R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts,
                                                                sub_wgts, height, width);
    } else {
        fifo_copy<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_hdr_out, XFCVDEPTH_rggb_out>(
            hdr_out, rggb_out, height, width);
    }
}

template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_bpc_in, int XFCVDEPTH_bpc_out, int N>
void function_degamma(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_bpc_in>& bpc_out,
                      xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_bpc_out>& dgamma_out,
                      uint32_t params[3][N][3],
                      unsigned short bayerp,
                      unsigned short height,
                      unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    if (USE_DEGAMMA) {
        degamma<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, DEGAMMA_KP, XFCVDEPTH_bpc_in, XF_CV_DEPTH_bpc_out>(
            bpc_out, dgamma_out, params, bayerp);
    } else {
        fifo_copy<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_bpc_in, XF_CV_DEPTH_bpc_out>(
            bpc_out, dgamma_out, height, width);
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
                    ap_uint<LUT_PTR_WIDTH>* lut,
                    int lutDim) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on
    xf::cv::Mat<XF_32FC3, SQ_LUTDIM, LUT_DIM, XF_NPPCX, XFCVDEPTH_3dlut> lutMat(lutDim * lutDim, lutDim);

#pragma HLS DATAFLOW
    xf::cv::Array2xfMat<LUT_PTR_WIDTH, XF_32FC3, SQ_LUTDIM, LUT_DIM, XF_NPPCX, XFCVDEPTH_3dlut>(lut, lutMat);
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
                         ap_uint<LUT_PTR_WIDTH>* lut,
                         int lutDim,
                         unsigned short height,
                         unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    if (USE_3DLUT) {
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
                       signed int ccm_matrix[3][3],
                       signed int offsetarray[3],
                       unsigned short height,
                       unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    if (USE_CCM) {
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

    xf::cv::bgr2yuyv<XF_GTM_T, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc>(csc_out, _imgOutput);
    xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc>(_imgOutput, img_out);
}

template <int GTM_T, int ROWS, int COLS, int NPC = 1, int XFCVDEPTH_csc>
void function_csc_or_mat_array(xf::cv::Mat<GTM_T, ROWS, COLS, NPC, XFCVDEPTH_csc>& csc_out,
                               ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                               unsigned short height,
                               unsigned short width) {
// clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    if (USE_CSC) {
        function_csc<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc>(csc_out, img_out, height, width);
    } else {
        xf::cv::xfMat2Array<OUTPUT_PTR_WIDTH, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XFCVDEPTH_csc>(csc_out, img_out);
    }
}

void ISPpipeline(ap_uint<INPUT_PTR_WIDTH>* img_inp,         /* Array2xfMat */
                 ap_uint<OUTPUT_PTR_WIDTH>* img_out,        /* xfMat2Array */
                 ap_uint<OUTPUT_PTR_WIDTH>* img_out_ir,     /* xfMat2Array */
                 unsigned short height,                     /* Height of the image */
                 unsigned short width,                      /* Width of the image */
                 short* wr_hls,                             /* HDR extract merge */
                 int params_decompand[3][4][3],             /* hdr_decompand */
                 char R_IR_C1_wgts[25],                     /* rgbir */
                 char R_IR_C2_wgts[25],                     /* rgbir */
                 char B_at_R_wgts[25],                      /* rgbir */
                 char IR_at_R_wgts[9],                      /* rgbir */
                 char IR_at_B_wgts[9],                      /* rgbir */
                 char sub_wgts[4],                          /* rgbir */
                 uint32_t aec_hist0[AEC_HIST_SIZE],         /* function_aec */
                 uint32_t aec_hist1[AEC_HIST_SIZE],         /* function_aec */
                 uint16_t pawb,                             /*Used to calculate thresh which is used in function_awb */
                 unsigned short bayerp,                     /* hdr_decompand, degamma*/
                 uint32_t params_degamma[3][DEGAMMA_KP][3], /* degamma */
                 uint16_t rgain,                            /* gaincontrol */
                 uint16_t bgain,                            /* gaincontrol */
                 uint32_t hist0[3][HIST_SIZE],              /* function_awb */
                 uint32_t hist1[3][HIST_SIZE],              /* function_awb */
                 int gain0[3],                              /* function_awb */
                 int gain1[3],                              /* function_awb */
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
                 unsigned int c1,                                                                          /* gtm */
                 unsigned int c2,                                                                          /* gtm */
                 unsigned char gamma_lut[256 * 3], /* gamma correction */
                 ap_uint<LUT_PTR_WIDTH>* lut,      /* 3dlut */
                 int lutDim,
                 signed int ccm_config_1[3][3],
                 signed int ccm_config_2[3],
                 unsigned short ggain) { /* 3dlut */
                                         // clang-format off
#pragma HLS INLINE OFF
    // clang-format on

    int mat_height, mat_width;

    if (USE_HDR_FUSION) {
        mat_height = height * 2;
        mat_width = width + NUM_H_BLANK;

    } else {
        mat_height = height;
        mat_width = width;
    }

    xf::cv::Mat<IN_TYPE, MAX_HEIGHT, MAX_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput> imgInput1(mat_height, mat_width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput1> imgInput2(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_hdr_out> hdr_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_rggb_out> rggb_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_blc_out> blc_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_bpc_out> bpc_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_aec_out> aec_out(height, width);
    xf::cv::Mat<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_dgamma_out> dgamma_out(height, width);
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

    float thresh_aec = (float)pawb / 256;
    float thresh_awb = 0.9;

    float inputMax = (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1; // 65535.0f;

    float mul_fact = (float)(inputMax / (inputMax - BLACK_LEVEL));
    unsigned int blc_config_1 = (int)(mul_fact * 65536); // mul_fact int Q16_16 format
    unsigned int blc_config_2 = BLACK_LEVEL;
    if (USE_HDR) {
        if (USE_HDR_FUSION) {
            xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, MAX_HEIGHT, MAX_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput>(
                img_inp, imgInput1);
            HDR_extract_merge<IN_TYPE, IN_TYPE, MAX_HEIGHT, MAX_WIDTH, XF_HEIGHT, XF_WIDTH, XF_NPPCX,
                              XF_CV_DEPTH_imgInput, XF_CV_DEPTH_hdr_out>(imgInput1, hdr_out, wr_hls, height, width);
        } else {
            xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput1>(
                img_inp, imgInput2);
            xf::cv::hdr_decompand<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput1,
                                  XF_CV_DEPTH_hdr_out>(imgInput2, hdr_out, params_decompand, bayerp);
        }
    } else {
        xf::cv::Array2xfMat<INPUT_PTR_WIDTH, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_imgInput1>(img_inp,
                                                                                                            hdr_out);
    }

    function_rgbir_or_fifo<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_hdr_out, XF_CV_DEPTH_rggb_out,
                           XF_CV_DEPTH_fullir_out, XF_CV_DEPTH_3XWIDTH>(hdr_out, rggb_out, img_out_ir, R_IR_C1_wgts,
                                                                        R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts,
                                                                        IR_at_B_wgts, sub_wgts, height, width);

    function_aec<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_rggb_out_aec, XF_CV_DEPTH_aec_out>(
        rggb_out, aec_out, height, width, thresh_aec, aec_hist0, aec_hist1);

    xf::cv::blackLevelCorrection<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 16, 15, 1, XF_CV_DEPTH_aec_out,
                                 XF_CV_DEPTH_blc_out>(aec_out, blc_out, blc_config_2, blc_config_1);

    xf::cv::badpixelcorrection<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, 0, XF_USE_URAM, XF_CV_DEPTH_blc_out,
                               XF_CV_DEPTH_bpc_out>(blc_out, bpc_out);

    function_degamma<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_bpc_out, XF_CV_DEPTH_dgamma_out,
                     DEGAMMA_KP>(bpc_out, dgamma_out, params_degamma, bayerp, height, width);

    xf::cv::Lscdistancebased<IN_TYPE, IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_dgamma_out,
                             XF_CV_DEPTH_lsc_out>(dgamma_out, LscOut);

    xf::cv::gaincontrol<IN_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_lsc_out, XF_CV_DEPTH_gain_out>(
        LscOut, gain_out, rgain, bgain, ggain, bayerp);

    xf::cv::demosaicing<IN_TYPE, OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_USE_URAM, XF_CV_DEPTH_gain_out,
                        XF_CV_DEPTH_demosaic_out>(gain_out, demosaic_out, bayerp);

    function_awb<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_demosaic_out, XF_CV_DEPTH_awb_out>(
        demosaic_out, awb_out, hist0, hist1, gain0, gain1, height, width, thresh_awb);

    function_ccm_fifo<OUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_awb_out, XF_CV_DEPTH_ccm>(
        awb_out, ccm_out, ccm_config_1, ccm_config_2, height, width);

    if (OUT_TYPE == XF_8UC3) {
        fifo_copy<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_ccm, XF_CV_DEPTH_aecin>(ccm_out, aecin,
                                                                                                         height, width);
    } else {
        function_tm<OUT_TYPE, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_ccm, XF_CV_DEPTH_aecin>(
            ccm_out, aecin, omin_r, omax_r, omin_w, omax_w, blk_height, blk_width, mean1, mean2, L_max1, L_max2, L_min1,
            L_min2, c1, c2, height, width);
    }

    xf::cv::gammacorrection<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_aecin, XF_CV_DEPTH_dst>(
        aecin, _dst, gamma_lut);

    function_3dlut_fifo<XF_GTM_T, XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_dst, XF_CV_DEPTH_lut_out,
                        XF_CV_DEPTH_3dlut>(_dst, lut_out, lut, lutDim, height, width);

    function_csc_or_mat_array<XF_GTM_T, XF_HEIGHT, XF_WIDTH, XF_NPPCX, XF_CV_DEPTH_lut_out>(lut_out, img_out, height,
                                                                                            width);
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
                       int height,                                  /* Height of the image */
                       int width,                                   /* Width of the image */
                       short wr_hls[NO_EXPS * XF_NPPCX * W_B_SIZE], /* HDR */
                       int params_decompand[3][4][3],               /* Decompand */
                       char R_IR_C1_wgts[25],                       /* rgbir2bayer */
                       char R_IR_C2_wgts[25],                       /* rgbir2bayer */
                       char B_at_R_wgts[25],                        /* rgbir2bayer */
                       char IR_at_R_wgts[9],                        /* rgbir2bayer */
                       char IR_at_B_wgts[9],                        /* rgbir2bayer */
                       char sub_wgts[4],                            /* rgbir2bayer */
                       uint16_t pawb,         /* used to calculate thresh which is used in function_awb */
                       unsigned short bayerp, /* hdr_decompand, degamma */
                       uint32_t params_degamma[3][DEGAMMA_KP][3], /*degamma*/
                       uint16_t rgain,                            /* gaincontrol */
                       uint16_t bgain,                            /* gaincontrol */
                       int blk_height,                            /* LTM */
                       int blk_width,                             /* LTM */
                       uint32_t c1,                               /* gtm */
                       uint32_t c2,                               /* gtm */
                       unsigned char gamma_lut[256 * 3],          /* gammacorrection */
                       ap_uint<LUT_PTR_WIDTH>* lut,               /* lut3d */
                       int lutDim,
                       signed int ccm_config_1[3][3],
                       signed int ccm_config_2[3],
                       unsigned short ggain) { /* lut3d */
                                               // clang-format off

#pragma HLS INTERFACE m_axi port=img_inp          offset=slave bundle=gmem1 
#pragma HLS INTERFACE m_axi port=img_out          offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi port=img_out_ir       offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi port=wr_hls           offset=slave bundle=gmem4
#pragma HLS INTERFACE m_axi port=params_decompand offset=slave bundle=gmem5
#pragma HLS INTERFACE m_axi port=R_IR_C1_wgts     offset=slave bundle=gmem6
#pragma HLS INTERFACE m_axi port=R_IR_C2_wgts     offset=slave bundle=gmem7
#pragma HLS INTERFACE m_axi port=B_at_R_wgts      offset=slave bundle=gmem8
#pragma HLS INTERFACE m_axi port=IR_at_R_wgts     offset=slave bundle=gmem9
#pragma HLS INTERFACE m_axi port=IR_at_B_wgts     offset=slave bundle=gmem10
#pragma HLS INTERFACE m_axi port=sub_wgts         offset=slave bundle=gmem11
#pragma HLS INTERFACE m_axi port=params_degamma   offset=slave bundle=gmem12
#pragma HLS INTERFACE m_axi port=gamma_lut        offset=slave bundle=gmem13
#pragma HLS INTERFACE m_axi port=lut              offset=slave bundle=gmem14
#pragma HLS INTERFACE m_axi port=ccm_config_1     bundle=gmem15 offset=slave
#pragma HLS INTERFACE m_axi port=ccm_config_2     bundle=gmem16 offset=slave

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
        ISPpipeline(img_inp, img_out, img_out_ir, height, width, wr_hls_tmp, params_decompand, R_IR_C1_wgts,
                    R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, hist0_aec, hist1_aec, pawb, bayerp,
                    params_degamma, rgain, bgain, hist0_awb, hist1_awb, igain_0, igain_1, omin[0], omax[0], omin[1],
                    omax[1], blk_height, blk_width, mean2, mean1, L_max2, L_max1, L_min2, L_min1, c1, c2, gamma_lut,
                    lut, lutDim, ccm_config_1, ccm_config_2, ggain);

        flag = 1;

    } else {
        ISPpipeline(img_inp, img_out, img_out_ir, height, width, wr_hls_tmp, params_decompand, R_IR_C1_wgts,
                    R_IR_C2_wgts, B_at_R_wgts, IR_at_R_wgts, IR_at_B_wgts, sub_wgts, hist1_aec, hist0_aec, pawb, bayerp,
                    params_degamma, rgain, bgain, hist1_awb, hist0_awb, igain_1, igain_0, omin[1], omax[1], omin[0],
                    omax[0], blk_height, blk_width, mean1, mean2, L_max1, L_max2, L_min1, L_min2, c1, c2, gamma_lut,
                    lut, lutDim, ccm_config_1, ccm_config_2, ggain);

        flag = 0;
    }
}
}
